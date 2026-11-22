"""Raw VOFA export ingestion: no user column mapping or unit conversion."""
import csv
import math
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from vofa_csv import read_capture, quality_report, control_feedback_summary
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

    def test_compact_control_evidence_and_fifo_quality(self):
        rows = [row(9, sequence=i, tick=1000+20*i, valid=64|16384|32768,
                    control_cycles=65535+4*i, feedback_cycles=100+i,
                    correction_cycles=10+i, submitted_correction_versions=0x12345678+i,
                    final_current_lf=-32768, final_current_rf=32767)
                for i in range(3)]
        rows[-1]['valid'] |= 128
        export(self.path, wire_rows(rows), time_at=0, renamed=True)
        capture = read_capture(self.path)
        self.assertEqual(capture['schema'], 9)
        self.assertEqual(capture['channel_count'], 64)
        self.assertEqual(capture['rows'][0]['submitted_correction_versions'], 0x12345678)
        self.assertEqual(capture['rows'][0]['final_current_lf'], -32768)
        quality = quality_report(capture)
        self.assertEqual(quality['capture_mode'], 'event_fifo')
        self.assertTrue(quality['capture_overflow_observed'])
        self.assertIsNone(quality['unique_filter_calls'])
        self.assertIsNone(quality['missing_filter_calls'])
        summary = control_feedback_summary(capture)
        self.assertEqual(summary['counter_increments']['control_cycles'], 8)
        self.assertEqual(summary['counter_increments']['correction_cycles'], 2)
        self.assertTrue(summary['corrected_targets_submitted'])
        self.assertEqual(capture['rows'][0]['trial_correction_vx_mps'], 0)

    def test_wheel_redundancy_schema_is_decoded_and_counted(self):
        rows = [row(10, sequence=i, tick=1000+20*i, valid=64|16384|32768|(1<<16),
                    control_cycles=4*i, feedback_cycles=i, correction_cycles=0,
                    submitted_correction_versions=0, redundancy_mode=1,
                    redundancy_reason=4, redundancy_excluded_wheel=2,
                    redundancy_used_wheel_mask=11, redundancy_confirmations=2,
                    redundancy_corrected=1, redundancy_applied=0,
                    redundancy_threshold_mps=.25, redundancy_dominance_ratio=1.75,
                    redundancy_max_reference_error_mps=.3,
                    redundancy_confirm_samples_required=2)
                for i in range(1, 4)]
        export(self.path, wire_rows(rows), time_at=0, renamed=True)
        capture = read_capture(self.path)
        self.assertEqual(capture['schema'], 10)
        self.assertEqual(capture['channel_count'], 83)
        self.assertEqual(capture['rows'][0]['redundancy_threshold_mps'], .25)
        summary = control_feedback_summary(capture)
        self.assertEqual(summary['wheel_redundancy']['corrected_rows'], 3)
        self.assertEqual(summary['wheel_redundancy']['candidate_wheels'], {'2': 3})

    def test_compact_summary_ignores_boot_history_and_counter_resets(self):
        rows = [row(9, sequence=0, tick=100, valid=64|16384|32768,
                    control_cycles=1000, correction_cycles=100, submitted_correction_versions=50),
                row(9, sequence=0, tick=10, valid=64|16384|32768),
                row(9, sequence=1, tick=30, valid=64)]
        export(self.path, wire_rows(rows))
        result = control_feedback_summary(read_capture(self.path))
        self.assertEqual(result['counter_increments']['correction_cycles'], 0)
        self.assertFalse(result['corrected_targets_submitted'])

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
                              (5, attempt(1, tick=0xfedcba98, wheel_n_x_mps=1.2345)),
                              (6, attempt(1, schema=6, tick=0xfedcba98, wheel_n_x_mps=1.2345))):
            with self.subTest(schema=schema):
                export(self.path, wire_rows([first]), time_at=0, renamed=True)
                capture = read_capture(self.path)
                self.assertEqual(capture['schema'], schema)
                self.assertEqual(capture['rows'][0]['tick_ms'], 0xfedcba98)
                key, value = ('meter_power_w', 12.345) if schema == 4 else ('wheel_n_x_mps', 1.2345)
                self.assertEqual(capture['rows'][0][key], value)

    def test_yaw_extension_keeps_fifo_replay_and_signed_degrees(self):
        rows = [attempt(i, schema=6, valid=31 | 64 | 256 | 512, yaw_ecd=8191,
                        yaw_offset_deg=-15.4248046875, yaw_age_ms=2, chassis_mode=1,
                        gimbal_yaw_total_deg=721.25, gimbal_imu_age_ms=1) for i in (1, 2, 3)]
        export(self.path, wire_rows(rows), time_at=0, renamed=True)
        capture = read_capture(self.path)
        self.assertEqual(capture['channel_count'], 102)
        decoded = capture['rows'][0]
        self.assertEqual(decoded['yaw_ecd'], 8191)
        self.assertEqual(decoded['yaw_offset_deg'], -15.4248046875)
        self.assertEqual(decoded['chassis_mode'], 1)
        self.assertEqual(decoded['gimbal_yaw_total_deg'], 721.25)
        quality = quality_report(capture)
        self.assertEqual(quality['capture_mode'], 'event_fifo')
        self.assertEqual(quality['unique_filter_calls'], 3)

    def test_yaw_control_extension_preserves_signed_command_and_fifo_replay(self):
        rows = [attempt(i, schema=7, valid=31 | 64 | 1024, yaw_ref_deg=731.5,
                        yaw_speed_dps=-12.5, yaw_speed_iout=3000, yaw_command=-7000,
                        yaw_control_flags=79, yaw_control_age_ms=1) for i in (1, 2, 3)]
        export(self.path, wire_rows(rows), time_at=0, renamed=True)
        capture = read_capture(self.path)
        self.assertEqual(capture['channel_count'], 108)
        decoded = capture['rows'][0]
        self.assertEqual(decoded['yaw_ref_deg'], 731.5)
        self.assertEqual(decoded['yaw_speed_dps'], -12.5)
        self.assertEqual(decoded['yaw_speed_iout'], 3000)
        self.assertEqual(decoded['yaw_command'], -7000)
        self.assertEqual(decoded['yaw_control_flags'], 79)
        self.assertEqual(decoded['yaw_control_age_ms'], 1)
        quality = quality_report(capture)
        self.assertEqual(quality['capture_mode'], 'event_fifo')
        self.assertEqual(quality['unique_filter_calls'], 3)

    def test_spin_turn_extension_decodes_count_and_validity(self):
        rows = [attempt(i, schema=8, valid=31 | 64 | 256 | 512 | 1024 | 2048,
                        spin_turn_count=turns, chassis_mode=1)
                for i, turns in ((1, 0.0), (2, 2.5), (3, 103.4165))]
        export(self.path, wire_rows(rows), time_at=0, renamed=True)
        capture = read_capture(self.path)
        self.assertEqual(capture['schema'], 8)
        self.assertEqual(capture['channel_count'], 109)
        decoded = capture['rows'][2]
        self.assertEqual(decoded['spin_turn_count'], 103.4165)
        self.assertEqual(decoded['chassis_mode'], 1)
        self.assertTrue(decoded['valid'] & 2048)
        quality = quality_report(capture)
        self.assertEqual(quality['capture_mode'], 'event_fifo')
        self.assertEqual(quality['unique_filter_calls'], 3)

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

    def test_fifo_capture_is_identified_without_latest_snapshot_warning(self):
        rows = [attempt(i, valid=31 | 64) for i in (1, 2, 3)]
        export(self.path, wire_rows(rows))
        capture = read_capture(self.path)
        quality = quality_report(capture)
        self.assertEqual(capture['channel_count'], 96)
        self.assertEqual(quality['capture_mode'], 'event_fifo')
        self.assertEqual(quality['sequence_kind'], 'capture_records')
        self.assertEqual(quality['capture_fifo_rows'], 3)
        self.assertEqual(quality['missing_filter_calls'], 0)
        self.assertFalse(quality['capture_overflow_observed'])
        self.assertNotIn('50_hz_latest_snapshot_is_not_every_control_or_filter_update',
                         quality['limitations'])

    def test_fifo_heartbeat_and_overflow_do_not_conflate_records_with_calls(self):
        rows = [row(5, sequence=sequence, tick=tick, valid=valid,
                    attempt_sequence=call, attempt_flags=63)
                for sequence, tick, call, valid in
                ((1, 20, 10, 95), (2, 120, 10, 223), (5, 180, 13, 223))]
        export(self.path, wire_rows(rows))
        quality = quality_report(read_capture(self.path))
        self.assertEqual(quality['missing_attempts'], 2)
        self.assertEqual(quality['missing_filter_calls'], 2)
        self.assertEqual(quality['unique_filter_calls'], 2)
        self.assertEqual(quality['duplicate_filter_calls'], 1)
        self.assertTrue(quality['capture_overflow_observed'])
        self.assertEqual(quality['capture_overflow_rows'], 2)
        self.assertIn('capture_fifo_overflow_observed_exact_drop_count_not_recorded',
                      quality['limitations'])

    def test_missing_filter_calls_handles_wrap_and_does_not_count_reboots_as_gaps(self):
        rows = [row(5, sequence=sequence, tick=tick, valid=95,
                    attempt_sequence=call, attempt_flags=63)
                for sequence, tick, call in
                ((0xfffffffe, 0xfffffff0, 0xfffffffe),
                 (0xffffffff, 4, 0), (0, 24, 0), (1, 44, 1), (0, 0, 0))]
        export(self.path, wire_rows(rows))
        quality = quality_report(read_capture(self.path))
        self.assertEqual(quality['missing_filter_calls'], 1)
        self.assertEqual(quality['unique_filter_calls'], 4)
        self.assertEqual(quality['duplicate_filter_calls'], 1)
        self.assertEqual(quality['restart_or_order_boundaries'], 1)

    def test_legacy_snapshot_and_power_bits_keep_their_existing_meaning(self):
        for schema, rows in ((5, [attempt(1), attempt(3)]),
                             (4, [row(4, valid=64 | 128)])):
            with self.subTest(schema=schema):
                export(self.path, wire_rows(rows))
                quality = quality_report(read_capture(self.path))
                self.assertEqual(quality['capture_mode'], 'latest_snapshot')
                self.assertEqual(quality['sequence_kind'], 'send_attempts')
                self.assertFalse(quality['capture_overflow_observed'])
                self.assertIn('50_hz_latest_snapshot_is_not_every_control_or_filter_update',
                              quality['limitations'])
                self.assertEqual(quality['missing_filter_calls'], 1 if schema == 5 else 0)

    def test_mixed_capture_modes_keep_the_snapshot_limitation(self):
        export(self.path, wire_rows([attempt(1), attempt(2, valid=95)]))
        quality = quality_report(read_capture(self.path))
        self.assertEqual(quality['capture_mode'], 'mixed')
        self.assertEqual(quality['sequence_kind'], 'mixed_capture_records_and_send_attempts')
        self.assertIn('mixed_capture_modes', quality['limitations'])
        self.assertIn('50_hz_latest_snapshot_is_not_every_control_or_filter_update',
                      quality['limitations'])

    def test_fifo_startup_without_an_attempt_does_not_invent_filter_calls(self):
        rows = [row(5, sequence=1, tick=0, valid=64),
                row(5, sequence=2, tick=20, valid=95, attempt_sequence=1, attempt_flags=55),
                row(5, sequence=3, tick=40, valid=95, attempt_sequence=2, attempt_flags=63)]
        export(self.path, wire_rows(rows))
        quality = quality_report(read_capture(self.path))
        self.assertEqual(quality['capture_fifo_rows'], 3)
        self.assertEqual(quality['unique_filter_calls'], 2)
        self.assertEqual(quality['missing_filter_calls'], 0)


if __name__ == '__main__':
    unittest.main()
