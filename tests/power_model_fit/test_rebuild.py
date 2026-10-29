"""Behavioral tests for the offline static model reconstruction contract."""
import csv
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts'))
from fixtures import write_example


class RebuildTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.directory = Path(self.temp.name)
        self.manifest = write_example(self.directory/'input')

    def tearDown(self):
        self.temp.cleanup()

    def run_tool(self, output='result', *extra):
        return subprocess.run([sys.executable, str(ROOT/'scripts/rebuild-power-model.py'),
            '--manifest', str(self.manifest), '--output-dir', str(self.directory/output),
            '--build-dir', str(self.directory/'build'), *extra],
            capture_output=True, text=True)

    def edit_manifest(self, change):
        data = json.loads(self.manifest.read_text())
        change(data)
        self.manifest.write_text(json.dumps(data), encoding='utf-8')

    def edit_rows(self, split, change):
        path = self.manifest.parent/f'{split}.csv'
        with path.open(newline='') as stream:
            reader = csv.DictReader(stream)
            fields, rows = reader.fieldnames, list(reader)
        change(rows)
        with path.open('w', newline='') as stream:
            writer = csv.DictWriter(stream, fields)
            writer.writeheader()
            writer.writerows(rows)

    def report(self, output='result'):
        return json.loads((self.directory/output/'report.json').read_text())

    def test_recovers_known_parameters_and_checks_production_c(self):
        run = self.run_tool('result', '--emit-c')
        self.assertEqual(run.returncode, 0, run.stderr + run.stdout)
        model = json.loads((self.directory/'result/model.json').read_text())
        for actual, expected in zip(model['coefficients'], [5, .08, .02, .003, .00008]):
            self.assertAlmostEqual(actual, expected, delta=1e-7)
        self.assertFalse(model['hardware_release'])
        report = self.report()
        self.assertLess(report['metrics']['test']['candidate']['rmse_w'], 1e-4)
        self.assertGreater(report['production_c']['limiter_checks'], 100)
        self.assertGreater(report['production_c']['infeasible_budgets'], 0)
        self.assertIn('synthetic_data', report['release_blockers'])
        self.assertIn('ANALYSIS ONLY', (self.directory/'result/candidate.inc').read_text())

    def test_wrong_units_fail_without_a_candidate(self):
        self.edit_manifest(lambda m: m['units'].update(command='A'))
        run = self.run_tool()
        self.assertNotEqual(run.returncode, 0)
        self.assertIn('units', self.report()['error'])
        self.assertFalse((self.directory/'result/model.json').exists())

    def test_non_object_manifest_produces_a_rejection_report(self):
        self.manifest.write_text('[]', encoding='utf-8')
        run = self.run_tool()
        self.assertEqual(run.returncode, 2)
        self.assertEqual(self.report()['status'], 'rejected')
        self.assertFalse((self.directory/'result/model.json').exists())

    def test_group_leakage_and_repeated_content_are_rejected(self):
        self.edit_manifest(lambda m: m['experiments'][1].update(session='synthetic-train'))
        self.assertNotEqual(self.run_tool().returncode, 0)
        self.assertIn('session', self.report()['error'])
        self.edit_manifest(lambda m: m['experiments'][1].update(session='other', path='train.csv'))
        self.assertNotEqual(self.run_tool('result2').returncode, 0)
        self.assertIn('reused', self.report('result2')['error'])

    def test_rank_deficient_training_does_not_export_coefficients(self):
        def constant(rows):
            for row in rows:
                for w in range(4):
                    row[f'command{w}'], row[f'rpm{w}'] = '3000', '1000'
        self.edit_rows('train', constant)
        self.assertNotEqual(self.run_tool().returncode, 0)
        self.assertIn('rank', self.report()['error'])
        self.assertGreater(self.report()['data_quality']['train']['accepted_rows'], 0)
        self.assertFalse((self.directory/'result/model.json').exists())

    def test_rejects_numeric_braking_stale_and_failed_submissions(self):
        def corrupt(rows):
            rows[80]['load_w'] = 'nan'
            rows[100]['command0'] = str(-int(rows[100]['command0']))
            rows[120]['meter_age_ms'] = '100'
            rows[140]['submitted'] = '0'
            rows[160]['online_mask'] = '7'
        self.edit_rows('train', corrupt)
        run = self.run_tool()
        self.assertEqual(run.returncode, 0, run.stderr)
        counts = self.report()['data_quality']['train']['rejected']
        for reason in ('numeric', 'braking', 'stale', 'submission', 'motor_health'):
            self.assertGreater(counts[reason], 0)

    def test_duplicate_and_backward_events_do_not_rewind_high_water(self):
        def events(rows):
            rows[80]['meter_sequence'] = rows[79]['meter_sequence']
            rows[100]['meter_sequence'] = '1'
            rows[101]['meter_sequence'] = '2'
        self.edit_rows('train', events)
        self.assertEqual(self.run_tool().returncode, 0)
        counts = self.report()['data_quality']['train']['rejected']
        self.assertEqual(counts['duplicate'], 1)
        self.assertEqual(counts['order'], 2)

    def test_uint32_wrap_keeps_valid_steady_windows(self):
        def wrap(rows):
            for row in rows:
                row['tick_ms'] = str((int(row['tick_ms']) + 0xffffff00) & 0xffffffff)
                row['meter_sequence'] = str((int(row['meter_sequence']) + 0xffffff00) & 0xffffffff)
        self.edit_rows('train', wrap)
        run = self.run_tool()
        self.assertEqual(run.returncode, 0, run.stderr)
        self.assertEqual(self.report()['data_quality']['train']['rejected'].get('order', 0), 0)

    def test_failed_test_set_is_not_used_to_refit_or_raise_margin(self):
        self.assertEqual(self.run_tool('clean').returncode, 0)
        clean = json.loads((self.directory/'clean/model.json').read_text())
        self.edit_rows('test', lambda rows: [r.update(load_w=float(r['load_w'])+100) for r in rows])
        run = self.run_tool('bad', '--emit-c')
        self.assertEqual(run.returncode, 2, run.stderr)
        bad = json.loads((self.directory/'bad/model.json').read_text())
        self.assertEqual(clean['coefficients'], bad['coefficients'])
        self.assertEqual(clean['empirical_margin_w'], bad['empirical_margin_w'])
        self.assertFalse(self.report('bad')['validation_passed'])
        self.assertFalse((self.directory/'bad/candidate.inc').exists())

    def test_unknown_boundary_and_existing_output_cannot_publish(self):
        self.edit_manifest(lambda m: m.update(boundary='unknown'))
        run = self.run_tool('unknown', '--emit-c')
        self.assertEqual(run.returncode, 2, run.stderr)
        self.assertFalse((self.directory/'unknown/candidate.inc').exists())
        original = (self.directory/'unknown/report.json').read_bytes()
        self.assertNotEqual(self.run_tool('unknown').returncode, 0)
        self.assertEqual(original, (self.directory/'unknown/report.json').read_bytes())

    def test_bounded_measurement_noise_does_not_become_a_release_claim(self):
        import random
        for index, split in enumerate(('train', 'calibration', 'test')):
            rng = random.Random(83+index)
            def noise(rows):
                for row in rows:
                    row['load_w'] = float(row['load_w']) + rng.uniform(-.05, .05)
            self.edit_rows(split, noise)
        run = self.run_tool()
        self.assertEqual(run.returncode, 0, run.stderr)
        report = self.report()
        self.assertLess(report['metrics']['test']['candidate']['rmse_w'], .06)
        self.assertEqual(report['metrics']['test']['candidate']['maximum_upper_bound_violation_w'], 0)
        self.assertFalse(report['hardware_release'])


if __name__ == '__main__':
    unittest.main()
