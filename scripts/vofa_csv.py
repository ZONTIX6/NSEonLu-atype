"""Strict positional VOFA CSV decoding and capture-quality accounting.

The engineering-owned schema supplies units and channel order. Display names
may change; missing channels and ambiguous layouts never receive a guessed map.
"""
from collections import Counter
import csv
import hashlib
import json
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
UINT32 = 0xffffffff
HALF = 0x80000000


def schemas():
    result = json.loads((ROOT / 'config/32_capture/vofa_schema.json').read_text(encoding='utf-8'))
    names = ['schema', 'tick_ms_hi', 'tick_ms_lo', 'tx_sequence_hi', 'tx_sequence_lo',
             'valid', 'observer_status', 'chassis_mode', 'wheel_online_mask', 'enabled_mask',
             'observer_age_ms', 'velocity_age_ms', 'wheel_age_ms']
    names += ['raw_rpm_' + w for w in ('lf', 'rf', 'lb', 'rb')]
    names += ['wheel_age_ms_' + w for w in ('lf', 'rf', 'lb', 'rb')]
    names += ['wheel_vx_mps', 'wheel_vy_mps', 'wheel_wz_radps',
              'target_vx_mps', 'target_vy_mps', 'target_wz_radps',
              'velocity_vx_mps', 'velocity_vy_mps', 'paired_body_x_mps', 'paired_body_y_mps',
              'accel_n_x_mps2', 'accel_n_y_mps2', 'imu_age_ms', 'imu_sequence_hi', 'imu_sequence_lo',
              'imu_qw', 'imu_qx', 'imu_qy', 'imu_qz', 'imu_ax_mps2', 'imu_ay_mps2', 'imu_az_mps2',
              'imu_gx_radps', 'imu_gy_radps', 'imu_gz_radps',
              'yaw_ecd', 'pitch_ecd', 'yaw_rpm', 'pitch_rpm', 'yaw_age_ms', 'pitch_age_ms']
    names += ['final_current_' + w for w in ('lf', 'rf', 'lb', 'rb')]
    names += ['model_unlimited_w', 'model_final_w', 'effective_limit_w', 'meter_power_w',
              'cycle_age_ms', 'target_age_ms', 'meter_age_ms', 'record_role']
    extra = ['imu_source', 'imu_receive_age_ms', 'imu_source_age_ms', 'imu_tick_ms_hi',
             'imu_tick_ms_lo', 'imu_receive_span_ms', 'imu_rejected_hi', 'imu_rejected_lo']
    for schema, columns in ((2, names), (3, names + extra)):
        result[str(schema)] = dict(name='legacy', channels=[dict(name=n,
            type='u16' if n.endswith(('_hi', '_lo')) else 'legacy', unit='legacy') for n in columns])
    return {int(k): v for k, v in result.items()}


def numeric(value):
    number = float(value)
    if not math.isfinite(number) or abs(number) > 3.4028234663852886e38:
        raise ValueError('non-finite numeric value')
    return number


def integer(value, minimum=0, maximum=65535):
    value = numeric(value)
    rounded = round(value)
    # The wire encoder may flip one float32 mantissa bit to avoid its delimiter.
    ulp = 2.0 ** (math.floor(math.log2(abs(value))) - 23) if value else 0
    if abs(value - rounded) > min(.249, max(1e-6, ulp * 1.1)):
        raise ValueError('non-integral metadata')
    if not minimum <= rounded <= maximum:
        raise ValueError('integer outside declared range')
    return rounded


