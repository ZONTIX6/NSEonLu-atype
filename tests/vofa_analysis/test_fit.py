"""Whole synthetic trials exercise normalization, recorded baselines and C fitting."""
import csv
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
import unittest

from fixtures import row, wire_rows
from test_csv import export

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from vofa_config import build_power_manifest
from vofa_power import fit_power_trials


def write_trials(directory, config):
    trials = []
    for index, split in enumerate(('train', 'calibration', 'test')):
        rng = random.Random(417 + index)
        states = [([0] * 4, [0] * 4), ([10000] * 4, [6000] * 4)]
        for _ in range(24):
            signs = [rng.choice((-1, 1)) for _ in range(4)]
            states.append(([s * rng.randint(1000, 8500) for s in signs],
                           [s * rng.randint(200, 5000) for s in signs]))
        rows = []
        for commands, rpm in states:
            power = 5 + sum(.08 * (u/1000)**2 + .02 * (u/1000)*(n*math.pi/30) +
                .003 * abs(n*math.pi/30) + .00008*(n*math.pi/30)**2 for u, n in zip(commands, rpm))
            for _ in range(16):
                sequence = len(rows) + 1
                values = dict(valid=15 | 512, wheel_masks=255, cycle_age_ms=0, final_age_ms=0,
                    meter_age_ms=0, meter_power_w=power, meter_sequence=sequence,
                    config_fingerprint=config['fingerprint'], config_version=config['config_version'],
                    provenance=config['tuning_source'])
                for w, u, n in zip(('lf', 'rf', 'lb', 'rb'), commands, rpm):
                    values.update({f'final_current_{w}': u, f'raw_current_{w}': u + 100,
                                   f'actual_rpm_{w}': n, f'wheel_age_ms_{w}': 1})
                rows.append(row(4, sequence, sequence*20, **values))
        path = directory / (split + '.csv')
        export(path, wire_rows(rows), time_at=0, renamed=True)
        trials.append(dict(id=split, session='synthetic-' + split, split=split, synthetic=True, path=str(path)))
    return trials


class FitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.configuration = build_power_manifest()

    def test_raw_vofa_trials_fit_with_actual_four_model_baseline(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            trials = write_trials(d, self.configuration)
            report, model = fit_power_trials(trials, self.configuration, d/'build')
            self.assertTrue(report['validation_passed'], report['validation_failures'])
            for actual, expected in zip(report['production_c']['baseline_coefficients'], self.configuration['models']):
                self.assertEqual({k: struct.pack('<f', v) for k, v in actual.items()},
                                 {k: struct.pack('<f', v) for k, v in expected.items()})
            for actual, expected in zip(model['coefficients'], [5, .08, .02, .003, .00008]):
                self.assertAlmostEqual(actual, expected, places=6)
            self.assertIn('synthetic_data', model['release_blockers'])

    def test_cli_fits_independent_trials_and_writes_analysis_candidate(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            trials = write_trials(d, self.configuration)
            (d/'trials.json').write_text(json.dumps(dict(trials=trials, instrument_error_w=.1)))
            (d/'configuration.json').write_text(json.dumps(self.configuration))
            result = subprocess.run([sys.executable, '-B', str(ROOT/'scripts/analyze-vofa.py'), trials[0]['path'],
                '--configuration', str(d/'configuration.json'), '--power-trials', str(d/'trials.json'),
                '--output-dir', str(d/'result'), '--build-dir', str(d/'build')], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            model = json.loads((d/'result/model.json').read_text())
            self.assertFalse(model['hardware_release'])
            self.assertTrue(model['validation_passed'])

    def test_trial_leakage_and_mismatched_actual_configuration_stop(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            trials = write_trials(d, self.configuration)
            trials[1]['session'] = trials[0]['session']
            with self.assertRaisesRegex(ValueError, 'leakage'):
                fit_power_trials(trials, self.configuration, d/'build')
            trials[1]['session'] = 'synthetic-calibration'
            other = build_power_manifest(config_version=1)
            with self.assertRaisesRegex(ValueError, 'configuration'):
                fit_power_trials(trials, other, d/'build')

    def test_renamed_reexport_of_same_measurements_is_not_an_independent_trial(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            trials = write_trials(d, self.configuration)
            original = Path(trials[0]['path']).read_text(encoding='utf-8-sig')
            for index in (1, 2):
                Path(trials[index]['path']).write_text(original.replace('renamed_', f'reexport_{index}_'), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'reused|overlap'):
                fit_power_trials(trials, self.configuration, d/'build')


if __name__ == '__main__':
    unittest.main()
