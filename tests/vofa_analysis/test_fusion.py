import copy
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from fixtures import attempt
from vofa_csv import continuous_attempt_segments
from vofa_fusion import FusionBridge, analyze_fusion


class SegmentsTests(unittest.TestCase):
    def test_cleared_attempt_valid_bit_breaks_segment(self):
        rows = [attempt(1), attempt(2, valid=15), attempt(3)]
        self.assertEqual([len(s) for s in continuous_attempt_segments(dict(schema=5, rows=rows))], [1, 1])

    def test_held_attempt_deduplicates_even_if_current_reset_count_changed(self):
        rows = [attempt(1, initialized=False), attempt(2), attempt(2), attempt(3)]
        rows[2]['reset_count'] = 7
        segments = continuous_attempt_segments(dict(schema=5, rows=rows))
        self.assertEqual([[r['attempt_sequence'] for r in s] for s in segments], [[1, 2, 3]])

    def test_gap_reset_config_change_and_bad_row_break_segments(self):
        rows = [attempt(1), attempt(2), attempt(4), attempt(5, resets=1),
                attempt(6, resets=1, config_fingerprint=54321), dict(line=7, error='bad'), attempt(7)]
        segments = continuous_attempt_segments(dict(schema=5, rows=rows))
        self.assertEqual([len(s) for s in segments], [2, 1, 1, 1, 1])

    def test_effective_accel_variance_can_change_within_one_static_config(self):
        rows = [attempt(1), attempt(2), attempt(3)]
        rows[1]['fusion_accel_variance'] = 1.5
        rows[2]['fusion_accel_variance'] = 3.0
        self.assertEqual([len(s) for s in continuous_attempt_segments(dict(schema=5, rows=rows))], [3])

    def test_same_attempt_with_changed_input_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'duplicate attempt'):
            continuous_attempt_segments(dict(schema=5, rows=[attempt(1), attempt(1, wheel_n_x_mps=5)]))


class ReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.bridge = FusionBridge(Path(cls.temp.name))

    @classmethod
    def tearDownClass(cls):
        cls.bridge.close()
        cls.temp.cleanup()

    def test_production_seed_rejection_and_reacquisition(self):
        state = self.bridge.new_state()
        first = self.bridge.update(state, attempt(1, initialized=False))
        self.assertTrue(first['fused'])
        rejected = self.bridge.update(state, attempt(2, wheel_n_x_mps=20))
        self.assertFalse(rejected['fused'])
        self.assertEqual(rejected['velocity_n_mps'][0], 20)
        recovered = self.bridge.update(state, attempt(3, wheel_n_x_mps=20))
        self.assertTrue(recovered['fused'])

    def test_continuous_recorded_inputs_replay_and_compare_without_python_filter(self):
        rows = [attempt(i, initialized=i != 1) for i in range(1, 81)]
        state = self.bridge.new_state()
        for r in rows:
            output = self.bridge.update(state, r)
            r['output_n_x_mps'], r['output_n_y_mps'] = output['velocity_n_mps']
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge, stationary=True)
        self.assertEqual(report['replay']['attempts'], 80)
        self.assertEqual(report['replay']['exact_initial_state_segments'], 1)
        self.assertEqual(report['replay']['output_mismatches'], 0)
        self.assertTrue(report['stationary_noise']['available'])
        self.assertFalse(report['hardware_validated'])

    def test_short_gapped_records_cannot_claim_parameter_improvement(self):
        rows = [attempt(1), attempt(3), attempt(5)]
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge, stationary=True)
        self.assertFalse(report['stationary_noise']['available'])
        self.assertIn('insufficient_continuous_inputs', report['limitations'])
        self.assertEqual(report['replay']['scored_attempts'], 0)

    def test_missing_actual_configuration_refuses_replay(self):
        rows = [attempt(1, fusion_wheel_variance=0)]
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge)
        self.assertEqual(report['replay']['attempts'], 0)
        self.assertIn('actual_filter_configuration_unavailable', report['limitations'])

    def test_compiled_source_zero_uses_recorded_effective_parameters(self):
        report = analyze_fusion(dict(schema=5, rows=[attempt(1, config_source=0)]), self.bridge)
        self.assertEqual(report['replay']['attempts'], 1)

    def test_candidate_reports_changes_and_equal_q_r_scaling_limitation(self):
        rows = [attempt(i, initialized=i != 1, wheel_n_x_mps=.02 * (-1 if i % 2 else 1))
                for i in range(1, 81)]
        candidate = dict(fusion_wheel_variance=.02, fusion_accel_variance=.5, fusion_jerk_density=2.)
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge, stationary=True, candidate=candidate)
        self.assertEqual(report['candidate_comparison']['attempts'], 80)
        self.assertEqual(report['candidate_comparison']['parameter_changes'][0]['old'], .01)
        self.assertIn('joint_q_r_scaling_does_not_establish_improvement', report['limitations'])
        self.assertFalse(report['parameter_change_validated'])

    def test_strict_gate_cannot_hide_noisy_samples_from_candidate_comparison(self):
        rows = [attempt(i, initialized=i != 1, wheel_n_x_mps=.6 if i % 10 == 0 else 0.)
                for i in range(1, 101)]
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge, stationary=True,
                                candidate=dict(fusion_max_residual_mps=.05))
        comparison = report['candidate_comparison']
        self.assertGreater(comparison['rejected_or_fallback'], 0)
        self.assertGreater(comparison['filter_deinitializations'], 0)
        noise = report['stationary_noise']
        self.assertEqual(noise['baseline_output']['x']['samples'], 100)
        self.assertEqual(noise['candidate_output']['x']['samples'], 100)
        self.assertGreater(noise['candidate_output']['x']['variance'], 0)
        self.assertLess(noise['common_fused_output']['samples'], 100)

    def test_baseline_reports_response_without_claiming_velocity_truth(self):
        rows = [attempt(i, initialized=i != 1, wheel_n_x_mps=i * .01) for i in range(1, 61)]
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge)
        response = report['response']
        self.assertEqual(response['baseline']['samples'], 60)
        self.assertGreater(response['baseline']['max_output_rate_mps2'], 0)
        self.assertIn('not independent speed accuracy', response['note'])

    def test_invalid_long_segment_cannot_make_short_valid_segments_sufficient(self):
        rows = [attempt(i, fusion_wheel_variance=0) for i in range(1, 101)]
        for group in range(5):
            rows.extend(attempt(200 + group * 100 + i) for i in range(30))
        report = analyze_fusion(dict(schema=5, rows=rows), self.bridge, stationary=True)
        self.assertFalse(report['stationary_noise']['available'])
        self.assertIn('insufficient_continuous_inputs', report['limitations'])


if __name__ == '__main__':
    unittest.main()