def _decode(values, schema, definition):
    row = {}
    for spec, value in zip(definition['channels'], values):
        kind, name = spec['type'], spec['name']
        if kind == 'i16' or name.startswith('final_current_'):
            row[name] = integer(value, -32768, 32767)
        elif kind == 'f32':
            row[name] = numeric(value)
        elif kind == 'age':
            row[name] = integer(value, -1, 0x7fffffff)
        elif kind in ('enum', 'legacy') or name == 'observer_status':
            row[name] = integer(value, -0x80000000, 0x7fffffff)
        else:
            row[name] = integer(value)
    for name in list(row):
        if name.endswith('_hi'):
            base = name[:-3]
            if base + '_lo' in row:
                row[base] = row[name] * 65536 + row[base + '_lo']
    if schema in (2, 3):
        for name in list(row):
            if name.endswith(('_mps', '_mps2', '_radps')):
                row[name] *= 2 * math.pi / 1000 if name.endswith('wz_radps') else .001
            elif name.startswith('imu_q'):
                row[name] *= .000001
            elif name.endswith('_w'):
                row[name] *= .01
    return row


def _header_matches(headers):
    for index, name in enumerate(headers):
        match = re.fullmatch(r'I(\d+)', name.strip(), flags=re.IGNORECASE)
        if match and int(match.group(1)) != index:
            return False
    return len(set(headers)) == len(headers)


def _time_columns(header, data):
    columns = []
    for index, title in enumerate(header):
        name = title.strip().lower().replace(' ', '')
        if re.fullmatch(r'(?:time|timestamp|时间|时间戳)(?:\((?:ms|s)\)|_(?:ms|s))?', name):
            columns.append(index)
            continue
        if re.fullmatch(r'i\d+', name):
            continue
        # An unknown numeric column is never discarded merely because a shifted
        # schema happens to fit. Text timestamps provide independent evidence.
        values = [r[index].strip() for r in data[:64] if len(r) == len(header)]
        if values and all(re.fullmatch(r'(?:\d{4}[-/]\d{1,2}[-/]\d{1,2}[ T])?\d{1,2}:\d{2}:\d{2}(?:\.\d+)?', v)
                          for v in values):
            columns.append(index)
    return columns


def read_capture(path):
    path = Path(path).resolve()
    raw = path.read_bytes()
    with path.open(encoding='utf-8-sig', newline='') as stream:
        table = list(csv.reader(stream))
    if len(table) < 2 or not table[0]:
        raise ValueError('CSV requires a header and data rows')
    header, data = table[0], table[1:]
    definitions, candidates = schemas(), []
    time_columns = _time_columns(header, data)
    for schema, definition in definitions.items():
        count = len(definition['channels'])
        if len(header) not in (count, count + 1):
            continue
        if len(header) == count and time_columns:
            continue
        removed = [None] if len(header) == count else time_columns
        for time_column in removed:
            indexes = [i for i in range(len(header)) if i != time_column]
            if not _header_matches([header[i] for i in indexes]):
                continue
            evidence = 0
            for raw_row in data[:64]:
                if len(raw_row) != len(header):
                    continue
                try:
                    if integer(raw_row[indexes[0]]) != schema:
                        continue
                    for i in range(1, 6):
                        integer(raw_row[indexes[i]], maximum=0xfffff if schema < 4 and i == 5 else 65535)
                    _decode([raw_row[i] for i in indexes], schema, definition)
                    evidence += 1
                except (ValueError, OverflowError):
                    continue
            if evidence:
                candidates.append((schema, definition, time_column, indexes))
    if len(candidates) != 1:
        raise ValueError(f'CSV layout is unsupported or ambiguous ({len(candidates)} candidates); preserve all channels')
    schema, definition, time_column, indexes = candidates[0]
    rows, previous_row = [], None
    for line, raw_row in enumerate(data, 2):
        try:
            if len(raw_row) != len(header):
                raise ValueError('CSV row width does not match header')
            value = numeric(raw_row[indexes[0]])
            if math.isfinite(value) and integer(value) != schema:
                raise RuntimeError(f'line {line}: mixed or unknown schema')
            row = _decode([raw_row[i] for i in indexes], schema, definition)
            if (previous_row is not None and row['tx_sequence'] == previous_row['tx_sequence'] and
                    row['tick_ms'] == previous_row['tick_ms'] and row != previous_row):
                raise ValueError('duplicate frame has inconsistent content')
            previous_row = dict(row)
            row['line'] = line
        except RuntimeError as error:
            raise ValueError(str(error)) from error
        except (ValueError, OverflowError) as error:
            row = dict(line=line, error=str(error))
        rows.append(row)
    return dict(path=str(path), sha256=hashlib.sha256(raw).hexdigest(), schema=schema,
                profile=definition['name'], channel_count=len(indexes), time_column=time_column,
                headers=header, rows=rows)


