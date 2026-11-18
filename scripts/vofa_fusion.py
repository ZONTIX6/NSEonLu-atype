"""Replay recorded paired navigation inputs with the production C fusion filter."""
import ctypes as ct
import hashlib
import math
from pathlib import Path
import statistics
import subprocess
import sys

from vofa_csv import ROOT, FILTER_CONFIG_FIELDS, continuous_attempt_segments


class FusionBridge:
    def __init__(self, directory):
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        self.sources = [ROOT/'tests/vofa_analysis/fusion_bridge.c',
                        ROOT/'modules/algorithm/chassis_velocity_fusion.c',
                        ROOT/'modules/algorithm/chassis_velocity_fusion.h']
        library = directory / ('vofa-fusion.dll' if sys.platform == 'win32' else 'vofa-fusion.so')
        subprocess.run(['gcc', '-shared', '-fPIC', '-std=c11', '-O2', '-fno-fast-math',
            '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'modules/algorithm'),
            *map(str, self.sources[:2]), '-lm', '-o', str(library)],
            check=True, capture_output=True, text=True)
        self.lib = ct.CDLL(str(library.resolve()))
        self.lib.VofaFusionStateSize.argtypes = []
        self.lib.VofaFusionStateSize.restype = ct.c_size_t
        self.lib.VofaFusionReset.argtypes = [ct.c_void_p]
        self.lib.VofaFusionReset.restype = None
        self.lib.VofaFusionUpdate.argtypes = [ct.c_void_p, ct.POINTER(ct.c_float),
            ct.POINTER(ct.c_uint32), ct.POINTER(ct.c_uint32), ct.POINTER(ct.c_float), ct.c_uint32,
            ct.POINTER(ct.c_float), ct.POINTER(ct.c_uint32)]
        self.lib.VofaFusionUpdate.restype = None

    def close(self):
        if self.lib is not None:
            import _ctypes
            if sys.platform == 'win32':
                _ctypes.FreeLibrary(self.lib._handle)
            else:
                _ctypes.dlclose(self.lib._handle)
            self.lib = None

    def new_state(self):
        state = ct.create_string_buffer(self.lib.VofaFusionStateSize())
        self.lib.VofaFusionReset(state)
        return state

    def update(self, state, row, candidate=None):
        settings = [row[key] for key in FILTER_CONFIG_FIELDS]
        if candidate is not None:
            settings[:4] = [candidate.get(key, settings[i]) for i, key in enumerate(FILTER_CONFIG_FIELDS[:4])]
        config = (ct.c_float * 4)(*settings[:4])
        limits = (ct.c_uint32 * 3)(*settings[4:])
        times = (ct.c_uint32 * 3)(row['call_us'], row['wheel_us'], row['imu_us'])
        values = (ct.c_float * 4)(row['wheel_n_x_mps'], row['wheel_n_y_mps'],
                                 row['accel_n_x_mps2'], row['accel_n_y_mps2'])
        output, metadata = (ct.c_float * 2)(), (ct.c_uint32 * 3)()
        self.lib.VofaFusionUpdate(state, config, limits, times, values, row['attempt_flags'], output, metadata)
        return dict(velocity_n_mps=list(output), valid=bool(metadata[0] & 1), fused=bool(metadata[0] & 2),
                    sample_us=metadata[1], initialized=bool(metadata[2]))


def _config_valid(row):
    return (row.get('config_source', -1) in (0, 1, 2, 3) and
        all(math.isfinite(row.get(key, 0)) and row.get(key, 0) > 0 for key in FILTER_CONFIG_FIELDS[:4]) and
        all(0 < row.get(key, 0) < 0x80000000 for key in ('fusion_max_age_us', 'fusion_max_gap_us')) and
        0 <= row.get('fusion_max_skew_us', -1) < 0x80000000)


def _variance(values):
    return dict(samples=len(values), mean=statistics.mean(values), variance=statistics.variance(values))


def _response_sample(metric, output, row, previous):
    if not output['valid']:
        return None
    velocity = output['velocity_n_mps']
    metric['samples'] += 1
    metric['peak_speed_mps'] = max(metric['peak_speed_mps'], math.hypot(*velocity))
    metric['posterior_discrepancy_squared'] += math.dist(velocity, [row['wheel_n_x_mps'], row['wheel_n_y_mps']])**2
    if previous is not None:
        delta = (row['call_us'] - previous[0]) & 0xffffffff
        if 0 < delta < 0x80000000:
            metric['max_output_rate_mps2'] = max(metric['max_output_rate_mps2'],
                math.dist(velocity, previous[1]) / (delta * 1e-6))
    return row['call_us'], velocity


