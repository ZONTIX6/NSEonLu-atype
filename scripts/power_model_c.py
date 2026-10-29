"""Host bridge to the production static predictor and limiter."""
import ctypes as ct
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
F4 = ct.c_float * 4


class Model(ct.Structure):
    _fields_ = [(key, ct.c_float) for key in ('k0', 'k1', 'k2', 'k3', 'k4', 'k5', 'current_conversion')]


class Bridge:
    def __init__(self, directory):
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        library = directory / ('static-power.dll' if sys.platform == 'win32' else 'static-power.so')
        self.sources = [ROOT/'tests/power_model_fit/model_bridge.c',
                        ROOT/'modules/algorithm/power_model.c',
                        ROOT/'modules/algorithm/chassis_power_transient.c',
                        ROOT/'modules/algorithm/chassis_power_transient.h',
                        ROOT/'modules/algorithm/power_model.h',
                        ROOT/'modules/algorithm/chassis_power_model_config.h']
        subprocess.run(['gcc', '-shared', '-fPIC', '-std=c11', '-O2', '-fno-fast-math',
            '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'modules/algorithm'),
            *map(str, self.sources[:3]), '-lm', '-o', str(library)], check=True,
            capture_output=True, text=True)
        self.lib = ct.CDLL(str(library.resolve()))
        self.lib.FitBaseline.argtypes = [ct.POINTER(Model)]
        self.lib.FitBaseline.restype = None
        self.lib.FitPredict.argtypes = [ct.POINTER(Model), ct.POINTER(ct.c_float), ct.POINTER(ct.c_float)]
        self.lib.FitPredict.restype = ct.c_float
        self.lib.FitLimit.argtypes = self.lib.FitPredict.argtypes + [ct.c_float,
            ct.POINTER(ct.c_float), ct.POINTER(ct.c_float), ct.POINTER(ct.c_uint)]
        self.lib.FitLimit.restype = ct.c_int
        self.baseline = Model()
        self.lib.FitBaseline(ct.byref(self.baseline))

    def predict(self, model, sample):
        return self.lib.FitPredict(ct.byref(model), F4(*sample['command']), F4(*sample['rpm']))

    def limit(self, model, sample, budget):
        final, total, flags = F4(), ct.c_float(), ct.c_uint()
        valid = self.lib.FitLimit(ct.byref(model), F4(*sample['command']), F4(*sample['rpm']),
                                 budget, final, ct.byref(total), ct.byref(flags))
        if not valid:
            raise ValueError('production C limiter numeric/configuration failure')
        return list(final), total.value, flags.value