def quality_report(capture):
    counts = Counter(total_rows=len(capture['rows']))
    errors, bit_counts, ages, intervals, ranges = [], Counter(), {}, [], []
    previous, first, previous_attempt = None, None, None
    for row in capture['rows']:
        if 'error' in row:
            counts['invalid_rows'] += 1
            errors.append(dict(line=row['line'], reason=row['error']))
            if first is not None:
                ranges.append(dict(first_line=first, last_line=previous['line']))
            previous, first, previous_attempt = None, None, None
            continue
        counts['valid_rows'] += 1
        boundary = False
        if previous is not None:
            dt = (row['tick_ms'] - previous['tick_ms']) & UINT32
            ds = (row['tx_sequence'] - previous['tx_sequence']) & UINT32
            if dt >= HALF or ds >= HALF or (ds == 0 and dt != 0):
                counts['restart_or_order_boundaries'] += 1
                boundary = True
                previous_attempt = None
            elif ds == 0:
                counts['duplicate_frames'] += 1
            else:
                counts['missing_attempts'] += max(0, ds - 1)
                counts['clock_wraps'] += row['tick_ms'] < previous['tick_ms']
                intervals.append(dt)
                boundary = ds != 1
        if boundary and first is not None:
            ranges.append(dict(first_line=first, last_line=previous['line']))
            first = None
        if first is None:
            first = row['line']
        previous = row
        if capture['schema'] == 5:
            fifo = bool(row['valid'] & 64)
            counts['capture_fifo_rows'] += fifo
            counts['capture_overflow_rows'] += fifo and bool(row['valid'] & 128)
            if row['valid'] & 16 and row['attempt_flags'] & 1:
                advance = ((row['attempt_sequence'] - previous_attempt) & UINT32
                           if previous_attempt is not None else None)
                if advance == 0:
                    counts['duplicate_filter_calls'] += 1
                else:
                    counts['unique_filter_calls'] += 1
                    if advance is not None and advance < HALF:
                        counts['missing_filter_calls'] += advance - 1
                    elif advance is not None:
                        counts['filter_restart_or_order_boundaries'] += 1
                previous_attempt = row['attempt_sequence']
        for bit in range(20):
            bit_counts[str(bit)] += bool(row['valid'] & (1 << bit))
        for name, value in row.items():
            if 'age_ms' in name:
                metric = ages.setdefault(name, dict(known=0, unknown=0, max_ms=None))
                if value < 0:
                    metric['unknown'] += 1
                else:
                    metric['known'] += 1
                    metric['max_ms'] = value if metric['max_ms'] is None else max(value, metric['max_ms'])
    if first is not None:
        ranges.append(dict(first_line=first, last_line=previous['line']))
    for key in ('valid_rows', 'invalid_rows', 'duplicate_frames', 'missing_attempts',
                'restart_or_order_boundaries', 'clock_wraps', 'capture_fifo_rows',
                'capture_overflow_rows', 'unique_filter_calls', 'duplicate_filter_calls',
                'missing_filter_calls', 'filter_restart_or_order_boundaries'):
        counts.setdefault(key, 0)
    fifo_rows = counts['capture_fifo_rows']
    capture_mode = ('event_fifo' if fifo_rows and fifo_rows == counts['valid_rows'] else
                    'mixed' if fifo_rows else 'latest_snapshot')
    sequence_kind = {'event_fifo': 'capture_records', 'latest_snapshot': 'send_attempts',
                     'mixed': 'mixed_capture_records_and_send_attempts'}[capture_mode]
    limitations = ['restart_and_out_of_order_cannot_always_be_distinguished',
                   'hardware_validation_not_performed']
    if capture_mode != 'event_fifo':
        limitations.append('50_hz_latest_snapshot_is_not_every_control_or_filter_update')
    if capture_mode == 'mixed':
        limitations.append('mixed_capture_modes')
    if counts['capture_overflow_rows']:
        limitations.append('capture_fifo_overflow_observed_exact_drop_count_not_recorded')
    if capture['schema'] < 4:
        limitations += ['legacy_inputs_not_replayable', 'legacy_actual_configuration_unavailable']
    return dict(counts, schema=capture['schema'], profile=capture['profile'], errors=errors,
                capture_mode=capture_mode, sequence_kind=sequence_kind,
                capture_overflow_observed=bool(counts['capture_overflow_rows']),
                valid_bit_counts=dict(bit_counts), ages=ages, contiguous_frame_ranges=ranges,
                interval_ms=dict(min=min(intervals), max=max(intervals)) if intervals else None,
                limitations=limitations)


