"""Power-profile diagnostics and adapters to existing production-C model fitting."""
from collections import Counter
import hashlib
import json
from pathlib import Path

from power_model_data import select_samples
from vofa_csv import HALF, UINT32, read_capture

WHEELS = ('lf', 'rf', 'lb', 'rb')
STEADY = dict(window_ms=200, max_gap_ms=40, command_span=150, rpm_span=100,
              max_meter_age_ms=20, max_motor_age_ms=20)


def canonical_power_rows(capture):
    if capture['schema'] != 4:
        raise ValueError('static power fitting requires schema 4 evidence')
    rows = []
    for raw in capture['rows']:
        if 'error' in raw:
            rows.append({'tick_ms': 'nan'})
            continue
        valid = raw['valid']
        row = dict(tick_ms=(raw['tick_ms'] - max(0, raw['cycle_age_ms'])) & UINT32,
            meter_sequence=raw['meter_sequence'], submitted=int(valid & 7 == 7 and
                raw['cycle_age_ms'] >= 0 and raw['final_age_ms'] >= 0),
            meter_online=int(bool(valid & 8)), meter_age_ms=raw['meter_age_ms'],
            enabled_mask=(raw['wheel_masks'] >> 4) & 15, online_mask=raw['wheel_masks'] & 15,
            load_w=raw['meter_power_w'])
        for index, wheel in enumerate(WHEELS):
            row[f'command{index}'] = raw[f'final_current_{wheel}']
            row[f'rpm{index}'] = raw[f'actual_rpm_{wheel}']
            row[f'motor_age_ms_{index}'] = raw[f'wheel_age_ms_{wheel}']
        rows.append(row)
    return rows


def _configuration_matches(row, configuration):
    return bool(row['valid'] & 512) and (
        row['config_fingerprint'] == configuration['fingerprint'] and
        row['config_version'] == configuration['config_version'] and
        row['provenance'] & 3 == configuration['tuning_source'])


def _check_configuration(configuration):
    try:
        from vofa_config import validate_power_manifest
        if not validate_power_manifest(configuration):
            raise ValueError('configuration artifact fingerprint or contents do not match')
    except (ImportError, KeyError, TypeError) as error:
        raise ValueError('configuration artifact cannot be verified: ' + str(error)) from error


def _steady(capture):
    canonical = canonical_power_rows(capture)
    groups, group, previous = [], [], None
    for raw, normalized in zip(capture['rows'], canonical):
        if 'error' not in raw:
            boundary = previous is not None and (
                ((raw['tick_ms'] - previous['tick_ms']) & UINT32) >= HALF or
                raw['config_fingerprint'] != previous['config_fingerprint'] or
                raw['config_version'] != previous['config_version'])
            if boundary and group:
                groups.append(group)
                group = []
            previous = raw
        group.append(normalized)
    if group:
        groups.append(group)
    accepted, rejected = [], Counter()
    for rows in groups:
        samples, quality = select_samples(rows, 'vofa4', STEADY)
        accepted.extend(samples)
        rejected.update(quality['rejected'])
    return accepted, dict(total_rows=len(canonical), accepted_rows=len(accepted),
                         rejected=dict(rejected), missing_evidence=[])


def analyze_power(capture, configuration=None):
    if configuration is not None:
        _check_configuration(configuration)
    _, quality = _steady(capture)
    rows = [r for r in capture['rows'] if 'error' not in r]
    matched = configuration is not None and bool(rows) and all(_configuration_matches(r, configuration) for r in rows)
    identities = sorted({(r['config_fingerprint'], r['config_version'], r['provenance'] & 3) for r in rows})
    limitations = ['20_ms_snapshots_do_not_reconstruct_fast_control_history',
                  'cap_echo_equality_is_not_command_acknowledgement',
                  'hardware_validation_not_performed', 'independent_trials_required_for_fitting']
    if not matched:
        limitations.append('actual_power_configuration_unmatched')
    phases, phase = [], None
    for r in capture['rows']:
        if 'error' in r or not r['valid'] & 1:
            phase = None
            continue
        key = (r['state'], r['target_power_w'], r['config_fingerprint'])
        if phase is None or phase['key'] != key or ((r['tick_ms'] - phase['last_tick_ms']) & UINT32) > 40:
            phase = dict(key=key, first_tick_ms=r['tick_ms'], last_tick_ms=r['tick_ms'],
                snapshots=0, maximum_excess_j=None, minimum_guard_scale=None,
                meter_min_w=None, meter_max_w=None, failed_can_snapshots=0)
            phases.append(phase)
        phase['last_tick_ms'] = r['tick_ms']
        phase['snapshots'] += 1
        phase['failed_can_snapshots'] += not bool(r['valid'] & 4)
        phase['maximum_excess_j'] = max(phase['maximum_excess_j'] or 0, r['excess_energy_j'])
        phase['minimum_guard_scale'] = r['guard_scale'] if phase['minimum_guard_scale'] is None else min(phase['minimum_guard_scale'], r['guard_scale'])
        if r['valid'] & 8:
            for key_name, function in (('meter_min_w', min), ('meter_max_w', max)):
                phase[key_name] = r['meter_power_w'] if phase[key_name] is None else function(phase[key_name], r['meter_power_w'])
    for phase in phases:
        state, target, fingerprint = phase.pop('key')
        phase.update(state=state, target_w=target, config_fingerprint=fingerprint,
                     observed_duration_ms=(phase['last_tick_ms'] - phase['first_tick_ms']) & UINT32)
    return dict(kind='power', steady=quality, phases=phases, actual_configuration_matched=matched,
        observed_configurations=[dict(fingerprint=f, version=v, source=s) for f, v, s in identities],
        parameter_change_validated=False, hardware_validated=False, limitations=limitations)


