import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from fixtures import attempt, wire_rows
from test_csv import export, legacy_rows
from test_power import power_rows

ROOT = Path(__file__).resolve().parents[2]


class CliTests(unittest.TestCase):
    def test_original_exports_produce_quality_and_analysis_without_csv_conversion(self):
        for schema in (2, 4, 5, 6, 7):
            with self.subTest(schema=schema), tempfile.TemporaryDirectory() as directory:
                d = Path(directory)
                capture = d / 'raw.csv'
                rows = legacy_rows() if schema == 2 else wire_rows(power_rows() if schema == 4 else
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


if __name__ == '__main__':
    unittest.main()
