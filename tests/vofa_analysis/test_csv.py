"""Raw VOFA export ingestion: no user column mapping or unit conversion."""
import csv
import math
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from vofa_csv import read_capture, quality_report
from fixtures import row, attempt, wire_rows


def legacy_rows(schema=2, count=6):
    rows = []
    for i in range(count):
        row = [0] * (72 if schema == 3 else 64)
        row[0], row[2], row[4], row[5], row[6] = schema, i * 20, i, 3, 2
        row[8], row[13], row[21], row[23], row[59] = 15, 120, 1500, 1000, 1234
        if schema == 3:
            row[64] = 2
        rows.append(row)
    return rows


def export(path, rows, time_at=None, renamed=False):
    headers = [('renamed_' if renamed else 'I') + str(i) for i in range(len(rows[0]))]
    rows = [list(row) for row in rows]
    if time_at is not None:
        headers.insert(time_at, 'Time')
        for i, row in enumerate(rows):
            row.insert(time_at, f'2026-11-13 12:00:00.{i * 20:03d}')
    with Path(path).open('w', newline='', encoding='utf-8-sig') as stream:
        writer = csv.writer(stream)
        writer.writerow(headers)
        writer.writerows(rows)


class CsvTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.path = Path(self.tmp.name) / 'original.csv'

    def tearDown(self):
        self.tmp.cleanup()

    def test_legacy_default_and_renamed_headers_with_time_column(self):
        for schema in (2, 3):
            for time_at in (None, 0, 17, 72 if schema == 3 else 64):
                for renamed in (False, True):
                    with self.subTest(schema=schema, time_at=time_at, renamed=renamed):
                        export(self.path, legacy_rows(schema), time_at, renamed)
                        capture = read_capture(self.path)
                        self.assertEqual(capture['schema'], schema)
                        row = capture['rows'][0]
                        self.assertEqual(row['wheel_vx_mps'], 1.5)
                        self.assertAlmostEqual(row['wheel_wz_radps'], 2 * math.pi)
                        self.assertEqual(row['meter_power_w'], 12.34)
                        self.assertEqual(capture['time_column'], time_at)
                        self.assertIn('legacy_inputs_not_replayable', quality_report(capture)['limitations'])

    def test_missing_columns_and_mixed_schema_stop(self):
        rows = legacy_rows()
        export(self.path, [row[:-1] for row in rows])
        with self.assertRaisesRegex(ValueError, 'layout'):
            read_capture(self.path)
        rows[3][0] = 3
        export(self.path, rows)
        with self.assertRaisesRegex(ValueError, 'schema'):
            read_capture(self.path)

    def test_bad_numeric_rows_remain_visible_as_quality_breaks(self):
        rows = legacy_rows()
        rows[2][13] = 'nan'
        export(self.path, rows)
        capture = read_capture(self.path)
        self.assertEqual(capture['rows'][2]['error'], 'non-finite numeric value')
        quality = quality_report(capture)
        self.assertEqual(quality['invalid_rows'], 1)
        self.assertEqual(quality['valid_rows'], 5)

    def test_rollover_duplicate_gap_and_reboot_are_reported(self):
        rows = legacy_rows(count=6)
        ticks = [0xfffffff0, 4, 24, 24, 84, 0]
        seq = [0xfffffffe, 0xffffffff, 0, 0, 3, 0]
        for row, tick, index in zip(rows, ticks, seq):
            row[1:5] = [tick >> 16, tick & 65535, index >> 16, index & 65535]
        export(self.path, rows)
        quality = quality_report(read_capture(self.path))
        self.assertEqual(quality['duplicate_frames'], 1)
        self.assertEqual(quality['missing_attempts'], 2)
        self.assertEqual(quality['restart_or_order_boundaries'], 1)
        self.assertEqual(quality['clock_wraps'], 1)

    def test_misordered_default_headers_do_not_silently_relabel(self):
        export(self.path, legacy_rows())
        data = self.path.read_text(encoding='utf-8-sig')
        data = data.replace('I13,I14', 'I14,I13')
        self.path.write_text(data, encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'layout'):
            read_capture(self.path)

    def test_conflicting_duplicate_frame_is_invalid_not_new_evidence(self):
        rows = legacy_rows()
        rows[2] = list(rows[1])
        rows[2][13] = 9000
        export(self.path, rows)
        capture = read_capture(self.path)
        self.assertIn('duplicate frame', capture['rows'][2]['error'])

    def test_new_profiles_preserve_si_floats_and_split_clock(self):
        for schema, first in ((4, row(4, tick=0xfedcba98, meter_power_w=12.345)),
                              (5, attempt(1, tick=0xfedcba98, wheel_n_x_mps=1.2345))):
            with self.subTest(schema=schema):
                export(self.path, wire_rows([first]), time_at=0, renamed=True)
                capture = read_capture(self.path)
                self.assertEqual(capture['schema'], schema)
                self.assertEqual(capture['rows'][0]['tick_ms'], 0xfedcba98)
                key, value = ('meter_power_w', 12.345) if schema == 4 else ('wheel_n_x_mps', 1.2345)
                self.assertEqual(capture['rows'][0][key], value)

    def test_renamed_numeric_extra_column_cannot_ambiguously_shift_layout(self):
        rows = legacy_rows(count=2)
        for r in rows:
            r.insert(0, 2)
        export(self.path, rows, renamed=True)
        with self.assertRaisesRegex(ValueError, 'ambiguous'):
            read_capture(self.path)

    def test_unknown_extra_numeric_channel_is_not_discarded_as_a_time_column(self):
        rows = wire_rows([row(4, sequence=i, tick=i * 20, valid=15) for i in range(1, 5)])
        for r in rows:
            r.append(987.654)
        export(self.path, rows)
        with self.assertRaisesRegex(ValueError, 'layout'):
            read_capture(self.path)

    def test_missing_last_channel_cannot_be_filled_by_the_export_time_column(self):
        rows = wire_rows([row(4, sequence=i, tick=i * 20, valid=15) for i in range(1, 5)])
        export(self.path, [r[:-1] for r in rows], time_at=95)
        content = self.path.read_text(encoding='utf-8-sig')
        for i in range(4):
            content = content.replace(f'2026-11-13 12:00:00.{i * 20:03d}', '12345')
        self.path.write_text(content, encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'layout'):
            read_capture(self.path)

    def test_final_integer_commands_tolerate_only_the_wire_delimiter_bit_change(self):
        original = row(4, final_current_lf=1000.0000610351562)
        export(self.path, wire_rows([original]))
        self.assertEqual(read_capture(self.path)['rows'][0]['final_current_lf'], 1000)


if __name__ == '__main__':
    unittest.main()