def fit_power_trials(trials, configuration, build_dir, instrument_error_w=.1):
    """Fit complete independent CSV trials; never split neighboring rows at random."""
    _check_configuration(configuration)
    from power_model_c import Bridge, Model
    from power_model_fit import fit_model
    records, sessions, hashes, ids, measurements = [], {}, set(), set(), []
    for trial in trials:
        if not isinstance(trial, dict) or not all(isinstance(trial.get(k), str) and trial[k].strip()
                for k in ('id', 'path', 'session', 'split')) or type(trial.get('synthetic')) is not bool:
            raise ValueError('each independent trial requires id/path/session/split and explicit synthetic boolean')
        if trial['split'] not in ('train', 'calibration', 'test') or trial['id'] in ids:
            raise ValueError('duplicate trial id or unsupported split')
        if trial['session'] in sessions and sessions[trial['session']] != trial['split']:
            raise ValueError('independent trial session leakage across splits')
        sessions[trial['session']] = trial['split']
        ids.add(trial['id'])
        capture = read_capture(trial['path'])
        if capture['schema'] != 4:
            raise ValueError('power fitting requires schema 4')
        if capture['sha256'] in hashes:
            raise ValueError('reused capture content; independent trials required')
        hashes.add(capture['sha256'])
        good = [r for r in capture['rows'] if 'error' not in r]
        if not good or not all(_configuration_matches(r, configuration) for r in good):
            raise ValueError('actual power configuration does not match captured trial')
        # CSV display names, textual number formatting and VOFA time columns are
        # not new measurement evidence. Reject full reexports and overlapping
        # subsets before assigning any trial to training or validation.
        normalized = [json.dumps({key: value for key, value in r.items() if key != 'line'},
                                 sort_keys=True, allow_nan=False) for r in good]
        evidence = {hashlib.sha256(value.encode()).hexdigest() for value in normalized}
        for previous_evidence in measurements:
            if len(evidence & previous_evidence) >= .95 * min(len(evidence), len(previous_evidence)):
                raise ValueError('reused or overlapping decoded measurements; reexporting is not an independent trial')
        measurements.append(evidence)
        measurement_sha256 = hashlib.sha256('\n'.join(normalized).encode()).hexdigest()
        samples, quality = _steady(capture)
        records.append(dict(**trial, format='vofa4', source=capture['path'], sha256=capture['sha256'],
                            measurement_sha256=measurement_sha256, samples=samples, quality=quality))
    if {r['split'] for r in records} != {'train', 'calibration', 'test'}:
        raise ValueError('independent train, calibration and test trials are required')
    if isinstance(instrument_error_w, bool) or not isinstance(instrument_error_w, (float, int)) or not 0 <= instrument_error_w < 1e6:
        raise ValueError('instrument_error_w must be finite and nonnegative')
    manifest = dict(schema=1, boundary='chassis_load', instrument_error_w=instrument_error_w,
        steady=STEADY, experiments=trials, configuration=configuration,
        units=dict(command='dji_command_raw', speed='rotor_rpm', power='W'))
    dataset = dict(manifest=manifest, experiments=records,
        manifest_sha256=hashlib.sha256(json.dumps(manifest, sort_keys=True, allow_nan=False).encode()).hexdigest())
    models = tuple(Model(**{name: m[name] for name, _ in Model._fields_}) for m in configuration['models'])
    bridge = Bridge(build_dir)
    try:
        report, model = fit_model(dataset, bridge, baseline_models=models)
        report['normalized_capture_identity'] = [dict(id=e['id'], sha256=e['measurement_sha256']) for e in records]
        return report, model
    finally:
        bridge.close()
