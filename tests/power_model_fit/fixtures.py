"""Deterministic synthetic experiments, never vehicle calibration evidence."""
import csv
import json
import math
from pathlib import Path
import random


def write_example(directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    fields = ['tick_ms', 'meter_sequence', 'submitted', 'meter_online',
              'meter_age_ms', 'enabled_mask', 'online_mask', 'load_w']
    fields += [f'{kind}{w}' for kind in ('command', 'rpm') for w in range(4)]
    manifest = {
        'schema': 1, 'boundary': 'chassis_load',
        'units': {'command': 'dji_command_raw', 'speed': 'rotor_rpm', 'power': 'W'},
        'instrument_error_w': 0.1,
        'steady': {'window_ms': 40, 'max_gap_ms': 20, 'command_span': 0,
                   'rpm_span': 0, 'max_meter_age_ms': 20, 'max_motor_age_ms': 20},
        'experiments': [],
    }
    for index, split in enumerate(('train', 'calibration', 'test')):
        rng = random.Random(417 + index)
        states = [([0]*4, [0]*4), ([10000]*4, [6000]*4)]
        for _ in range(38):
            signs = [rng.choice((-1, 1)) for _ in range(4)]
            states.append(([s*rng.randint(1000, 8500) for s in signs],
                           [s*rng.randint(200, 5000) for s in signs]))
        rows = []
        for command, rpm in states:
            current = [u/1000 for u in command]
            omega = [n*math.pi/30 for n in rpm]
            # Independent generating equation: [P0,R,Km,K1,K2].
            power = 5 + sum(.08*i*i + .02*i*w + .003*abs(w) + .00008*w*w
                            for i, w in zip(current, omega))
            for _ in range(8):
                seq = len(rows) + 1
                row = dict(tick_ms=seq*10, meter_sequence=seq, submitted=1,
                           meter_online=1, meter_age_ms=0, enabled_mask=15,
                           online_mask=15, load_w=power)
                row.update({f'command{w}': command[w] for w in range(4)})
                row.update({f'rpm{w}': rpm[w] for w in range(4)})
                rows.append(row)
        path = directory / f'{split}.csv'
        with path.open('w', newline='', encoding='utf-8') as stream:
            writer = csv.DictWriter(stream, fields)
            writer.writeheader()
            writer.writerows(rows)
        manifest['experiments'].append(dict(id=split, session=f'synthetic-{split}',
            split=split, path=path.name, format='aligned_static', synthetic=True))
    path = directory/'manifest.json'
    path.write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    return path


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    if args.directory.exists():
        parser.error('example directory must not already exist')
    print(write_example(args.directory))
