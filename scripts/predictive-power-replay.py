"""Replay aligned SI/equivalent-unit captures through the production C predictor.

Requires GCC and Python stdlib only. No test calibration is installed on a board.
The CSV must contain sample_ms, meter_sequence, submitted, load_w and
current0..3, omega0..3, command0..3. Samples must already share one time base
and measurement boundary; receive time alone is not a sample-time alignment.
"""
import argparse
import csv
import ctypes as ct
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
F4 = ct.c_float * 4


class Model(ct.Structure):
    _fields_ = [(n, ct.c_uint32) for n in ('schema', 'version', 'checksum')] + [
        (n, ct.c_float) for n in ('command_scale', 'horizon_s', 'max_dt_s', 'max_speed',
                                'max_current', 'max_command')] + [
        (n, F4) for n in ('current_tau', 'current_gain', 'speed_gain', 'speed_drag')] + [
        (n, ct.c_float) for n in ('base_w', 'resistance', 'mechanical', 'speed_loss',
                                'speed_squared_loss', 'power_margin_w', 'speed_error',
                                'current_error', 'acceleration_bound')] + [
        ('command_delay_ms', ct.c_uint32), ('braking_valid', ct.c_bool)]


class State(ct.Structure):
    _fields_ = [('current', F4), ('omega', F4), ('applied_command', F4),
                ('tick_ms', ct.c_uint32), ('model_version', ct.c_uint32),
                ('valid', ct.c_bool), ('applied_valid', ct.c_bool)]


class Prediction(ct.Structure):
    _fields_ = [('current', F4), ('omega', F4), ('power_mean_w', ct.c_float),
                ('power_upper_w', ct.c_float), ('h', F4), ('g', F4),
                ('c', ct.c_float), ('valid', ct.c_bool)]


