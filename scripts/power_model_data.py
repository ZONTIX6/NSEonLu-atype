"""Explicit capture contracts and conservative steady-state sample selection."""
from collections import Counter, deque
import csv
import hashlib
import json
import math
from pathlib import Path

WHEELS = ('lf', 'rf', 'lb', 'rb')
UNITS = {'command': 'dji_command_raw', 'speed': 'rotor_rpm', 'power': 'W'}
SPLITS = ('train', 'calibration', 'test')


def number(value):
    if isinstance(value, bool):
        raise ValueError('boolean is not a numeric measurement')
    result = float(value)
    if not math.isfinite(result) or abs(result) > 1e30:
        raise ValueError('non-finite numeric value')
    return result


def integer(value, maximum=0xffffffff, minimum=0):
    result = number(value)
    if result != int(result) or not minimum <= result <= maximum:
        raise ValueError('integer outside declared range')
    return int(result)


def read_sample(row, fmt):
    """Missing legacy evidence stays None; it is never converted into success."""
    if None in row or any(value is None for value in row.values()):
        raise ValueError('CSV row width does not match its header')
    if fmt == 'aligned_static':
        return dict(tick=integer(row['tick_ms']), event=integer(row['meter_sequence']),
            submitted=integer(row['submitted'], 1), online=integer(row['meter_online'], 1),
            age=number(row['meter_age_ms']), enabled=integer(row['enabled_mask'], 15),
            motor_online=integer(row['online_mask'], 15), motor_age=None,
            power=number(row['load_w']), flags=0,
            command=[integer(row[f'command{w}'], 32767, -32768) for w in range(4)],
            rpm=[number(row[f'rpm{w}']) for w in range(4)])
    if fmt == 'legacy26':
        return dict(tick=integer(row['tick']), event=integer(row['meter_tick_ms']),
            submitted=None, online=integer(row['meter_online'], 1),
            age=number(row['meter_age_ms']), enabled=integer(row['motor_enabled_mask'], 15),
            motor_online=None, motor_age=None, power=number(row['p_meter_cw'])/100, flags=0,
            command=[integer(row[f'i_{w}_cmd'], 32767, -32768) for w in WHEELS],
            rpm=[number(row[f'n_{w}_rpm']) for w in WHEELS])
    return dict(tick=integer(row['tick_ms']), event=integer(row['meter_tick_ms']),
        submitted=None, online=None, age=number(row['meter_age_ms']),
        enabled=integer(row['enabled_mask'], 15), motor_online=None,
        motor_age=[number(row[f'motor_age_ms_{w}']) for w in WHEELS],
        power=number(row['measured_w']), flags=integer(row['flags'], 255),
        command=[integer(row[f'final_current_{w}'], 32767, -32768) for w in WHEELS],
        rpm=[number(row[f'actual_rpm_{w}']) for w in WHEELS])


def select_samples(rows, fmt, steady):
    accepted, rejected, window = [], Counter(), deque()
    event, tick, elapsed = None, None, 0
    total = 0
    for row in rows:
        total += 1
        try:
            sample = read_sample(row, fmt)
        except (ValueError, TypeError, KeyError, OverflowError):
            rejected['numeric'] += 1
            window.clear()
            continue
        dt = 0 if tick is None else (sample['tick'] - tick) & 0xffffffff
        if dt >= 0x80000000:
            rejected['order'] += 1
            window.clear()
            continue
        # Control rows and meter events have independent clocks. A repeated
        # meter value is not another fit sample, but its control state still
        # reveals command changes, failures and gaps inside the steady window.
        elapsed += dt
        tick = sample['tick']
        advance = 1 if event is None else (sample['event']-event) & 0xffffffff
        if advance >= 0x80000000:
            rejected['order'] += 1
            window.clear()
            continue
        new_event = advance != 0
        if new_event:
            event = sample['event']
        reason = None
        if sample['submitted'] == 0:
            reason = 'submission'
        elif sample['online'] == 0 or not 0 <= sample['age'] <= steady['max_meter_age_ms']:
            reason = 'stale'
        elif (sample['enabled'] != 15 or sample['motor_online'] not in (None, 15) or
              (sample['motor_age'] is not None and any(
                  not 0 <= a <= steady['max_motor_age_ms'] for a in sample['motor_age']))):
            reason = 'motor_health'
        elif sample['flags'] & 0x8b:
            reason = 'capture_flags'
        elif sample['power'] < 0:
            reason = 'negative_power'
        elif any(abs(u) > 15000 for u in sample['command']) or any(abs(n) > 32768 for n in sample['rpm']):
            reason = 'domain'
        elif any(u*n < 0 for u, n in zip(sample['command'], sample['rpm'])):
            reason = 'braking'
        elif any((u == 0) != (n == 0) for u, n in zip(sample['command'], sample['rpm'])):
            reason = 'ambiguous_quadrant'
        if reason:
            rejected[reason] += 1
            window.clear()
            continue
        if dt > steady['max_gap_ms']:
            window.clear()
        sample['elapsed_ms'] = elapsed
        window.append(sample)
        while len(window) > 1 and elapsed - window[1]['elapsed_ms'] >= steady['window_ms']:
            window.popleft()
        if not new_event:
            rejected['duplicate'] += 1
            continue
        stable = elapsed-window[0]['elapsed_ms'] >= steady['window_ms']
        for field, span in (('command', 'command_span'), ('rpm', 'rpm_span')):
            stable &= all(max(s[field][w] for s in window)-min(s[field][w] for s in window)
                          <= steady[span] for w in range(4))
        if stable:
            accepted.append(sample)
        else:
            rejected['not_steady'] += 1
    evidence = [] if fmt == 'aligned_static' else [
        'submission_unavailable', 'receive_time_only', 'meter_sequence_unavailable',
        'motor_online_unavailable']
    return accepted, dict(total_rows=total, accepted_rows=len(accepted),
                          rejected=dict(rejected), missing_evidence=evidence)