FILTER_CONFIG_FIELDS = ('fusion_wheel_variance', 'fusion_accel_variance', 'fusion_jerk_density',
    'fusion_max_residual_mps', 'fusion_max_age_us', 'fusion_max_skew_us', 'fusion_max_gap_us')
STATIC_FILTER_CONFIG_FIELDS = tuple(key for key in FILTER_CONFIG_FIELDS if key != 'fusion_accel_variance')
ATTEMPT_FIELDS = ('attempt_sequence', 'attempt_flags', 'call_receiver_ms', 'call_us',
    'wheel_us', 'imu_us', 'middle_receiver_ms', 'wheel_n_x_mps', 'wheel_n_y_mps',
    'accel_n_x_mps2', 'accel_n_y_mps2', 'output_n_x_mps', 'output_n_y_mps',
    'output_sample_us', 'resets_before', 'config_fingerprint', 'config_source') + FILTER_CONFIG_FIELDS


def continuous_attempt_segments(capture):
    """Keep only actual calls, preserving rejected calls and breaking at lost calls.

Current pipeline status/reset counters may change while the last attempt is
held. Only the reset count captured BEFORE that call describes its filter state.
The acceleration variance is an effective per-attempt input and may vary while
the static config fingerprint stays unchanged.
"""
    if capture['schema'] != 5:
        return []
    segments, segment, previous, previous_frame = [], [], None, None
    for row in capture['rows']:
        if 'error' in row or not row.get('attempt_flags', 0) & 1 or not row.get('valid', 0) & 16:
            if segment:
                segments.append(segment)
            segment, previous, previous_frame = [], None, None
            continue
        boundary = previous_frame is not None and (
            (row['tick_ms'] - previous_frame['tick_ms']) & UINT32) >= HALF
        previous_frame = row
        if previous is not None:
            advance = (row['attempt_sequence'] - previous['attempt_sequence']) & UINT32
            if advance == 0 and not boundary:
                if any(row.get(key) != previous.get(key) for key in ATTEMPT_FIELDS):
                    raise ValueError(f"line {row['line']}: duplicate attempt has inconsistent captured input/configuration")
                continue
            boundary |= advance != 1 or row['resets_before'] != previous['resets_before']
            boundary |= any(row.get(key) != previous.get(key)
                for key in ('config_fingerprint', 'config_source') + STATIC_FILTER_CONFIG_FIELDS)
            boundary |= ((row['call_us'] - previous['call_us']) & UINT32) >= HALF
        if boundary and segment:
            segments.append(segment)
            segment = []
        segment.append(row)
        previous = row
    if segment:
        segments.append(segment)
    return segments
