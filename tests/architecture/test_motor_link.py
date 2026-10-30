"""The reusable motor object must not require vehicle tuning/telemetry symbols."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MotorLinkBoundary(unittest.TestCase):
    def test_motor_compiles_without_vehicle_policy_headers_or_link_symbols(self):
        # A direct call or inline dependency on tuning must make this fail.
        includes = ('tests/dji_chassis_power_rtt/stubs', 'modules/motor/DJImotor',
                    'modules/motor', 'modules/algorithm', 'modules/vehicle_safety', 'Inc')
        with tempfile.TemporaryDirectory() as temporary:
            obj = str(Path(temporary) / 'motor.o')
            compile_result = subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                *['-I' + str(ROOT / p) for p in includes], '-c',
                str(ROOT / 'modules/motor/DJImotor/dji_motor.c'), '-o', obj],
                capture_output=True, text=True)
            self.assertEqual(compile_result.returncode, 0, compile_result.stderr)
            symbols = subprocess.run(['nm', '-u', obj], capture_output=True, text=True, check=True).stdout
            self.assertNotRegex(symbols, r'RuntimeTuning|Autotune|Telemetry|DJITuning')


if __name__ == '__main__':
    unittest.main()
