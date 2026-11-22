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
        vofa_config = project / "config/_vofa/vofa_config.h"
        vofa_config.parent.mkdir(parents=True, exist_ok=True)
        vofa_config.write_text(
            "#ifndef VOFA_OBSERVATION_PROFILE\n" + definition + "\n#endif\n", encoding="utf-8")
        return project

    def test_shared_profile_drives_build_and_flash_arguments(self):
        for profile in (0, 1, 2, 3):
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
        for definition in ("", "#define VOFA_OBSERVATION_PROFILE 4",
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

    def test_presets_use_matching_can_roles_and_distinct_outputs(self):
        main = self.show("-Board", 1)
        imu = self.show("-Board", 2)
        self.assertNotEqual(main["buildDirectory"], imu["buildDirectory"])
        common = {
            "ONE_BOARD": "1", "CHASSIS_CONTROL_MODE": "0",
            "CHASSIS_FORCE_CALIBRATION_ENABLE": "0", "ATYPE_BENCH_MODE": "0",
            "CHASSIS_VELOCITY_FUSION_CALIBRATED": "0",
            "VOFA_OBSERVATION_PROFILE": "2",
        }
        cases = (
            (main, {"IMU_CAPTURE_ONLY": "0", "IMU_CAPTURE_CAN_TX_ENABLE": "0",
                    "CHASSIS_IMU_CAN_RX_ENABLE": "1", "CHASSIS_VELOCITY_FUSION_ENABLE": "1",
                    "CHASSIS_REMOTE_IMU_FUSION_ENABLE": "1",
                    "VOFA_TELEMETRY_ENABLE": "1", "VOFA_TELEMETRY_MODE": "2", "USB_TELEMETRY_ENABLE": "1"}),
            (imu, {"IMU_CAPTURE_ONLY": "1", "IMU_CAPTURE_CAN_TX_ENABLE": "1",
                   "CHASSIS_IMU_CAN_RX_ENABLE": "0", "CHASSIS_VELOCITY_FUSION_ENABLE": "0",
                   "CHASSIS_REMOTE_IMU_FUSION_ENABLE": "0",
                   "VOFA_TELEMETRY_ENABLE": "0", "VOFA_TELEMETRY_MODE": "1", "USB_TELEMETRY_ENABLE": "0"}),
        )
        for selected, role in cases:
            with self.subTest(board=selected["board"]):
                options = dict(option.split("=", 1) for option in selected["options"])
                for key, value in {**common, **role}.items():
                    self.assertEqual(options.get(key), value, key)
                for key in ("CHASSIS_BOARD", "GIMBAL_BOARD", "CHASSIS_IMU_CAN_SOURCE"):
                    self.assertNotIn(key, options)
                self.assertIn("BUILD_DIR=" + selected["buildDirectory"], selected["buildArguments"])
                self.assertIn("BUILD_DIR=" + selected["buildDirectory"], selected["flashArguments"])
                self.assertTrue(set(selected["options"]) <= set(selected["buildArguments"]))

    def test_observation_profile_is_forwarded_without_changing_transport(self):
        for profile in (0, 1, 2, 3):
            with self.subTest(profile=profile):
                selected = self.show("-ConfigFile", self.config_with(
                    BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE=profile))
                options = dict(option.split("=", 1) for option in selected["options"])
                self.assertEqual(options["VOFA_OBSERVATION_PROFILE"], str(profile))
                self.assertEqual(options["VOFA_TELEMETRY_MODE"], "2")
                self.assertIn(f"VOFA_OBSERVATION_PROFILE={profile}", selected["buildArguments"])
        self.assertEqual(self.commands(), [])

    def test_invalid_explicit_observation_profile_stops_before_tools(self):
        path = self.config_with(BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE=4)
        result = self.invoke("-ConfigFile", path, "-Action", "Flash")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("VOFA_OBSERVATION_PROFILE", result.stderr)
        self.assertEqual(self.commands(), [])

    def test_zero_and_one_select_the_same_main_board(self):
        self.assertEqual(self.show("-Board", 0), self.show("-Board", 1))

    def test_config_selection_and_explicit_override(self):
        path = self.config_with(BOARD_FLASH_TARGET=2)
        self.assertEqual(self.show("-ConfigFile", path)["board"], 2)
        self.assertEqual(self.show("-ConfigFile", path, "-Board", 1)["board"], 1)

    def test_invalid_board_argument_is_rejected_before_tools(self):
        result = self.invoke("-Board", 3, "-Action", "Flash")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.commands(), [])

    def test_invalid_config_selection_stops_before_build_or_flash(self):
        for target in (0, 3):
            with self.subTest(target=target):
                path = self.config_with(BOARD_FLASH_TARGET=target)
                result = self.invoke("-ConfigFile", path, "-Action", "Flash")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.commands(), [])

    def test_invalid_config_cannot_be_masked_by_explicit_main_board(self):
        path = self.config_with(BOARD_FLASH_TARGET=3)
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

    def test_boards_cannot_share_the_same_output_directory(self):
        source = CONFIG.read_text(encoding="utf-8-sig")
        main_directory = re.search(r'^#define BOARD_FLASH_1_BUILD_DIR\s+"([^"]+)"', source, re.M)[1]
        source = re.sub(r'(^#define BOARD_FLASH_2_BUILD_DIR\s+)"[^"]*"',
                        lambda m: m.group(1) + '"build/' + main_directory[6:].upper() + '"', source, flags=re.M)
        path = self.directory / "shared output.h"
        path.write_text(source, encoding="utf-8")
        result = self.invoke("-ConfigFile", path, "-Action", "Flash", "-Board", 2)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("different build directories", result.stderr)
        self.assertEqual(self.commands(), [])

    def test_programmer_selection_changes_only_the_download_target(self):
        dap = self.show("-ConfigFile", self.config_with(BOARD_FLASH_PROGRAMMER=1))
        jlink = self.show("-ConfigFile", self.config_with(BOARD_FLASH_PROGRAMMER=2))
        self.assertEqual(dap["programmer"], "DAP")
        self.assertEqual(jlink["programmer"], "JLink")
        self.assertIn("download_dap", dap["flashArguments"])
        self.assertIn("download_jlink", jlink["flashArguments"])
        self.assertEqual(dap["buildArguments"], jlink["buildArguments"])

    def test_named_download_overrides_programmer_and_preserves_selected_board(self):
        for configured, requested, target in ((2, "DAP", "download_dap"),
                                              (1, "JLink", "download_jlink")):
            with self.subTest(programmer=requested):
                path = self.config_with(BOARD_FLASH_TARGET=2, BOARD_FLASH_PROGRAMMER=configured)
                baseline = self.show("-ConfigFile", path)
                selected = self.show("-ConfigFile", path, "-Programmer", requested)
                self.assertEqual(selected["board"], 2)
                self.assertEqual(selected["programmer"], requested)
                self.assertEqual(selected["flashArguments"][0], target)
                self.assertEqual(selected["buildArguments"], baseline["buildArguments"])
                self.assertEqual(selected["flashArguments"][1:], baseline["flashArguments"][1:])
        self.assertEqual(self.commands(), [])

    def test_invalid_programmer_override_stops_before_tools(self):
        result = self.invoke("-Action", "Flash", "-Programmer", "unknown")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.commands(), [])

    def test_show_for_flash_configuration_never_invokes_a_tool(self):
        self.show("-Board", 2, "-Jobs", 3)
        self.assertEqual(self.commands(), [])

    def test_build_failure_never_invokes_download(self):
        for board, capture, directory in ((1, 0, "board-1-main"), (2, 1, "board-2-imu")):
            with self.subTest(board=board):
                before = len(self.commands())
                result = self.invoke("-Action", "Flash", "-Board", board)
                self.assertNotEqual(result.returncode, 0)
                commands = self.commands()[before:]
                self.assertEqual(len(commands), 1, commands)
                self.assertTrue(commands[0].startswith(("make ", "mingw32-make ")), commands)
                self.assertNotIn("download_", commands[0])
                self.assertIn(f"IMU_CAPTURE_ONLY={capture}", commands[0])
                self.assertIn(f"BUILD_DIR=build/{directory}", commands[0])

    def debug_fixture(self):
        project = self.shared_config_fixture("#define VOFA_OBSERVATION_PROFILE 2")
        for board, directory in ((1, "board-1-main"), (2, "board-2-imu")):
            elf = project / "build" / directory / "basic_framework.elf"
            elf.parent.mkdir(parents=True, exist_ok=True)
            elf.write_bytes(f"board-{board}-elf".encode())
        for name in ("make", "mingw32-make"):
            stub = self.directory / f"{name}.cmd"
            stub.write_bytes(stub.read_bytes().replace(b"exit /b 7", b"exit /b 0"))
        tasks = json.loads((ROOT / ".vscode/tasks.json").read_text(encoding="utf-8-sig"))["tasks"]
        # Preserve strings (including URLs) while removing JSONC comments.
        launch = (ROOT / ".vscode/launch.json").read_text(encoding="utf-8-sig")
        launch = re.sub(r'("(?:\\.|[^"\\])*")|//[^\n]*|/\*[\s\S]*?\*/',
                        lambda match: match.group(1) or "", launch)
        launch = re.sub(r",\s*([}\]])", r"\1", launch)
        return project, {task["label"]: task for task in tasks}, json.loads(launch)["configurations"]

    def select_debug_board(self, project, board):
        config = project / "config/00_flash/board_flash_config.h"
        source = config.read_text(encoding="utf-8-sig")
        config.write_text(re.sub(r"(^#define BOARD_FLASH_TARGET\s+)\d+",
                                lambda match: match.group(1) + str(board), source, flags=re.M),
                          encoding="utf-8")

    def run_debug_task(self, project, tasks, launch):
        self.assertIn("preLaunchTask", launch, launch["name"])
        task = tasks[launch["preLaunchTask"]]
        arguments = task["args"]
        script = arguments[arguments.index("-File") + 1].replace("${workspaceFolder}", str(project))
        return self.invoke(*arguments[arguments.index("-File") + 2:], script=Path(script))

    def debug_executable(self, project, launch):
        return Path(launch["executable"].replace("${workspaceRoot}", str(project))
                    .replace("${workspaceFolder}", str(project)))

    def test_debug_entries_follow_selection_except_dap_attach_stays_on_main_board(self):
        project, tasks, launches = self.debug_fixture()
        for board in (2, 1):
            self.select_debug_board(project, board)
            for launch in launches:
                with self.subTest(board=board, launch=launch["name"]):
                    before = len(self.commands())
                    if launch["name"] == "DAP-attach":
                        self.assertEqual(launch["request"], "attach")
                        self.assertNotIn("preLaunchTask", launch)
                        expected_board = 1
                    else:
                        result = self.run_debug_task(project, tasks, launch)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertNotIn("programmer:", result.stdout)
                        expected_board = board
                    self.assertEqual(self.debug_executable(project, launch).read_bytes(),
                                     f"board-{expected_board}-elf".encode())
                    commands = self.commands()[before:]
                    if launch["request"] == "launch":
                        self.assertEqual(len(commands), 1, commands)
                        self.assertIn(f"IMU_CAPTURE_ONLY={int(board == 2)}", commands[0])
                        self.assertNotIn("download_", commands[0])
                    else:
                        self.assertEqual(commands, [])

    def test_failed_debug_preparation_cannot_reuse_the_previous_board_elf(self):
        for failure in ("build", "config", "missing-elf"):
            with self.subTest(failure=failure):
                project, tasks, launches = self.debug_fixture()
                launch = next(entry for entry in launches if entry["name"] == "Jlink")
                executable = self.debug_executable(project, launch)
                executable.parent.mkdir(parents=True, exist_ok=True)
                executable.write_bytes(b"previous-board-elf")
                self.select_debug_board(project, 2)
                if failure == "build":
                    for name in ("make", "mingw32-make"):
                        stub = self.directory / f"{name}.cmd"
                        stub.write_bytes(stub.read_bytes().replace(b"exit /b 0", b"exit /b 7"))
                elif failure == "config":
                    self.select_debug_board(project, 3)
                else:
                    (project / "build/board-2-imu/basic_framework.elf").unlink()
                result = self.run_debug_task(project, tasks, launch)
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse(executable.exists(), "stale ELF must not remain loadable")

    def test_attach_reports_missing_selected_board_elf_without_building_or_using_stale_symbols(self):
        project, tasks, launches = self.debug_fixture()
        self.select_debug_board(project, 2)
        (project / "build/board-2-imu/basic_framework.elf").unlink()
        launch = next(entry for entry in launches if entry["name"] == "Jlink-attach")
        executable = self.debug_executable(project, launch)
        executable.parent.mkdir(parents=True, exist_ok=True)
        executable.write_bytes(b"previous-board-symbols")
        result = self.run_debug_task(project, tasks, launch)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(executable.exists())
        self.assertEqual(self.commands(), [])

    def test_debug_uses_custom_selected_board_directory(self):
        project, tasks, launches = self.debug_fixture()
        self.select_debug_board(project, 2)
        config = project / "config/00_flash/board_flash_config.h"
        config.write_text(config.read_text(encoding="utf-8").replace(
            '"build/board-2-imu"', '"build/custom/imu"'), encoding="utf-8")
        elf = project / "build/custom/imu/basic_framework.elf"
        elf.parent.mkdir(parents=True)
        elf.write_bytes(b"custom-imu-elf")
        launch = next(entry for entry in launches if entry["name"] == "Jlink")
        result = self.run_debug_task(project, tasks, launch)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.debug_executable(project, launch).read_bytes(), b"custom-imu-elf")
        self.assertIn("BUILD_DIR=build/custom/imu", self.commands()[0])


if __name__ == "__main__":
    unittest.main()
