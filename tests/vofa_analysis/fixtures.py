"""Synthetic wire rows; these fixtures are not vehicle validation evidence."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from vofa_csv import schemas


def row(schema, sequence=1, tick=20, **values):
    result = {field['name']: (-1 if field['type'] == 'age' else 0)
              for field in schemas()[schema]['channels']}
    result.update(schema=schema, tick_ms=tick, tx_sequence=sequence, **values)
    result.setdefault('line', sequence + 1)
    for name in list(result):
        if name.endswith('_hi'):
            base = name[:-3]
            if base in result:
                result[name], result[base + '_lo'] = result[base] >> 16, result[base] & 65535
    return result


def wire_rows(rows):
    return [[r[field['name']] for field in schemas()[r['schema']]['channels']] for r in rows]


def attempt(sequence, tick=None, initialized=True, resets=0, **changes):
    tick = sequence * 20 if tick is None else tick
    values = dict(valid=31, config_source=1, config_fingerprint=12345,
        attempt_sequence=sequence, attempt_flags=55 | (8 if initialized else 0),
        call_us=(tick * 1000 + 20000) & 0xffffffff, wheel_us=tick * 1000 & 0xffffffff,
        imu_us=tick * 1000 & 0xffffffff, output_sample_us=tick * 1000 & 0xffffffff,
        resets_before=resets, fusion_wheel_variance=.01, fusion_accel_variance=.25,
        fusion_jerk_density=1., fusion_max_residual_mps=1.5,
        fusion_max_age_us=60000, fusion_max_skew_us=0, fusion_max_gap_us=60000)
    values.update(changes)
    return row(5, sequence=sequence, tick=tick & 0xffffffff, **values)
