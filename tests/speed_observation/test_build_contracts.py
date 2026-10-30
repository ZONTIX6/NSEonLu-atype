"""Compile the real observation option contract, including disabled defaults."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class BuildContracts(unittest.TestCase):
    def compile(self, defines):
        with tempfile.TemporaryDirectory() as tmp:
            return subprocess.run(
                ['gcc', '-std=c11', '-Werror', '-x', 'c', '-c', '-',
                 '-I' + str(ROOT / 'modules/predictive_power'),
                 *['-D' + value for value in defines], '-o', str(Path(tmp) / 'test.o')],
                input='#include "speed_observation_config.h"\nint main(void) { return 0; }\n',
                text=True, capture_output=True)

    def test_valid_modes(self):
        for options in ([], ['CHASSIS_SPEED_CAPTURE_ENABLE=1', 'LOG_OUTPUT=2'],
                        ['CHASSIS_SPEED_KF_SHADOW_ENABLE=1', 'CHASSIS_PREDICTIVE_POWER_MODE=1'],
                        ['CHASSIS_SPEED_KF_SHADOW_ENABLE=1', 'CHASSIS_PREDICTIVE_POWER_MODE=1',
                         'CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1']):
            with self.subTest(options=options):
                result = self.compile(options)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_invalid_modes(self):
        for options in (['CHASSIS_SPEED_CAPTURE_ENABLE=2'], ['CHASSIS_SPEED_KF_SHADOW_ENABLE=-1'],
                        ['CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=2'],
                        ['CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1'],
                        ['CHASSIS_SPEED_CAPTURE_ENABLE=1'],
                        ['CHASSIS_SPEED_CAPTURE_ENABLE=1', 'LOG_OUTPUT=1'],
                        ['CHASSIS_SPEED_CAPTURE_ENABLE=1', 'LOG_OUTPUT=2', 'CHASSIS_CONTROL_MODE=1'],
                        ['CHASSIS_SPEED_KF_SHADOW_ENABLE=1'],
                        ['CHASSIS_SPEED_KF_SHADOW_ENABLE=1', 'CHASSIS_PREDICTIVE_POWER_MODE=1',
                         'CHASSIS_FORCE_CALIBRATION_ENABLE=1']):
            with self.subTest(options=options):
                result = self.compile(options)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('error:', result.stderr)


if __name__ == '__main__':
    unittest.main()
