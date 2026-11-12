from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from fixtures import row
from vofa_power import analyze_power, canonical_power_rows


def power_rows():
    rows = []
    for i in range(30):
        values = dict(valid=15 | 512, wheel_masks=255, cycle_age_ms=0, final_age_ms=0,
            meter_power_w=12, meter_age_ms=0, meter_sequence=i // 2,
            config_fingerprint=123, config_version=1, provenance=1)
        for wheel in ('lf', 'rf', 'lb', 'rb'):
            values.update({f'final_current_{wheel}': 1000, f'raw_current_{wheel}': 8000,
                           f'actual_rpm_{wheel}': 100, f'wheel_age_ms_{wheel}': 1})
        rows.append(row(4, i, i * 20, **values))
    return rows


class PowerTests(unittest.TestCase):
    def test_real_final_commands_meter_events_and_wheel_ages_are_preserved(self):
        capture = dict(schema=4, rows=power_rows())
        converted = canonical_power_rows(capture)
        self.assertEqual(converted[0]['command0'], 1000)
        self.assertEqual(converted[0]['motor_age_ms_0'], 1)
        report = analyze_power(capture)
        self.assertGreater(report['steady']['accepted_rows'], 0)
        self.assertGreater(report['steady']['rejected']['duplicate'], 0)
        self.assertIn('actual_power_configuration_unmatched', report['limitations'])
        self.assertFalse(report['parameter_change_validated'])

    def test_invalid_zero_meter_and_stale_wheel_are_not_fit_samples(self):
        rows = power_rows()
        rows[10]['valid'] &= ~8
        rows[10]['meter_power_w'] = 0
        rows[20]['wheel_age_ms_lf'] = 100
        report = analyze_power(dict(schema=4, rows=rows))
        self.assertEqual(report['steady']['rejected']['stale'], 1)
        self.assertEqual(report['steady']['rejected']['motor_health'], 1)

    def test_failed_final_submission_on_duplicate_meter_breaks_steady_window(self):
        rows = power_rows()
        baseline = analyze_power(dict(schema=4, rows=power_rows()))
        rows[15]['valid'] &= ~4
        report = analyze_power(dict(schema=4, rows=rows))
        self.assertEqual(report['steady']['rejected']['submission'], 1)
        self.assertLess(report['steady']['accepted_rows'], baseline['steady']['accepted_rows'])

    def test_unverified_fingerprint_label_is_not_configuration_evidence(self):
        with self.assertRaisesRegex(ValueError, 'configuration'):
            analyze_power(dict(schema=4, rows=power_rows()), configuration={
                'fingerprint': 123, 'config_version': 1, 'tuning_source': 1,
                'models': [dict(k0=1)] * 4})


if __name__ == '__main__':
    unittest.main()
