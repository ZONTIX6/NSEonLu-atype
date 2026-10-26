"""Compile APP entry headers as independent, repeated, multi-file consumers."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ApplicationHeaders(unittest.TestCase):
    def test_public_entries_can_be_included_by_multiple_consumers(self):
        includes = ["-I" + str(ROOT / "application" / directory)
                    for directory in (".", "board", "runtime")]
        source = '''
        #include "robot_task.h"
        #include "board_devices.h"
        #include "tuning_policy.h"
        #include "robot_task.h"
        #include "board_devices.h"
        #include "tuning_policy.h"
        void (*task_entry_{name})(void) = OSTaskInit;
        bool (*board_entry_{name})(void) = BoardDevicesInit;
        void (*tuning_entry_{name})(void) = TuningPolicyInit;
        '''
        with tempfile.TemporaryDirectory() as temp:
            objects = []
            for name in ("first", "second"):
                path = Path(temp) / (name + ".c")
                obj = path.with_suffix(".o")
                path.write_text(source.format(name=name), encoding="utf-8")
                result = subprocess.run(
                    ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                     "-Wstrict-prototypes", *includes, "-c", str(path), "-o", str(obj)],
                    capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                objects.append(str(obj))
            result = subprocess.run(
                ["gcc", "-r", *objects, "-o", str(Path(temp) / "consumers.o")],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
