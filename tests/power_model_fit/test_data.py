"""Independent checks of capture normalization and steady-window boundaries."""
import csv
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts'))
from power_model_data import load_manifest, read_sample, select_samples
from fixtures import write_example

STEADY = dict(window_ms=40, max_gap_ms=20, command_span=0, rpm_span=0,
              max_meter_age_ms=20, max_motor_age_ms=20)
WHEELS = ('lf', 'rf', 'lb', 'rb')
LEGACY_HEADER = (
    'tick,nominal_budget_cw,target_power_cw,corrected_budget_cw,effective_limit_cw,'
    'p_unlimited_cw,p_limited_cw,p_meter_cw,p_filtered_cw,error_cw,correction_cw,'
    'min_scale_pm,limited_mask,outer_mode,meter_online,meter_age_ms,motor_enabled_mask,'
    'i_lf_cmd,i_rf_cmd,i_lb_cmd,i_rb_cmd,n_lf_rpm,n_rf_rpm,n_lb_rpm,n_rb_rpm,meter_tick_ms'
).split(',')


def aligned(tick, sequence=None, command=(1000, -2000, 3000, -4000),
            rpm=(100, -200, 300, -400)):
    row = dict(tick_ms=tick, meter_sequence=tick//10 if sequence is None else sequence,
               submitted=1, meter_online=1, meter_age_ms=0, enabled_mask=15,
               online_mask=15, load_w=12.34)
    for w in range(4):
        row[f'command{w}'], row[f'rpm{w}'] = command[w], rpm[w]
    return row


def legacy(tick):
    row = dict.fromkeys(LEGACY_HEADER, 0)
    row.update(tick=tick, meter_tick_ms=tick, meter_online=1,
               motor_enabled_mask=15, p_meter_cw=1234)
    for w, current, speed in zip(WHEELS, (1000, -2000, 3000, -4000), (100, -200, 300, -400)):
        row[f'i_{w}_cmd'], row[f'n_{w}_rpm'] = current, speed
    return row


def cpc1(tick):
    # Exact column names emitted by decode-chassis-power-capture.ps1.
    row = dict(capture_id=7, sequence=tick//10, tick_ms=tick, meter_tick_ms=tick,
               state=4, flags=0x30)
    for prefix, values in (
            ('raw_current', (8000, -8000, 8000, -8000)),
            ('final_current', (1000, -2000, 3000, -4000)),
            ('feedback_current', (900, -1900, 2900, -3900)),
            ('actual_rpm', (100, -200, 300, -400)),
            ('target_rpm', (110, -210, 310, -410))):
        row.update({f'{prefix}_{w}': value for w, value in zip(WHEELS, values)})
    row.update(budget_w=20, measured_w=12.34, actual_model_w=11, guard_model_w=14,
               fast_w=0, slow_w=0, excess_j=0, allowance_j=0)
    row.update({f'motor_age_ms_{w}': 2 for w in WHEELS})
    row.update(meter_age_ms=0, enabled_mask=15, drive_mask=15, brake_mask=0,
               trigger_tick_ms=20, t90_status=2, t90_ms=40, dropped_triggers=0)
    return row


