"""Compile the public A-board configuration as a consumer would."""
import pathlib
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class BoardConfiguration(unittest.TestCase):
    def compile(self, source, *defines):
        return subprocess.run(
            ["gcc", "-x", "c", "-fsyntax-only", "-I", str(ROOT / "Inc"),
             "-I", str(ROOT / "modules/algorithm"),
             *["-D" + value for value in defines], "-"],
            input='#include "board_config.h"\n' + source,
            text=True, capture_output=True)

    def test_vehicle_defaults(self):
        result = self.compile('''
        #if REFEREE_ENABLE != 1 || LOG_OUTPUT != 1 || ATYPE_BENCH_MODE != 0
        #error Wrong vehicle defaults
        #endif
        #if !BOARD_LOG_RTT_ENABLED || BOARD_LOG_UART_ENABLED
        #error Wrong default log transport
        #endif
        ''')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_retired_uart1_selector_is_rejected(self):
        result = self.compile("", "UART1_USE=0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("DBUS", result.stderr)

    def test_log_override_and_invalid_value(self):
        result = self.compile('''
        #if LOG_OUTPUT != 2 || !BOARD_LOG_UART_ENABLED || BOARD_LOG_RTT_ENABLED
        #error Log override ignored
        #endif
        ''', "LOG_OUTPUT=2")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotEqual(self.compile("", "LOG_OUTPUT=4").returncode, 0)

    def test_uart_resource_conflicts(self):
        self.assertNotEqual(self.compile("", "LOG_OUTPUT=2", "BOARD_AUX_UART2_DEVICE=1").returncode, 0)
        self.assertNotEqual(self.compile("", "BOARD_AUX_UART2_DEVICE=2", "BOARD_AUX_UART7_DEVICE=2").returncode, 0)
        self.assertEqual(self.compile("", "BOARD_AUX_UART2_DEVICE=2", "BOARD_AUX_UART7_DEVICE=1").returncode, 0)

    def test_power_stage_and_capture_transport(self):
        source = '#include "chassis_power_config.h"\n'
        self.assertNotEqual(self.compile(source, "CHASSIS_POWER_CONTROL_STAGE=4").returncode, 0)
        self.assertNotEqual(self.compile(source, "CHASSIS_POWER_CAPTURE_ENABLE=1").returncode, 0)
        self.assertEqual(self.compile(source, "CHASSIS_POWER_CAPTURE_ENABLE=1", "LOG_OUTPUT=2").returncode, 0)


if __name__ == "__main__":
    unittest.main()
