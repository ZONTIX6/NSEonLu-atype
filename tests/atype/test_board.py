"""Compile the public A-board configuration as a consumer would."""
import pathlib
import subprocess
import tempfile
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
        #if !defined(VOFA_TELEMETRY_MODE) || VOFA_TELEMETRY_MODE != 2 || VOFA_TELEMETRY_ENABLE != 1
        #error Wrong default VOFA configuration
        #endif
        #if !defined(VOFA_OBSERVATION_PROFILE) || VOFA_OBSERVATION_PROFILE != 2
        #error Wrong default VOFA observation profile
        #endif
        ''')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_observation_profile_accepts_only_documented_values(self):
        for mode in (0, 1, 2):
            for profile in (0, 1, 2):
                result = self.compile("", f"VOFA_TELEMETRY_MODE={mode}",
                                      f"VOFA_OBSERVATION_PROFILE={profile}")
                self.assertEqual(result.returncode, 0, result.stderr)
        for profile in (-1, 3):
            result = self.compile("", f"VOFA_OBSERVATION_PROFILE={profile}")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("VOFA_OBSERVATION_PROFILE", result.stderr)

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
        self.assertEqual(self.compile("", "REFEREE_UART_PORT=6", "BOARD_AUX_UART2_DEVICE=2", "BOARD_AUX_UART7_DEVICE=1").returncode, 0)

    def test_referee_uart7_is_exclusive_and_uart6_releases_it(self):
        for device in (1, 2, 3):
            result = self.compile("", "REFEREE_UART_PORT=7", f"BOARD_AUX_UART7_DEVICE={device}")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("UART7", result.stderr)
            self.assertEqual(self.compile("",
                f"BOARD_AUX_UART7_DEVICE={device}").returncode, 0)
            self.assertEqual(self.compile("", "REFEREE_UART_PORT=7", "REFEREE_ENABLE=0",
                f"BOARD_AUX_UART7_DEVICE={device}").returncode, 0)

    def test_referee_cannot_select_ports_owned_by_other_modules(self):
        for port in (0, 1, 2, 3, 8):
            result = self.compile("", f"REFEREE_UART_PORT={port}")
            self.assertNotEqual(result.returncode, 0)

    def test_vofa_reserves_uart7_only_when_enabled(self):
        enabled = "VOFA_TELEMETRY_ENABLE=1"
        disabled = '''
        #if VOFA_TELEMETRY_ENABLE
        #error VOFA should be disabled
        #endif
        '''
        for mode in (0, 1, 2):
            selector = f"VOFA_TELEMETRY_MODE={mode}"
            for owner in ("REFEREE_UART_PORT=7", "BOARD_AUX_UART7_DEVICE=1"):
                with self.subTest(mode=mode, owner=owner):
                    conflict = self.compile("", enabled, selector, owner)
                    self.assertNotEqual(conflict.returncode, 0)
                    self.assertIn("UART7", conflict.stderr)
                    self.assertEqual(self.compile(disabled, selector, owner).returncode, 0)
        self.assertEqual(self.compile(disabled, "VOFA_TELEMETRY_ENABLE=0").returncode, 0)
        self.assertEqual(self.compile(disabled, "CHASSIS_CONTROL_MODE=1").returncode, 0)
        self.assertEqual(self.compile("", enabled, "LOG_OUTPUT=3").returncode, 0)
        self.assertEqual(self.compile("", enabled, "REFEREE_UART_PORT=7",
                                      "REFEREE_ENABLE=0").returncode, 0)

    def test_vofa_mode_rejects_values_outside_protocols(self):
        for mode in (-1, 3):
            with self.subTest(mode=mode):
                result = self.compile("", f"VOFA_TELEMETRY_MODE={mode}")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("VOFA_TELEMETRY_MODE", result.stderr)

    def test_uart7_high_baud_is_exclusive_to_full_justfloat(self):
        cases = [
            (("VOFA_TELEMETRY_MODE=2",), 230400),
            (("VOFA_TELEMETRY_MODE=0",), 115200),
            (("VOFA_TELEMETRY_MODE=1",), 115200),
            (("VOFA_TELEMETRY_MODE=2", "VOFA_TELEMETRY_ENABLE=0"), 115200),
            (("VOFA_TELEMETRY_MODE=2", "REFEREE_UART_PORT=7"), 115200),
            (("VOFA_TELEMETRY_MODE=2", "CHASSIS_CONTROL_MODE=1"), 115200),
        ]
        cases += [(("VOFA_TELEMETRY_MODE=2", f"BOARD_AUX_UART7_DEVICE={device}"), 115200)
                  for device in (1, 2, 3)]
        for defines, expected in cases:
            with self.subTest(defines=defines):
                result = self.compile(f'''
                #if !defined(BOARD_UART7_BAUDRATE) || BOARD_UART7_BAUDRATE != {expected}
                #error Unexpected UART7 baud rate for its selected owner
                #endif
                ''', *defines)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_registered_uart_handles_are_distinct(self):
        source = '''
        #include "board_config.h"
        int huart1, huart2, huart3, huart6, huart7, huart8;
        int main(void) {
            if (BOARD_REFEREE_UART != EXPECT_REFEREE_HANDLE) return 1;
            int *ports[] = {BOARD_REMOTE_UART, BOARD_VISION_UART, BOARD_VTX_UART,
        #if REFEREE_ENABLE
                BOARD_REFEREE_UART,
        #endif
        #if BOARD_LOG_UART_ENABLED
                BOARD_LOG_UART,
        #endif
        #if BOARD_AUX_UART2_DEVICE
                &huart2,
        #endif
        #if BOARD_AUX_UART7_DEVICE
                &huart7,
        #endif
        #if VOFA_TELEMETRY_ENABLE
                BOARD_VOFA_UART,
        #endif
            };
            for (unsigned i = 0; i < sizeof(ports) / sizeof(ports[0]); ++i)
                for (unsigned j = i + 1; j < sizeof(ports) / sizeof(ports[0]); ++j)
                    if (ports[i] == ports[j]) return 2;
            return 0;
        }
        '''
        cases = [([], "(&huart6)"), (["LOG_OUTPUT=3"], "(&huart6)"),
            (["REFEREE_UART_PORT=7"], "(&huart7)"),
            (["REFEREE_UART_PORT=6", "LOG_OUTPUT=2", "BOARD_AUX_UART7_DEVICE=1"], "(&huart6)"),
            (["REFEREE_UART_PORT=6", "BOARD_AUX_UART2_DEVICE=2", "BOARD_AUX_UART7_DEVICE=1"], "(&huart6)")]
        with tempfile.TemporaryDirectory() as temporary:
            target = str(pathlib.Path(temporary) / "uart_config.exe")
            for defines, expected in cases:
                result = subprocess.run(["gcc", "-x", "c", "-I", str(ROOT / "Inc"),
                    *["-D" + value for value in defines],
                    "-DEXPECT_REFEREE_HANDLE=" + expected, "-", "-o", target],
                    input=source, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(subprocess.run([target]).returncode, 0)

    def test_power_stage_and_capture_transport(self):
        source = '#include "chassis_power_config.h"\n'
        self.assertNotEqual(self.compile(source, "CHASSIS_POWER_CONTROL_STAGE=4").returncode, 0)
        self.assertNotEqual(self.compile(source, "CHASSIS_POWER_CAPTURE_ENABLE=1").returncode, 0)
        self.assertEqual(self.compile(source, "CHASSIS_POWER_CAPTURE_ENABLE=1", "LOG_OUTPUT=2").returncode, 0)


if __name__ == "__main__":
    unittest.main()
