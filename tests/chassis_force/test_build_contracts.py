"""Verify firmware macro rejection and numeric/source isolation using actual CMake flags."""
import json
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
entries = json.loads((ROOT / "build/cmake-force-release/compile_commands.json").read_text())
selected = next(e for e in entries if e["file"].endswith("/application/board/board_chassis_force.c"))
args = shlex.split(selected["command"])
flags = []
skip = False
for arg in args[1:]:
    if skip:
        skip = False
        continue
    if arg in ("-o", "-c"):
        skip = True
        continue
    if arg.startswith("-DCHASSIS_CONTROL_MODE="):
        continue
    flags.append(arg)

cases = [
    ({}, True),
    ({"CHASSIS_CONTROL_MODE": 1}, True),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_FORCE_EXIT_MODE": 1}, True),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_POWER_SOURCE_MODE": 1}, True),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_FORCE_SPIN_MODE": 0, "POWER_METER_ENABLE": 0}, True),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_FORCE_CALIBRATION_VALID": 1}, True),
    ({"CHASSIS_CONTROL_MODE": 2}, False),
    ({"CHASSIS_FORCE_EXIT_MODE": 2}, False),
    ({"CHASSIS_FORCE_CALIBRATION_VALID": 2}, False),
    ({"CHASSIS_FORCE_SPIN_MODE": 2}, False),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_DRIVE_TYPE": 0}, False),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_POWER_CONTROL_STAGE": 2}, False),
    ({"CHASSIS_CONTROL_MODE": 1, "POWER_METER_ENABLE": 0}, False),
    ({"CHASSIS_CONTROL_MODE": 1, "CHASSIS_POWER_SOURCE_MODE": 1, "CHASSIS_BUFFER_POWER_ENABLE": 0}, False),
]
for defines, valid in cases:
    command = [args[0], *flags, *(f"-D{k}={v}" for k, v in defines.items()),
               "-fsyntax-only", "-x", "c", "-"]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                            input='#include "board/board_chassis_force.h"\n', encoding="utf-8", errors="replace")
    assert (result.returncode == 0) == valid, (defines, result.stderr)
    if not valid:
        assert "error:" in result.stderr

files = {e["file"].replace("\\", "/") for e in entries}
assert not any(p.endswith(("/chassis/chassis.c", "/algorithm/chassis_spin.c", "/DJImotor/dji_power_adapter.c")) for p in files)
for entry in entries:
    path = entry["file"].replace("\\", "/")
    if any(part in path for part in ("/modules/chassis_force/", "/modules/chassis_power/", "/modules/motor/DJImotor/")):
        command = shlex.split(entry["command"])
        assert command.index("-fno-fast-math") > command.index("-Ofast"), path
speed = subprocess.check_output(
    ["arm-none-eabi-nm", "build/cmake-speed-debug/basic_framework.elf"], cwd=ROOT, text=True)
force = subprocess.check_output(
    ["arm-none-eabi-nm", "build/cmake-force-release/basic_framework.elf"], cwd=ROOT, text=True)
assert "ChassisForce" not in speed and "DJIForce" not in speed
assert "ChassisSpinUpdate" not in force and "DJIPowerAdapter" not in force
print(f"PASS: {len(cases)} macro contracts, backend symbols and Release finite-math options")
