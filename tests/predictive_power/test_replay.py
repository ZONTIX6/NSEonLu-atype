import ctypes as ct
import importlib.util
import itertools
import math
from pathlib import Path
import random
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('replay', ROOT/'scripts/predictive-power-replay.py')
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


class Problem(ct.Structure):
    _fields_ = [('q', replay.F4*4), ('linear', replay.F4), ('power_h', replay.F4),
                ('power_g', replay.F4), ('power_c', ct.c_float), ('low', replay.F4),
                ('high', replay.F4), ('scale', ct.c_float), ('budget_w', ct.c_float)]


class Result(ct.Structure):
    _fields_ = [('command', ct.c_int16*4), ('power_w', ct.c_float), ('objective', ct.c_float),
                ('residual', ct.c_float), ('iterations', ct.c_uint32), ('elapsed_us', ct.c_uint32),
                ('feasible', ct.c_bool)]


def solve(a, b):
    """Independent double-precision Gaussian elimination with pivoting."""
    n = len(b)
    a = [list(row)+[rhs] for row, rhs in zip(a, b)]
    for k in range(n):
        pivot = max(range(k, n), key=lambda j: abs(a[j][k]))
        a[k], a[pivot] = a[pivot], a[k]
        for j in range(k+1, n):
            ratio = a[j][k]/a[k][k]
            a[j] = [v-ratio*w for v, w in zip(a[j], a[k])]
    x = [0.0]*n
    for k in reversed(range(n)):
        x[k] = (a[k][n]-sum(a[k][j]*x[j] for j in range(k+1, n)))/a[k][k]
    return x


def reference(q, linear, budget, limit):
    def objective(x):
        return sum(linear[j]*x[j]+sum(q[j][k]*x[j]*x[k] for k in range(4)) for j in range(4))

    def box(multiplier):
        best = None
        for states in itertools.product((-1, 0, 1), repeat=4):
            free = [j for j in range(4) if states[j] == 0]
            fixed = [j for j in range(4) if states[j] != 0]
            x = [limit if states[j] == 1 else 0.0 for j in range(4)]
            solution = solve([[q[j][k]+(multiplier if j == k else 0) for k in free] for j in free],
                             [-linear[j]/2-sum(q[j][k]*x[k] for k in fixed) for j in free])
            for j, v in zip(free, solution):
                x[j] = v
            if all(0 <= v <= limit for v in x):
                value = objective(x)+multiplier*sum(v*v for v in x)
                if best is None or value < best[0]:
                    best = value, x
        return best[1]

    x = box(0)
    if sum(v*v for v in x) > budget:
        low, high = 0, 1
        while sum(v*v for v in box(high)) > budget:
            high *= 2
        for _ in range(45):
            middle = (low+high)/2
            if sum(v*v for v in box(middle)) > budget:
                low = middle
            else:
                high = middle
        x = box(high)
    return objective(x)


class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Windows cannot delete a loaded DLL; keep it in the ignored build tree.
        cls.lib = replay.load_library(ROOT/'build/predictive-host-tests')
        cls.lib.PPAllocate.argtypes = [ct.POINTER(Problem), ct.POINTER(Result)]
        cls.lib.PPAllocate.restype = ct.c_bool

    def test_coupled_solver_against_double_reference(self):
        rng = random.Random(428)
        for case in range(16):
            a = [[rng.uniform(-1, 1) for _ in range(4)] for _ in range(4)]
            q = [[sum(a[k][j]*a[k][l] for k in range(4))+(1 if j == l else 0)
                  for l in range(4)] for j in range(4)]
            linear = [-rng.uniform(1, 12) for _ in range(4)]
            budget = rng.uniform(.5, 6)
            p = Problem(q=(replay.F4*4)(*(replay.F4(*row) for row in q)),
                        linear=replay.F4(*linear), power_h=replay.F4(1, 1, 1, 1),
                        high=replay.F4(3000, 3000, 3000, 3000), scale=1000, budget_w=budget)
            result = Result()
            self.assertTrue(self.lib.PPAllocate(ct.byref(p), ct.byref(result)), case)
            self.assertLessEqual(result.power_w, p.budget_w)
            optimal = reference(q, linear, budget, 3)
            self.assertLess(abs(result.objective-optimal), .05, (case, result.objective, optimal))

    def model(self):
        values = {name: 0 for name, _ in replay.Model._fields_ if name != 'checksum'}
        values.update(schema=1, version=1, command_scale=1000, horizon_s=.01, max_dt_s=.02,
                      max_speed=1000, max_current=30, max_command=15000, base_w=2,
                      resistance=1, power_margin_w=1, speed_error=2, current_error=.01,
                      acceleration_bound=100, braking_valid=False,
                      current_tau=[.01]*4, current_gain=[1]*4, speed_gain=[10]*4, speed_drag=[1]*4)
        return replay.read_model(self.lib, values)

    def test_replay_rejects_duplicate_failed_and_nonfinite_samples(self):
        model = self.model()
        rows = []
        for n in range(6):
            row = dict(sample_ms=str(n*10), meter_sequence=str(n), submitted='1', load_w='2')
            for w in range(4):
                for field in ('current', 'omega', 'command'):
                    row[f'{field}{w}'] = '0'
            rows.append(row)
        rows[1]['submitted'] = '0'
        rows[4]['meter_sequence'] = rows[3]['meter_sequence']
        report = replay.replay(self.lib, model, rows)
        self.assertEqual(report['accepted_pairs'], 2)
        self.assertEqual(report['rejected']['submission'], 1)
        self.assertEqual(report['rejected']['duplicate'], 1)
        self.assertEqual(report['rmse_w'], 0)
        reordered = [dict(rows[n], submitted='1', meter_sequence=seq) for n, seq in zip(range(3), ('10', '9', '10'))]
        report = replay.replay(self.lib, model, reordered)
        self.assertEqual(report['accepted_pairs'], 0)
        duplicate = [dict(rows[n], submitted='1', meter_sequence=seq) for n, seq in zip(range(3), ('10', '10', '11'))]
        duplicate[1]['current0'] = '1000'
        report = replay.replay(self.lib, model, duplicate)
        self.assertEqual(report['accepted_pairs'], 0)  # break interval after rejected state
        self.assertEqual(report['rejected']['domain'], 0)
        rows[3]['load_w'] = 'nan'
        with self.assertRaises(ValueError):
            replay.replay(self.lib, model, rows)


if __name__ == '__main__':
    unittest.main()