def load_manifest(path):
    path = Path(path).resolve()
    raw = path.read_bytes()
    manifest = json.loads(raw.decode('utf-8-sig'))
    if not isinstance(manifest, dict):
        raise ValueError('manifest must be a JSON object')
    if type(manifest.get('schema')) is not int or manifest['schema'] != 1:
        raise ValueError('manifest schema must be 1')
    if manifest.get('units') != UNITS:
        raise ValueError('units must be dji_command_raw, rotor_rpm and W')
    if manifest.get('boundary') not in ('chassis_load', 'unknown'):
        raise ValueError('boundary must be chassis_load or unknown; do not mix power boundaries')
    error = number(manifest['instrument_error_w'])
    if error < 0:
        raise ValueError('instrument_error_w must be nonnegative')
    steady = manifest['steady']
    for key in ('window_ms', 'max_gap_ms', 'command_span', 'rpm_span',
                'max_meter_age_ms', 'max_motor_age_ms'):
        steady[key] = number(steady[key])
        if steady[key] < 0 or (key in ('window_ms', 'max_gap_ms') and steady[key] <= 0):
            raise ValueError(f'invalid steady.{key}')
    if steady['max_gap_ms'] > steady['window_ms']:
        raise ValueError('max_gap_ms must not exceed steady window_ms')
    experiments = manifest['experiments']
    if not isinstance(experiments, list) or not experiments:
        raise ValueError('experiments must be a nonempty list')
    ids, paths, hashes, sessions = set(), set(), set(), {}
    records = []
    for entry in experiments:
        eid, session, split, fmt = (entry[k] for k in ('id', 'session', 'split', 'format'))
        if not isinstance(eid, str) or not eid.strip() or eid in ids:
            raise ValueError('experiment id must be unique and nonempty')
        if not isinstance(session, str) or not session.strip():
            raise ValueError('session must name a complete independent experiment')
        if split not in SPLITS or fmt not in ('aligned_static', 'legacy26', 'cpc1'):
            raise ValueError('unsupported split or capture format')
        if type(entry.get('synthetic')) is not bool:
            raise ValueError('every experiment must declare synthetic as a boolean')
        if session in sessions and sessions[session] != split:
            raise ValueError('session leakage across train/calibration/test')
        source = (path.parent / entry['path']).resolve()
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        if source in paths or digest in hashes:
            raise ValueError('reused capture file/content; independent experiment splits required')
        ids.add(eid)
        paths.add(source)
        hashes.add(digest)
        sessions[session] = split
        with source.open(encoding='utf-8-sig', newline='') as stream:
            reader = csv.DictReader(stream)
            if not reader.fieldnames or len(reader.fieldnames) != len(set(reader.fieldnames)):
                raise ValueError(f'{eid}: missing or duplicate CSV headers')
            samples, quality = select_samples(reader, fmt, steady)
        records.append(dict(**entry, sha256=digest, source=str(source),
                            samples=samples, quality=quality))
    if {r['split'] for r in records} != set(SPLITS):
        raise ValueError('train, calibration and test experiments are all required')
    return dict(manifest=manifest, manifest_sha256=hashlib.sha256(raw).hexdigest(),
                experiments=records)