def analyze_fusion(capture, bridge, stationary=False, candidate=None):
    if candidate is not None:
        if not isinstance(candidate, dict) or not candidate or set(candidate) - set(FILTER_CONFIG_FIELDS[:4]):
            raise ValueError('candidate must contain only recorded filter parameter names')
        if any(isinstance(v, bool) or not isinstance(v, (float, int)) or not math.isfinite(v) or v <= 0
               for v in candidate.values()):
            raise ValueError('candidate filter parameters must be finite and positive')
    segments = continuous_attempt_segments(capture)
    replay = dict(attempts=0, scored_attempts=0, exact_initial_state_segments=0,
                  output_mismatches=0, rejected_or_fallback=0, invalid_outputs=0,
                  warmup_attempts=0, maximum_output_difference_mps=0.,
                  filter_deinitializations=0, filter_initializations=0)
    limitations = ['hardware_validation_not_performed', 'wheel_odometry_is_not_independent_velocity_truth',
                  'unknown_sensor_latency_not_identifiable', 'no_parameter_optimization_claim']
    report_segments, samples, predictions, candidate_values = [], [], [], []
    common_baseline, common_candidate = [], []
    response = {label: dict(samples=0, peak_speed_mps=0., max_output_rate_mps2=0.,
                           posterior_discrepancy_squared=0.)
                for label in (('baseline', 'candidate') if candidate else ('baseline',))}
    comparison = dict(attempts=0, rejected_or_fallback=0, invalid_outputs=0, paired_scored_outputs=0,
        maximum_output_change_mps=0., baseline_posterior_discrepancy_squared=0.,
        candidate_posterior_discrepancy_squared=0., filter_deinitializations=0,
        filter_initializations=0, parameter_changes=[]) if candidate else None
    if capture['schema'] not in (5, 6, 7, 8):
        limitations.append('legacy_inputs_not_replayable')
    for segment in segments:
        if not all(_config_valid(row) for row in segment):
            limitations.append('actual_filter_configuration_unavailable')
            continue
        exact = not bool(segment[0]['attempt_flags'] & 8)
        replay['exact_initial_state_segments'] += exact
        state, candidate_state = bridge.new_state(), bridge.new_state() if candidate else None
        was_initialized, candidate_was_initialized = False, False
        previous_output, previous_alternative = None, None
        if comparison is not None:
            for key in FILTER_CONFIG_FIELDS[:4]:
                if key in candidate and candidate[key] != segment[0][key]:
                    change = dict(parameter=key, old=segment[0][key], new=candidate[key])
                    if change not in comparison['parameter_changes']:
                        comparison['parameter_changes'].append(change)
            scales = [candidate.get(key, segment[0][key]) / segment[0][key] for key in FILTER_CONFIG_FIELDS[:3]]
            if abs(scales[0] - 1) > 1e-5 and max(scales) - min(scales) < max(scales) * 1e-5:
                limitations.append('joint_q_r_scaling_does_not_establish_improvement')
        # Unknown initial covariance cannot be reconstructed from a latest-snapshot CSV.
        # A 20-call settling exclusion permits descriptive metrics, never exact-state claims.
        warmup = 0 if exact else min(20, len(segment))
        report_segments.append(dict(first_line=segment[0]['line'], last_line=segment[-1]['line'],
            attempts=len(segment), exact_initial_state=exact, warmup_attempts=warmup,
            config={key: segment[0][key] for key in FILTER_CONFIG_FIELDS}))
        for index, row in enumerate(segment):
            output = bridge.update(state, row)
            alternative = bridge.update(candidate_state, row, candidate) if candidate else None
            replay['attempts'] += 1
            replay['rejected_or_fallback'] += not output['fused']
            replay['invalid_outputs'] += not output['valid']
            replay['filter_deinitializations'] += was_initialized and not output['initialized']
            replay['filter_initializations'] += not was_initialized and output['initialized']
            was_initialized = output['initialized']
            if comparison is not None:
                comparison['attempts'] += 1
                comparison['rejected_or_fallback'] += not alternative['fused']
                comparison['invalid_outputs'] += not alternative['valid']
                comparison['filter_deinitializations'] += candidate_was_initialized and not alternative['initialized']
                comparison['filter_initializations'] += not candidate_was_initialized and alternative['initialized']
                candidate_was_initialized = alternative['initialized']
            if index < warmup:
                replay['warmup_attempts'] += 1
                continue
            replay['scored_attempts'] += 1
            # Candidate response uses the same valid input/output population.
            if alternative is None or output['valid'] and alternative['valid']:
                previous_output = _response_sample(response['baseline'], output, row, previous_output)
                if alternative is not None:
                    previous_alternative = _response_sample(response['candidate'], alternative, row, previous_alternative)
            else:
                previous_output, previous_alternative = None, None
            if comparison is not None and output['valid'] and alternative['valid']:
                comparison['paired_scored_outputs'] += 1
                comparison['maximum_output_change_mps'] = max(comparison['maximum_output_change_mps'],
                    math.dist(output['velocity_n_mps'], alternative['velocity_n_mps']))
                wheel = [row['wheel_n_x_mps'], row['wheel_n_y_mps']]
                comparison['baseline_posterior_discrepancy_squared'] += math.dist(output['velocity_n_mps'], wheel)**2
                comparison['candidate_posterior_discrepancy_squared'] += math.dist(alternative['velocity_n_mps'], wheel)**2
            if exact:
                difference = max(abs(output['velocity_n_mps'][i] - row[key])
                    for i, key in enumerate(('output_n_x_mps', 'output_n_y_mps')))
                replay['maximum_output_difference_mps'] = max(replay['maximum_output_difference_mps'], difference)
                flags_match = output['valid'] == bool(row['attempt_flags'] & 16) and output['fused'] == bool(row['attempt_flags'] & 32)
                replay['output_mismatches'] += (not flags_match or output['sample_us'] != row['output_sample_us'] or
                    difference > max(2e-5, max(map(abs, output['velocity_n_mps'])) * 2e-5))
            if row['attempt_flags'] & 6 == 6:
                samples.append(row)
                if output['valid'] and (alternative is None or alternative['valid']):
                    predictions.append(output['velocity_n_mps'])
                    if alternative is not None:
                        candidate_values.append(alternative['velocity_n_mps'])
                if alternative is not None and output['fused'] and alternative['fused']:
                    common_baseline.append(output['velocity_n_mps'])
                    common_candidate.append(alternative['velocity_n_mps'])
    if not any(segment['attempts'] - segment['warmup_attempts'] >= 50 for segment in report_segments):
        limitations.append('insufficient_continuous_inputs')
    if any(not s['exact_initial_state'] for s in report_segments):
        limitations.append('unknown_initial_state_replay_is_warmed_up_not_exact')
    noise = dict(available=bool(stationary and len(samples) >= 50 and 'insufficient_continuous_inputs' not in limitations),
                 stationary_annotation=bool(stationary), samples=len(samples))
    if noise['available']:
        noise['output_comparison_population'] = 'Same input calls with both outputs valid, including wheel fallback.'
        noise['inputs'] = {key: _variance([r[key] for r in samples]) for key in
            ('wheel_n_x_mps', 'wheel_n_y_mps', 'accel_n_x_mps2', 'accel_n_y_mps2')}
        if len(predictions) >= 2:
            noise['baseline_output'] = {axis: _variance([p[i] for p in predictions]) for i, axis in enumerate(('x', 'y'))}
        if len(candidate_values) >= 2:
            noise['candidate_output'] = {axis: _variance([p[i] for p in candidate_values]) for i, axis in enumerate(('x', 'y'))}
        if candidate is not None:
            noise['common_fused_output'] = dict(samples=len(common_baseline), attempted_inputs=len(samples))
            if len(common_baseline) >= 2:
                noise['common_fused_output']['baseline'] = {axis: _variance([p[i] for p in common_baseline])
                    for i, axis in enumerate(('x', 'y'))}
                noise['common_fused_output']['candidate'] = {axis: _variance([p[i] for p in common_candidate])
                    for i, axis in enumerate(('x', 'y'))}
    if replay['output_mismatches']:
        limitations.append('production_c_does_not_match_recorded_output')
    if comparison is not None:
        for label in ('baseline', 'candidate'):
            squared = comparison.pop(label + '_posterior_discrepancy_squared')
            comparison[label + '_posterior_discrepancy_rms_mps'] = (
                math.sqrt(squared / comparison['paired_scored_outputs']) if comparison['paired_scored_outputs'] else None)
        comparison['metric_note'] = 'Posterior wheel discrepancy is not pre-correction innovation or independent speed error.'
    for metric in response.values():
        squared = metric.pop('posterior_discrepancy_squared')
        metric['posterior_discrepancy_rms_mps'] = math.sqrt(squared / metric['samples']) if metric['samples'] else None
    response['note'] = 'Response diagnostics from the same recorded input times; not independent speed accuracy.'
    return dict(kind='kalman', hardware_validated=False, parameter_change_validated=False,
                replay=replay, segments=report_segments, candidate_comparison=comparison, response=response,
                stationary_noise=noise, candidate=candidate, limitations=sorted(set(limitations)),
                source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in bridge.sources})
