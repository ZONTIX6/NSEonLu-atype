"""Check the actual board header using the installed ARM compiler."""
from pathlib import Path
import re
import json
import subprocess
import unittest
ROOT = Path(__file__).resolve().parents[2]
INCLUDES = re.findall(r'(?:^|\s)-I([^\s\\]+)', (ROOT / 'Makefile').read_text(encoding='utf-8-sig'))
INCLUDES += [str(p.relative_to(ROOT)) for p in sorted({h.parent for h in (ROOT / 'config').rglob('*.h')})]
BASE = dict(CHASSIS_FORCE_CALIBRATION_ENABLE=1, CHASSIS_CONTROL_MODE=1,
            CHASSIS_FORCE_SPIN_MODE=0, CHASSIS_POWER_CONTROL_STAGE=3,
            ATYPE_BENCH_MODE=1, USB_TELEMETRY_ENABLE=1, POWER_METER_ENABLE=1,
            CHASSIS_DRIVE_TYPE=1, ONE_BOARD=1)
class BuildContracts(unittest.TestCase):
    def compile(self, changes):
        flags = BASE | changes
        command = ['arm-none-eabi-gcc', '-std=c11', '-mcpu=cortex-m4', '-mthumb',
                   '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard', '-DSTM32F427xx', '-DUSE_HAL_DRIVER',
                   '-fsyntax-only', '-x', 'c', '-']
        command += ['-I' + str(ROOT / p) for p in INCLUDES]
        command += ['-I' + str(ROOT / 'modules/chassis_force_calibration')]
        command += [f'-D{k}={v}' for k, v in flags.items() if v is not None]
        return subprocess.run(command, input='#include "board/board_force_calibration.h"\n', text=True, capture_output=True, cwd=ROOT)
    def test_valid_bench(self):
        result = self.compile({})
        self.assertEqual(result.returncode, 0, result.stderr)
    def test_illegal_combinations_fail_for_the_right_reason(self):
        for changes, reason in [
            ({'CHASSIS_FORCE_CALIBRATION_ENABLE': 2}, '0 or 1'),
            ({'CHASSIS_CONTROL_MODE': 0}, 'force backend'),
            ({'ONE_BOARD': None, 'CHASSIS_BOARD': 1}, 'ONE_BOARD'),
            ({'CHASSIS_DRIVE_TYPE': 0}, 'OMNI_X'),
            ({'ATYPE_BENCH_MODE': 0}, 'bench'),
            ({'USB_TELEMETRY_ENABLE': 0}, 'USB'),
            ({'POWER_METER_ENABLE': 0}, 'power meter'),
            ({'CHASSIS_POWER_CONTROL_STAGE': 2}, 'stage 3'),
            ({'CHASSIS_FORCE_SPIN_MODE': 1}, 'SPIN=0')]:
            with self.subTest(changes=changes):
                result = self.compile(changes)
                self.assertNotEqual(result.returncode, 0, 'invalid combination accepted')
                self.assertIn(reason, result.stderr)
    def test_built_source_isolation_and_numeric_flags(self):
        for name, enabled in [('speed', False), ('normal', False), ('bench', True)]:
            path = ROOT / f'build/cmake-fc-{name}/compile_commands.json'
            self.assertTrue(path.exists(), 'run the documented CMake builds before this check')
            commands = json.loads(path.read_text(encoding='utf-8'))
            calibration = [c for c in commands if any(x in c['file'].replace('\\', '/') for x in (
                '/modules/chassis_force_calibration/', '/dji_force_calibration.c', '/force_calibration_app.c', '/board_force_calibration.c'))]
            self.assertEqual(bool(calibration), enabled)
            if enabled:
                self.assertEqual(len(calibration), 5)
                for entry in calibration:
                    command = entry['command']
                    self.assertIn('-fno-fast-math', command)
                    self.assertGreater(command.rfind('-fno-fast-math'), command.rfind('-Ofast'))
    def test_elf_storage_and_disabled_symbols(self):
        for name, enabled in [('fc-tool-speed', False), ('fc-tool-normal-force', False), ('fc-tool-bench', True), ('fc-tool-cap', True), ('cmake-fc-bench', True)]:
            elf = ROOT / f'build/{name}/basic_framework.elf'
            self.assertTrue(elf.exists())
            symbols = subprocess.check_output(['arm-none-eabi-nm', '-S', str(elf)], text=True)
            self.assertEqual(bool(re.search(r'\bFcStart$', symbols, re.M)), enabled)
            self.assertEqual(bool(re.search(r'\bDJIForceCalibrationPrepare$', symbols, re.M)), enabled)
            sections = subprocess.check_output(['arm-none-eabi-objdump', '-h', str(elf)], text=True)
            ccm = re.search(r'\.ccmram\s+([0-9a-f]+)\s+([0-9a-f]+)', sections)
            self.assertEqual(int(ccm[1], 16) if ccm else 0, 61440 if enabled else 0)
            if enabled:
                self.assertEqual(int(ccm[2], 16), 0x10000000)
                response = re.search(r'^([0-9a-f]+)\s+00000200\s+[bBdD]\s+response$', symbols, re.M)
                self.assertIsNotNone(response)
                self.assertTrue(0x20000000 <= int(response[1], 16) < 0x20030000)
if __name__ == '__main__':
    unittest.main()
