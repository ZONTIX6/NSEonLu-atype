"""Exercise the board selector and fail-closed build/flash workflow without hardware."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/build-board.ps1"
CONFIG = ROOT / "config/00_flash/board_flash_config.h"


class BoardFlashTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="board-flash-test-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.log = self.directory / "commands.log"
        self.env = os.environ.copy()
        self.env["PATH"] = str(self.directory) + os.pathsep + self.env["PATH"]
        self.env["BOARD_FLASH_TEST_LOG"] = str(self.log)
        # Every external build/programming tool is a failing local stub. Even a
        # broken Show implementation cannot contact a connected programming probe.
        for name in ("make", "mingw32-make", "openocd", "JFlash"):
            content = (
                "@echo off\r\n"
                f'>> "%BOARD_FLASH_TEST_LOG%" echo {name} %*\r\n'
                "exit /b 7\r\n"
            )
            (self.directory / f"{name}.cmd").write_bytes(content.encode("ascii"))

    def invoke(self, *arguments, script=SCRIPT):
        return subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
             str(script), *map(str, arguments)],
            cwd=ROOT, env=self.env, capture_output=True, encoding="utf-8",
            errors="replace", timeout=30,
        )

    def show(self, *arguments):
        result = self.invoke("-Action", "Show", *arguments)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return json.loads(result.stdout.lstrip("\ufeff"))

    def config_with(self, **values):
        source = CONFIG.read_text(encoding="utf-8-sig")
        for name, value in values.items():
            source, count = re.subn(
                rf"(^\s*#define\s+{re.escape(name)}\s+)\d+",
                lambda match: match.group(1) + str(value), source, flags=re.M,
            )
            if count == 0 and name == "BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE":
                source += f"\n#define {name} {value}\n"
            else:
                self.assertEqual(count, 1, name)
        # The repository and optional config path may both contain spaces.
        path = self.directory / "board flash config.h"
        path.write_text(source, encoding="utf-8")
        return path

    def commands(self):
        return self.log.read_text(encoding="utf-8").splitlines() if self.log.exists() else []

    def shared_config_fixture(self, definition):
        project = self.directory / "shared config project"
        for relative in ("scripts/build-board.ps1", "config/99_build/atype-options.txt",
                         "config/90_board/board_config.h", "config/00_flash/board_flash_config.h"):
            target = project / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)
        board = project / "config/00_flash/board_flash_config.h"
        board.write_text(re.sub(r"^#define BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE.*$", "",
                               board.read_text(encoding="utf-8-sig"), flags=re.M), encoding="utf-8")
        (project / "config/vofa_config.h").write_text(
            "#ifndef VOFA_OBSERVATION_PROFILE\n" + definition + "\n#endif\n", encoding="utf-8")
        return project

    def test_shared_profile_drives_build_and_flash_arguments(self):
        for profile in (0, 1, 2):
            with self.subTest(profile=profile):
                project = self.shared_config_fixture(f"#define VOFA_OBSERVATION_PROFILE {profile}")
                result = self.invoke("-Action", "Show", script=project / "scripts/build-board.ps1")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                selected = json.loads(result.stdout.lstrip("\ufeff"))
                for key in ("options", "buildArguments", "flashArguments"):
                    self.assertIn(f"VOFA_OBSERVATION_PROFILE={profile}", selected[key])
        self.assertEqual(self.commands(), [])

    def test_shared_profile_is_firmware_default_and_can_be_overridden(self):
        project = self.shared_config_fixture("#define VOFA_OBSERVATION_PROFILE 1")
        for expected, options in ((1, []), (2, ["-DVOFA_OBSERVATION_PROFILE=2"])):
            with self.subTest(expected=expected):
                result = subprocess.run(
                    ["gcc", "-x", "c", "-std=c11", "-fsyntax-only", "-I", str(project), *options, "-"],
                    input='#include "config/90_board/board_config.h"\n'
                          f'_Static_assert(VOFA_OBSERVATION_PROFILE == {expected}, "shared profile");\n',
                    cwd=project, text=True, capture_output=True, timeout=30,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_invalid_shared_profile_stops_before_tools(self):
        for definition in ("", "#define VOFA_OBSERVATION_PROFILE 3",
                           "#define VOFA_OBSERVATION_PROFILE 1 + 1",
                           "#define VOFA_OBSERVATION_PROFILE 1\n#define VOFA_OBSERVATION_PROFILE 2"):
            with self.subTest(definition=definition):
                project = self.shared_config_fixture(definition)
                result = self.invoke("-Action", "Flash", script=project / "scripts/build-board.ps1")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("VOFA_OBSERVATION_PROFILE", result.stderr)
        self.assertEqual(self.commands(), [])

    def test_default_entry_reads_the_config_without_running_tools(self):
        selected = self.show()
        source = CONFIG.read_text(encoding="utf-8-sig")
        target = int(re.search(r"^#define BOARD_FLASH_TARGET\s+(\d+)", source, re.M)[1])
        self.assertEqual(selected["board"], target)
        self.assertEqual(self.commands(), [])

    def test_main_preset_uses_dm_observation_without_obsolete_board_options(self):
        main = self.show("-Board", 1)
        expected = {
            "ONE_BOARD": "1", "CHASSIS_CONTROL_MODE": "0",
            "CHASSIS_FORCE_CALIBRATION_ENABLE": "0", "ATYPE_BENCH_MODE": "0",
            "CHASSIS_VELOCITY_FUSION_CALIBRATED": "0",
            "IMU_CAPTURE_ONLY": "0", "CHASSIS_IMU_CAN_RX_ENABLE": "1",
            "CHASSIS_VELOCITY_FUSION_ENABLE": "1", "CHASSIS_REMOTE_IMU_FUSION_ENABLE": "1",
            "VOFA_TELEMETRY_ENABLE": "1", "VOFA_TELEMETRY_MODE": "2", "USB_TELEMETRY_ENABLE": "1",
            "VOFA_OBSERVATION_PROFILE": "2",
        }
        options = dict(option.split("=", 1) for option in main["options"])
        for key, value in expected.items():
            self.assertEqual(options.get(key), value, key)
        for key in ("CHASSIS_BOARD", "GIMBAL_BOARD", "IMU_CAPTURE_CAN_TX_ENABLE", "CHASSIS_IMU_CAN_SOURCE"):
            self.assertNotIn(key, options)
        self.assertIn("BUILD_DIR=" + main["buildDirectory"], main["buildArguments"])
        self.assertIn("BUILD_DIR=" + main["buildDirectory"], main["flashArguments"])
        self.assertTrue(set(main["options"]) <= set(main["buildArguments"]))
        self.assertNotIn("BOARD_FLASH_2_", CONFIG.read_text(encoding="utf-8-sig"))

    def test_observation_profile_is_forwarded_without_changing_transport(self):
        for profile in (0, 1, 2):
            with self.subTest(profile=profile):
                selected = self.show("-ConfigFile", self.config_with(
                    BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE=profile))
                options = dict(option.split("=", 1) for option in selected["options"])
                self.assertEqual(options["VOFA_OBSERVATION_PROFILE"], str(profile))
                self.assertEqual(options["VOFA_TELEMETRY_MODE"], "2")
                self.assertIn(f"VOFA_OBSERVATION_PROFILE={profile}", selected["buildArguments"])
        self.assertEqual(self.commands(), [])

    def test_invalid_explicit_observation_profile_stops_before_tools(self):
        path = self.config_with(BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE=3)
        result = self.invoke("-ConfigFile", path, "-Action", "Flash")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("VOFA_OBSERVATION_PROFILE", result.stderr)
        self.assertEqual(self.commands(), [])

    def test_zero_and_one_select_the_same_main_board(self):
        self.assertEqual(self.show("-Board", 0), self.show("-Board", 1))

    def test_removed_board_argument_is_rejected_before_tools(self):
        result = self.invoke("-Board", 2, "-Action", "Flash")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.commands(), [])

    def test_invalid_config_selection_stops_before_build_or_flash(self):
        for target in (0, 2, 3):
            with self.subTest(target=target):
                path = self.config_with(BOARD_FLASH_TARGET=target)
                result = self.invoke("-ConfigFile", path, "-Action", "Flash")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.commands(), [])

    def test_invalid_config_cannot_be_masked_by_explicit_main_board(self):
        path = self.config_with(BOARD_FLASH_TARGET=2)
        result = self.invoke("-ConfigFile", path, "-Board", 1, "-Action", "Flash")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.commands(), [])

    def test_output_directory_cannot_escape_build(self):
        for directory in ("../escape", "build/../escape", "D:/escape", "build"):
            with self.subTest(directory=directory):
                source = CONFIG.read_text(encoding="utf-8-sig")
                source = re.sub(r'(^#define BOARD_FLASH_1_BUILD_DIR\s+)"[^"]*"',
                                lambda m: m.group(1) + '"' + directory + '"', source, flags=re.M)
                path = self.directory / "invalid output.h"
                path.write_text(source, encoding="utf-8")
                result = self.invoke("-ConfigFile", path, "-Action", "Flash")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.commands(), [])

    def test_programmer_selection_changes_only_the_download_target(self):
        dap = self.show("-ConfigFile", self.config_with(BOARD_FLASH_PROGRAMMER=1))
        jlink = self.show("-ConfigFile", self.config_with(BOARD_FLASH_PROGRAMMER=2))
        self.assertEqual(dap["programmer"], "DAP")
        self.assertEqual(jlink["programmer"], "JLink")
        self.assertIn("download_dap", dap["flashArguments"])
        self.assertIn("download_jlink", jlink["flashArguments"])
        self.assertEqual(dap["buildArguments"], jlink["buildArguments"])

    def test_show_for_flash_configuration_never_invokes_a_tool(self):
        self.show("-Board", 1, "-Jobs", 3)
        self.assertEqual(self.commands(), [])

    def test_build_failure_never_invokes_download(self):
        result = self.invoke("-Action", "Flash", "-Board", 1)
        self.assertNotEqual(result.returncode, 0)
        commands = self.commands()
        self.assertEqual(len(commands), 1, commands)
        self.assertTrue(commands[0].startswith(("make ", "mingw32-make ")), commands)
        self.assertNotIn("download_", commands[0])
        self.assertIn("IMU_CAPTURE_ONLY=0", commands[0])
        self.assertIn("BUILD_DIR=build/board-1-main", commands[0])


if __name__ == "__main__":
    unittest.main()
