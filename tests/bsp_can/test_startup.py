"""Exercise the production BSP against registers, including a frozen HAL tick.

Each scenario runs in a new process so static registration state is realistic.
The original vendor HAL_CAN_Start is extracted at build time: using it as the
old implementation's dependency exposes the actual unbounded wait, rather than
letting an always-successful HAL stub conceal the regression.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def hal_start(source):
    signature = "HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)"
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise ValueError("HAL_CAN_Start body is incomplete")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--case", action="append")
    parser.add_argument("--bsp-source", type=Path, help="optional isolated baseline BSP source")
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    root = here.parents[1]
    cases = args.case or ["timeout", "wrap_timeout", "bus_isolation", "delayed_ack",
                          "no_clock", "not_ready", "notification_error", "success", "unmasked"]
    source = (root / "Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_can.c").read_text(encoding="utf-8")
    failed = []
    with tempfile.TemporaryDirectory(prefix="bsp-can-startup-") as temporary:
        temporary = Path(temporary)
        vendor = temporary / "hal_can_start.c"
        vendor.write_text('#include "can.h"\n' + hal_start(source) + "\n", encoding="utf-8")
        executable = temporary / "test_startup.exe"
        subprocess.run([args.cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O2",
                        "-I" + str(here / "stubs"), "-I" + str(root / "bsp/can"),
                        str(here / "test_startup.c"), str(args.bsp_source or root / "bsp/can/bsp_can.c"),
                        str(vendor), "-o", str(executable)], check=True)
        for case in cases:
            try:
                result = subprocess.run([str(executable), case], capture_output=True,
                                        text=True, timeout=2)
            except subprocess.TimeoutExpired:
                print(f"FAIL: {case}: CANRegister did not return or reach Error_Handler within 2 s")
                failed.append(case)
                continue
            if result.returncode:
                print(f"FAIL: {case}: exit {result.returncode}\n{result.stdout}{result.stderr}")
                failed.append(case)
            else:
                print(result.stdout.strip())
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
