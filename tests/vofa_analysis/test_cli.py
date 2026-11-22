import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from fixtures import attempt, row, wire_rows
from test_csv import export, legacy_rows
from test_power import power_rows

ROOT = Path(__file__).resolve().parents[2]


class CliTests(unittest.TestCase):
    def test_original_exports_produce_quality_and_analysis_without_csv_conversion(self):
        for schema in (2, 4, 5, 6, 7, 8, 9, 10):
            with self.subTest(schema=schema), tempfile.TemporaryDirectory() as directory:
                d = Path(directory)
                capture = d / 'raw.csv'
                rows = legacy_rows() if schema == 2 else wire_rows(power_rows() if schema == 4 else
                    [row(schema, sequence=i, tick=20*i, valid=64|16384|32768|((1<<16) if schema == 10 else 0), control_cycles=4*i,
                         correction_cycles=i, submitted_correction_versions=i,
                         redundancy_mode=1 if schema == 10 else 0,
                         redundancy_reason=4 if schema == 10 else 0,
                         redundancy_excluded_wheel=2 if schema == 10 else -1,
                         redundancy_used_wheel_mask=11 if schema == 10 else 15,
                         redundancy_corrected=1 if schema == 10 else 0,
                         redundancy_applied=0,
                         redundancy_threshold_mps=.25 if schema == 10 else 0,
                         redundancy_dominance_ratio=1.75 if schema == 10 else 0,
                         redundancy_max_reference_error_mps=.3 if schema == 10 else 0,
                         redundancy_confirm_samples_required=2 if schema == 10 else 0)
                    for i in range(1, 61)] if schema in (9, 10) else
                    [attempt(i, initialized=i != 1, config_source=0, schema=schema) for i in range(1, 61)])
                export(capture, rows, time_at=0, renamed=True)
                command = [sys.executable, '-B', str(ROOT / 'scripts/analyze-vofa.py'), str(capture),
                           '--output-dir', str(d / 'report'), '--build-dir', str(d / 'build'), '--stationary']
                result = subprocess.run(command, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
                report = json.loads((d / 'report/report.json').read_text())
                self.assertEqual(report['capture']['schema'], schema)
                self.assertFalse(report['hardware_validated'])
                self.assertIn('quality', report)
                if schema in (9, 10):
                    self.assertEqual(report['analysis']['counter_increments']['correction_cycles'], 59)
                    self.assertTrue(report['analysis']['corrected_targets_submitted'])
                if schema == 10:
                    self.assertEqual(report['analysis']['wheel_redundancy']['corrected_rows'], 60)
                    self.assertEqual(report['analysis']['wheel_redundancy']['candidate_wheels'], {'2': 60})
                    self.assertEqual(report['analysis']['wheel_redundancy']['thresholds']['redundancy_threshold_mps'], .25)
                self.assertTrue((d / 'report/report.md').is_file())
                self.assertFalse((d / 'report/parameters-applied.json').exists())
                self.assertNotEqual(subprocess.run(command, text=True, capture_output=True).returncode, 0)

    def test_malformed_csv_reports_failure_without_parameter_output(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            capture = d / 'raw.csv'
            capture.write_text('I0,I1\n4,2\n')
            result = subprocess.run([sys.executable, '-B', str(ROOT/'scripts/analyze-vofa.py'), str(capture),
                '--output-dir', str(d/'report')], text=True, capture_output=True)
            self.assertEqual(result.returncode, 2)
            report = json.loads((d/'report/report.json').read_text())
            self.assertEqual(report['status'], 'rejected')

    def test_compact_capture_explicitly_rejects_filter_fitting(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            capture = d/'raw.csv'
            export(capture, wire_rows([row(9, sequence=i, tick=20*i) for i in range(1, 4)]))
            candidate = d/'candidate.json'
            candidate.write_text('{}')
            result = subprocess.run([sys.executable, '-B', str(ROOT/'scripts/analyze-vofa.py'), str(capture),
                '--output-dir', str(d/'report'), '--fusion-candidate', str(candidate)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 2)
            report = json.loads((d/'report/report.json').read_text())
            self.assertIn('compact schema 9/10 omits fitting/replay inputs', report['error'])
            self.assertFalse((d/'report/model.json').exists())


if __name__ == '__main__':
    unittest.main()