def load_library(directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    library = directory / ('predictive.dll' if sys.platform == 'win32' else 'predictive.so')
    sources = sorted((ROOT / 'modules/predictive_power').glob('*.c'))
    subprocess.run(['gcc', '-shared', '-fPIC', '-std=c11', '-O2', '-fno-fast-math',
                    '-Wall', '-Wextra', '-Werror', *map(str, sources), '-lm', '-o', str(library)], check=True)
    lib = ct.CDLL(str(library.resolve()))
    lib.PPModelChecksum.argtypes = [ct.POINTER(Model)]
    lib.PPModelChecksum.restype = ct.c_uint32
    lib.PPModelValid.argtypes = [ct.POINTER(Model)]
    lib.PPModelValid.restype = ct.c_bool
    lib.PPPredict.argtypes = [ct.POINTER(Model), ct.POINTER(State), ct.POINTER(ct.c_float), ct.c_float, ct.POINTER(Prediction)]
    lib.PPPredict.restype = ct.c_bool
    return lib


def read_model(lib, values):
    model = Model()
    for name, kind in Model._fields_:
        if name == 'checksum':
            continue
        value = values[name]
        if kind is F4:
            if len(value) != 4 or not all(math.isfinite(v) for v in value):
                raise ValueError(f'{name}: four finite values required')
            setattr(model, name, F4(*value))
        else:
            if kind is ct.c_bool and type(value) is not bool:
                raise ValueError(f'{name}: boolean required')
            if kind is ct.c_uint32 and (type(value) is not int or not 0 <= value <= 0xffffffff):
                raise ValueError(f'{name}: uint32 required')
            if not math.isfinite(value):
                raise ValueError(f'{name}: finite value required')
            setattr(model, name, value)
    model.checksum = lib.PPModelChecksum(ct.byref(model))
    if 'checksum' in values and values['checksum'] != model.checksum:
        raise ValueError('model checksum mismatch')
    if not lib.PPModelValid(ct.byref(model)):
        raise ValueError('model rejected by production parameter validation')
    return model


def replay(lib, model, rows):
    errors, under = [], []
    rejected = {'submission': 0, 'time': 0, 'duplicate': 0, 'domain': 0}
    previous = None
    sequence = None
    last_tick = None
    history = []
    for row in rows:
        tick, seq = int(row['sample_ms']), int(row['meter_sequence'])
        if not 0 <= tick <= 0xffffffff or not 0 <= seq <= 0xffffffff:
            raise ValueError('sample_ms and meter_sequence must be uint32')
        if row['submitted'] not in ('0', '1'):
            raise ValueError('submitted must be 0 or 1')
        for name in ('load_w', *(f'{field}{w}' for field in ('current', 'omega', 'command') for w in range(4))):
            if not math.isfinite(float(row[name])):
                raise ValueError(f'non-finite {name}')
        command = [float(row[f'command{w}']) for w in range(4)]
        if any(u != int(u) or not -32768 <= u <= 32767 for u in command):
            raise ValueError('command columns must be final int16 CAN commands')
        if sequence is not None:
            advance = (seq - sequence) & 0xffffffff
            tick_advance = (tick-last_tick) & 0xffffffff
            if advance == 0 or advance >= 0x80000000 or tick_advance == 0 or tick_advance >= 0x80000000:
                rejected['duplicate' if advance == 0 else 'time'] += 1
                previous = None
                history.clear()
                continue  # rejected measurements cannot rewind either high-water mark
        if previous is not None:
            dt = ((tick - int(previous['sample_ms'])) & 0xffffffff) * .001
            held = None
            if model.command_delay_ms:
                # Match the firmware's supported constant-command delay window.
                # history excludes the candidate submitted at previous.sample_ms.
                past_tick = int(previous['sample_ms'])
                for index in range(len(history)-1, -1, -1):
                    past = history[index]
                    age = (past_tick-int(past['sample_ms'])) & 0xffffffff
                    if model.command_delay_ms <= age <= model.command_delay_ms+5:
                        commands = [float(past[f'command{w}']) for w in range(4)]
                        if all(h['submitted'] == '1' and
                               all(float(h[f'command{w}']) == commands[w] for w in range(4))
                               for h in history[index:]):
                            held = commands
                        break
            if not 0 < dt <= model.horizon_s + 1e-9:
                rejected['time'] += 1
                history.clear()
            elif previous['submitted'] != '1':
                rejected['submission'] += 1
            else:
                state = State(current=F4(*(float(previous[f'current{w}']) for w in range(4))),
                              omega=F4(*(float(previous[f'omega{w}']) for w in range(4))),
                              applied_command=F4(*(held or [0]*4)), model_version=model.version,
                              valid=True, applied_valid=held is not None)
                candidate = F4(*(float(previous[f'command{w}']) for w in range(4)))
                prediction = Prediction()
                if lib.PPPredict(ct.byref(model), ct.byref(state), candidate, dt, ct.byref(prediction)):
                    measured = float(row['load_w'])
                    errors.append(prediction.power_mean_w-measured)
                    under.append(max(0, measured-prediction.power_upper_w))
                else:
                    rejected['domain'] += 1
            history.append(previous)
            history = history[-32:]
        previous, sequence, last_tick = row, seq, tick
    return {'accepted_pairs': len(errors), 'rejected': rejected,
            'rmse_w': math.sqrt(sum(e*e for e in errors)/len(errors)) if errors else None,
            'mean_error_w': sum(errors)/len(errors) if errors else None,
            'maximum_upper_bound_violation_w': max(under) if under else None,
            'model_checksum': model.checksum, 'hardware_release': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/predictive-replay')
    args = parser.parse_args()
    lib = load_library(args.build_dir)
    model = read_model(lib, json.loads(args.model.read_text(encoding='utf-8-sig')))
    with args.capture.open(encoding='utf-8-sig', newline='') as stream:
        report = replay(lib, model, csv.DictReader(stream))
    report['capture_sha256'] = hashlib.sha256(args.capture.read_bytes()).hexdigest()
    report['source_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    report['source_dirty'] = bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).strip())
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
    return 0 if report['accepted_pairs'] else 2


if __name__ == '__main__':
    sys.exit(main())
