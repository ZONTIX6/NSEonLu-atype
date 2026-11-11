"""Check migration boundaries; exact baseline entries expire as modules move."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASELINE = Path(__file__).with_name("baseline.json")
INCLUDE = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]', re.M)
STANDARD = {"stdint.h", "stdbool.h", "stddef.h", "stdlib.h", "stdio.h", "string.h",
            "memory.h", "math.h", "float.h", "limits.h", "stdarg.h", "assert.h"}
HARDWARE = re.compile(r'\b(?:HAL_(?:SPI|GPIO|I2C|ADC|DMA|TIM|FLASH|CAN|UART|USART)\w*|__HAL_\w+|h(?:spi|uart|adc|can|tim|i2c)\d+)\b')
REGISTER = re.compile(r'\b(GPIO[A-K]|SPI[1-6]|TIM\d+|DMA\w*|FLASH|ADC\d+|USART\d+|UART\d+)\s*->')
TARGETS = ("modules/chassis_power", "modules/super_cap", "modules/power_meter",
           "modules/predictive_power",
           "modules/chassis_force", "modules/chassis_force_calibration",
           "modules/mpu6500", "modules/vtx", "modules/oled",
           "modules/vehicle_safety", "modules/telemetry", "modules/motor/DJImotor")
FILES = ("modules/imu/ins_service.c", "modules/imu/ins_task.h",
         "modules/motor/DJImotor/dji_motor.c", "modules/motor/DJImotor/dji_motor.h",
         "modules/telemetry/tuning_flash.c")


def code_text(text):
    # Preserve strings (include operands), removing only comments.
    return re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*',
                  lambda m: m[0] if m[0].startswith('"') else ' ', text, flags=re.S)


def scan(root):
    makefile = root / "Makefile"
    include_dirs = [root / name for name in re.findall(r'(?:^|\s)-I([^\s\\]+)',
                    makefile.read_text(encoding="utf-8-sig"))] if makefile.exists() else []
    include_dirs.extend(sorted({p.parent for p in (root / "config").rglob("*.h")}))
    headers = {}
    for base in ("application", "modules", "bsp", "Inc", "Drivers", "config"):
        for path in (root / base).rglob("*.h"):
            headers.setdefault(path.name, []).append(path)
    paths = set()
    for target in TARGETS:
        paths.update(p for p in (root / target).rglob("*") if p.suffix in (".h", ".c"))
    paths.update(root / name for name in FILES if (root / name).exists())
    algorithms = root / "modules/algorithm"
    paths.update(p for p in algorithms.rglob("*") if p.suffix in (".h", ".c"))
    boundary_paths = set(paths)
    # Private application state is protected against every module/application,
    # including legacy modules outside the migration's hardware-rule scope.
    for base in ('application', 'modules'):
        paths.update(p for p in (root / base).rglob('*') if p.suffix in ('.h', '.c'))
    found = Counter()
    for path in sorted(paths):
        rel = path.relative_to(root).as_posix()
        text = code_text(path.read_text(encoding="utf-8-sig"))
        for name in INCLUDE.findall(text):
            if name in STANDARD:
                continue
            local = (path.parent / name).resolve()
            matches = [directory / name for directory in include_dirs if (directory / name).is_file()]
            candidates = [local] if local.is_file() else matches[:1] or headers.get(Path(name).name, [])
            if len(candidates) != 1:
                # Application sources also consume generated experiment headers.
                # Here they are scanned for private-header isolation; the build
                # checks dependency completeness for the selected configuration.
                if path in boundary_paths:
                    found[(rel, "unresolved-include", name)] += 1
                continue
            dest = candidates[0].relative_to(root).as_posix()
            if dest == 'application/chassis/chassis_internal.h' and path.parent != candidates[0].parent:
                found[(rel, 'application-private', dest)] += 1
            if path not in boundary_paths:
                continue
            if rel == 'modules/motor/DJImotor/dji_motor.c' and (
                    dest.startswith('modules/telemetry/') or
                    candidates[0].name == 'dji_tuning_adapter.h'):
                found[(rel, 'motor-vehicle-policy', dest)] += 1
            if rel.startswith('modules/') and dest.startswith("application/"):
                found[(rel, "application-include", dest)] += 1
            if rel.startswith(("modules/algorithm/", "modules/predictive_power/", "modules/chassis_force/", "modules/chassis_force_calibration/")) and (
                    dest.startswith("Drivers/") or dest == "Inc/main.h" or "FreeRTOS" in dest):
                found[(rel, "algorithm-hardware", dest)] += 1
            if rel.startswith("modules/chassis_force_calibration/") and dest.startswith('bsp/'):
                found[(rel, "algorithm-hardware", dest)] += 1
            if rel.startswith("modules/chassis_power/") and path.suffix == ".h" and (
                    candidates[0].name == "dji_motor.h"):
                found[(rel, "power-public-motor", dest)] += 1
        if path in boundary_paths and not rel.startswith("modules/algorithm/"):
            for symbol in HARDWARE.findall(text):
                found[(rel, "hardware-access", symbol)] += 1
            for symbol in REGISTER.findall(text):
                found[(rel, "register-access", symbol)] += 1
    return found


def records(counter):
    return [dict(path=p, rule=r, evidence=e, count=n)
            for (p, r, e), n in sorted(counter.items())]


class BoundaryTests(unittest.TestCase):
    def test_chassis_private_header_rejects_any_module_consumer(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ('application/chassis', 'modules/can_comm'):
                (root / directory).mkdir(parents=True)
            (root / 'application/chassis/chassis_internal.h').write_text('')
            (root / 'modules/can_comm/bridge.c').write_text(
                '#include "../../application/chassis/chassis_internal.h"')
            self.assertEqual(scan(root)[('modules/can_comm/bridge.c',
                'application-private', 'application/chassis/chassis_internal.h')], 1)

    def test_chassis_private_header_stays_inside_its_application(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ('application/chassis', 'application/runtime'):
                (root / directory).mkdir(parents=True)
            (root / 'application/chassis/chassis_internal.h').write_text('')
            (root / 'application/runtime/collector.c').write_text(
                '#include "../chassis/chassis_internal.h"')
            self.assertEqual(scan(root)[('application/runtime/collector.c',
                'application-private', 'application/chassis/chassis_internal.h')], 1)

    def test_motor_driver_rejects_vehicle_tuning_policy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ('modules/motor/DJImotor', 'modules/telemetry'):
                (root / directory).mkdir(parents=True)
            (root / 'modules/telemetry/autotune_protocol.h').write_text('')
            (root / 'modules/motor/DJImotor/dji_motor.c').write_text(
                '#include "../../telemetry/autotune_protocol.h"')
            self.assertEqual(scan(root)[('modules/motor/DJImotor/dji_motor.c',
                'motor-vehicle-policy', 'modules/telemetry/autotune_protocol.h')], 1)

    def test_telemetry_rejects_application_dependencies(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ('modules/telemetry', 'application/board'):
                (root / directory).mkdir(parents=True)
            (root / 'application/board/limits.h').write_text('')
            (root / 'modules/telemetry/protocol.c').write_text(
                '#include "../../application/board/limits.h"')
            self.assertEqual(scan(root)[('modules/telemetry/protocol.c',
                'application-include', 'application/board/limits.h')], 1)

    def test_calibration_core_rejects_rtos_and_application_dependencies(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ('modules/chassis_force_calibration', 'application', 'Inc'):
                (root / directory).mkdir(parents=True)
            (root / 'Inc/main.h').write_text('')
            (root / 'application/policy.h').write_text('')
            (root / 'modules/chassis_force_calibration/x.c').write_text(
                '#include "../../Inc/main.h"\n#include "../../application/policy.h"')
            self.assertEqual(sum(scan(root).values()), 2)

    def test_main_header_is_a_hardware_dependency(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "modules/algorithm").mkdir(parents=True)
            (root / "Inc").mkdir()
            (root / "Inc/main.h").write_text('#include "stm32f4xx_hal.h"')
            (root / "modules/algorithm/x.h").write_text('#include "main.h"')
            result = scan(root)
            self.assertEqual(result[("modules/algorithm/x.h", "algorithm-hardware", "Inc/main.h")], 1)

    def test_resolves_relative_and_unique_public_headers(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "modules/chassis_power").mkdir(parents=True)
            (root / "application").mkdir()
            (root / "application/policy.h").write_text("")
            (root / "modules/chassis_power/x.c").write_text(
                '#include "policy.h"\n#include "../../application/policy.h"\n'
                '// HAL_SPI_Transmit ignored\nHAL_SPI_Transmit();\n')
            result = scan(root)
            self.assertEqual(result[("modules/chassis_power/x.c", "application-include",
                                     "application/policy.h")], 2)
            self.assertEqual(result[("modules/chassis_power/x.c", "hardware-access",
                                     "HAL_SPI_Transmit")], 1)

    def test_ambiguous_include_is_not_silently_accepted(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ("modules/chassis_power", "application/a", "application/b"):
                (root / directory).mkdir(parents=True)
            for directory in ("a", "b"):
                (root / f"application/{directory}/config.h").write_text("")
            (root / "modules/chassis_power/x.c").write_text('#include "config.h"')
            self.assertEqual(sum(scan(root).values()), 1)

    def test_angle_include_and_direct_register_access_are_checked(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "modules/chassis_power").mkdir(parents=True)
            (root / "application").mkdir()
            (root / "application/policy.h").write_text("")
            (root / "modules/chassis_power/x.c").write_text(
                '#include <../../application/policy.h>\nGPIOF->BSRR = 1; SPI5->CR1 = 0;')
            result = scan(root)
            self.assertEqual(result[("modules/chassis_power/x.c", "application-include",
                                     "application/policy.h")], 1)
            self.assertEqual(result[("modules/chassis_power/x.c", "register-access", "GPIOF")], 1)
            self.assertEqual(result[("modules/chassis_power/x.c", "register-access", "SPI5")], 1)

    def test_resolved_motor_rtos_and_can_uart_boundaries(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for directory in ("modules/chassis_power", "modules/motor/DJImotor",
                              "modules/algorithm", "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS"):
                (root / directory).mkdir(parents=True)
            (root / "Makefile").write_text('-IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS')
            (root / "modules/motor/DJImotor/dji_motor.h").write_text("")
            (root / "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.h").write_text("")
            (root / "modules/chassis_power/chassis_power.h").write_text(
                '#include "../motor/DJImotor/dji_motor.h"\nHAL_CAN_AddTxMessage(); HAL_UART_Transmit();')
            (root / "modules/algorithm/x.c").write_text('#include "cmsis_os.h"')
            result = scan(root)
            self.assertEqual(sum(result.values()), 4)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record-baseline", action="store_true")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(BoundaryTests))
    if not result.wasSuccessful():
        return 1
    actual = scan(ROOT)
    if args.record_baseline:
        BASELINE.write_text(json.dumps(records(actual), indent=2) + "\n", encoding="utf-8")
    allowed = Counter() if args.strict else Counter({
        (x["path"], x["rule"], x["evidence"]): x["count"]
        for x in json.loads(BASELINE.read_text(encoding="utf-8"))})
    added, stale = actual - allowed, allowed - actual
    for label, issues in (("NEW", added), ("REMOVE STALE BASELINE", stale)):
        for item in records(issues):
            print(label, json.dumps(item))
    print(f"Architecture: {sum(actual.values())} recorded boundary violations; "
          f"{sum(added.values())} new, {sum(stale.values())} stale")
    return int(bool(added or stale))


if __name__ == "__main__":
    raise SystemExit(main())