def csv_rows(rows):
    stream = io.StringIO()
    writer = csv.DictWriter(stream, list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
    stream.seek(0)
    return list(csv.DictReader(stream))


class CaptureDataTests(unittest.TestCase):
    def selected(self, rows, fmt='aligned_static'):
        return select_samples(rows, fmt, STEADY)

    def test_real_26_column_format_preserves_signs_scaling_and_missing_evidence(self):
        self.assertEqual(len(LEGACY_HEADER), 26)
        rows = csv_rows([legacy(t) for t in range(0, 60, 10)])
        samples, quality = self.selected(rows, 'legacy26')
        self.assertEqual([s['tick'] for s in samples], [40, 50])
        self.assertEqual(samples[0]['command'], [1000, -2000, 3000, -4000])
        self.assertEqual(samples[0]['rpm'], [100, -200, 300, -400])
        self.assertEqual(samples[0]['power'], 12.34)
        self.assertIsNone(samples[0]['submitted'])
        self.assertIsNone(samples[0]['motor_online'])
        self.assertIn('submission_unavailable', quality['missing_evidence'])
        self.assertIn('meter_sequence_unavailable', quality['missing_evidence'])

    def test_cpc1_uses_final_command_not_raw_demand_or_feedback(self):
        rows = csv_rows([cpc1(t) for t in range(0, 60, 10)])
        samples, quality = self.selected(rows, 'cpc1')
        self.assertEqual([s['tick'] for s in samples], [40, 50])
        self.assertEqual(samples[0]['command'], [1000, -2000, 3000, -4000])
        self.assertEqual(samples[0]['rpm'], [100, -200, 300, -400])
        self.assertEqual(samples[0]['power'], 12.34)
        self.assertIsNone(samples[0]['submitted'])
        self.assertIsNone(samples[0]['online'])
        self.assertIn('receive_time_only', quality['missing_evidence'])

    def test_manifest_reads_both_legacy_formats_without_promoting_their_evidence(self):
        for fmt, factory in (('legacy26', legacy), ('cpc1', cpc1)):
            with self.subTest(fmt=fmt), tempfile.TemporaryDirectory() as directory:
                manifest_path = write_example(directory)
                manifest = json.loads(manifest_path.read_text())
                manifest['experiments'][0]['format'] = fmt
                manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
                rows = [factory(t) for t in range(0, 80, 10)]
                with (Path(directory)/'train.csv').open('w', newline='', encoding='utf-8-sig') as stream:
                    writer = csv.DictWriter(stream, list(rows[0]))
                    writer.writeheader()
                    writer.writerows(rows)
                dataset = load_manifest(manifest_path)
                train = dataset['experiments'][0]
                self.assertEqual(train['quality']['accepted_rows'], 4)
                self.assertTrue(all(s['submitted'] is None for s in train['samples']))
                self.assertTrue(train['quality']['missing_evidence'])

    def test_nonobject_manifest_is_rejected_as_input_error(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'manifest.json'
            for invalid in (None, [], 'schema 1'):
                with self.subTest(invalid=invalid):
                    path.write_text(json.dumps(invalid), encoding='utf-8')
                    with self.assertRaises(ValueError):
                        load_manifest(path)

    def test_idle_samples_keep_shared_baseline_observable(self):
        rows = [aligned(t, command=(0,)*4, rpm=(0,)*4) for t in range(0, 60, 10)]
        for row in rows:
            row['load_w'] = 5
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [40, 50])
        self.assertEqual([s['power'] for s in samples], [5, 5])
        self.assertNotIn('ambiguous_quadrant', quality['rejected'])

    def test_one_braking_wheel_rejects_positive_aggregate_mechanical_power(self):
        rows = [aligned(t) for t in range(0, 60, 10)]
        for row in rows:
            row['command0'] = -1000
        samples, quality = self.selected(rows)
        self.assertEqual(samples, [])
        self.assertEqual(quality['rejected']['braking'], 6)

    def test_missing_nonfinite_and_out_of_range_fields_break_windows(self):
        for corruption in ('missing', 'nan', 'oversized_command'):
            with self.subTest(corruption=corruption):
                rows = [aligned(t) for t in range(0, 120, 10)]
                if corruption == 'missing':
                    del rows[6]['rpm2']
                elif corruption == 'nan':
                    rows[6]['load_w'] = 'nan'
                else:
                    rows[6]['command2'] = 32768
                samples, quality = self.selected(rows)
                self.assertEqual([s['tick'] for s in samples], [40, 50, 110])
                self.assertEqual(quality['rejected']['numeric'], 1)

    def test_long_gap_starts_new_steady_window(self):
        rows = [aligned(t) for t in (0, 10, 20, 30, 40, 100, 110, 120, 130, 140)]
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [40, 140])
        self.assertEqual(quality['accepted_rows'], 2)

    def test_csv_row_with_extra_columns_cannot_bridge_a_steady_window(self):
        source = io.StringIO()
        rows = [aligned(t) for t in range(0, 120, 10)]
        writer = csv.DictWriter(source, list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
        lines = source.getvalue().splitlines()
        lines[7] += ',unexpected trailing column'
        parsed = list(csv.DictReader(io.StringIO('\n'.join(lines))))
        samples, quality = self.selected(parsed)
        self.assertEqual([s['tick'] for s in samples], [40, 50, 110])
        self.assertEqual(quality['rejected']['numeric'], 1)

    def test_invalid_measurement_breaks_stability_even_when_commands_stay_constant(self):
        for change, reason in ((dict(submitted=0), 'submission'),
                               (dict(meter_age_ms=21), 'stale'),
                               (dict(online_mask=7), 'motor_health')):
            with self.subTest(reason=reason):
                rows = [aligned(t) for t in range(0, 120, 10)]
                rows[6].update(change)
                samples, quality = self.selected(rows)
                self.assertEqual([s['tick'] for s in samples], [40, 50, 110])
                self.assertEqual(quality['rejected'][reason], 1)

    def test_duplicate_event_is_not_an_extra_measurement_but_keeps_stability(self):
        rows = [aligned(t) for t in range(0, 120, 10)]
        rows[6]['meter_sequence'] = 5
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [40, 50, 70, 80, 90, 100, 110])
        self.assertEqual(quality['rejected']['duplicate'], 1)

    def test_faster_control_log_accepts_each_slower_meter_event_once(self):
        for fmt, factory in (('aligned_static', aligned), ('legacy26', legacy), ('cpc1', cpc1)):
            with self.subTest(fmt=fmt):
                rows = [factory(t) for t in range(0, 82, 2)]
                for tick, row in zip(range(0, 82, 2), rows):
                    event = tick//10
                    row['meter_sequence' if fmt == 'aligned_static' else 'meter_tick_ms'] = (
                        event if fmt == 'aligned_static' else event*10)
                    row['meter_age_ms'] = tick % 10
                samples, quality = self.selected(rows, fmt)
                self.assertEqual([s['tick'] for s in samples], [40, 50, 60, 70, 80])
                self.assertEqual(len({s['event'] for s in samples}), 5)
                self.assertEqual(quality['rejected']['duplicate'], 32)

    def test_bad_repeated_meter_row_still_breaks_stability(self):
        for change, reason in ((dict(submitted=0), 'submission'),
                               (dict(meter_online=0), 'stale'),
                               (dict(online_mask=7), 'motor_health'),
                               (dict(command0=-1000), 'braking')):
            with self.subTest(reason=reason):
                rows = [aligned(t, t//10) for t in range(0, 122, 2)]
                rows[22].update(change)  # 44 ms repeats the 40 ms meter event.
                samples, quality = self.selected(rows)
                self.assertEqual([s['tick'] for s in samples], [40, 90, 100, 110, 120])
                self.assertEqual(quality['rejected'][reason], 1)

    def test_command_jump_between_meter_events_delays_steady_acceptance(self):
        rows = [aligned(t, t//10) for t in range(0, 122, 2)]
        rows[22]['command0'] = 1500
        samples, _ = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [40, 90, 100, 110, 120])

    def test_repeated_meter_event_advances_control_time_high_water(self):
        rows = [aligned(t) for t in range(0, 60, 10)]
        rows += [aligned(60, 5), aligned(55, 6)]
        rows += [aligned(t) for t in range(70, 120, 10)]
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [40, 50, 110])
        self.assertEqual(quality['rejected']['order'], 1)
        self.assertEqual(quality['rejected']['duplicate'], 1)

    def test_repeated_event_stream_preserves_independent_time_and_sequence_wrap(self):
        rows = [aligned((0xfffffff6+t) & 0xffffffff, (0xfffffffb+t//10) & 0xffffffff)
                for t in range(0, 82, 2)]
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [30, 40, 50, 60, 70])
        self.assertEqual([s['event'] for s in samples], [0xffffffff, 0, 1, 2, 3])
        self.assertNotIn('order', quality['rejected'])

    def test_backward_events_cannot_rewind_time_or_event_high_water_marks(self):
        rows = [aligned(t) for t in range(0, 130, 10)]
        rows[6]['tick_ms'] = 40
        rows[7]['meter_sequence'] = 2
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [40, 50, 120])
        self.assertEqual(quality['rejected']['order'], 2)

    def test_time_and_event_wrap_keep_their_forward_meaning(self):
        rows = [aligned((0xffffffe7+t) & 0xffffffff, (0xfffffffc+t//10) & 0xffffffff)
                for t in range(0, 60, 10)]
        samples, quality = self.selected(rows)
        self.assertEqual([s['tick'] for s in samples], [15, 25])
        self.assertNotIn('order', quality['rejected'])

    def test_cpc1_corrupt_quantized_and_stale_motor_records_are_rejected(self):
        for field, value, reason in (('flags', 0xb0, 'capture_flags'),
                                      ('flags', 0x31, 'capture_flags'),
                                      ('flags', 0x32, 'capture_flags'),
                                      ('flags', 0x38, 'capture_flags'),
                                      ('motor_age_ms_rb', 21, 'motor_health')):
            with self.subTest(field=field, value=value):
                rows = [cpc1(t) for t in range(0, 60, 10)]
                for row in rows:
                    row[field] = value
                samples, quality = self.selected(rows, 'cpc1')
                self.assertEqual(samples, [])
                self.assertEqual(quality['rejected'][reason], 6)

    def test_values_outside_production_float_range_cannot_be_accepted(self):
        for field in ('rpm0', 'load_w'):
            with self.subTest(field=field):
                rows = [aligned(t) for t in range(0, 60, 10)]
                for row in rows:
                    row[field] = 1e100
                samples, quality = self.selected(rows)
                self.assertEqual(samples, [])
                self.assertEqual(quality['rejected']['numeric'], 6)

    def test_command_signs_do_not_turn_unknown_submission_into_success(self):
        for fmt, factory in (('legacy26', legacy), ('cpc1', cpc1)):
            with self.subTest(fmt=fmt):
                sample = read_sample(factory(10), fmt)
                self.assertIsNone(sample['submitted'])


if __name__ == '__main__':
    unittest.main()
