from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from power_model_c import Bridge, Model


class BaselineTests(unittest.TestCase):
    def test_actual_four_wheel_models_are_the_production_prediction_baseline(self):
        with tempfile.TemporaryDirectory() as directory:
            bridge = Bridge(directory)
            try:
                models = tuple(Model(k0=base, k4=.1, current_conversion=1000) for base in (1, 2, 3, 4))
                sample = dict(command=[1000, 2000, 3000, 4000], rpm=[100] * 4)
                self.assertAlmostEqual(bridge.predict(models, sample), 13, places=5)
            finally:
                bridge.close()


if __name__ == '__main__':
    unittest.main()
