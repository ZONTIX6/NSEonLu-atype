"""Compile and compare real speed-path final commands in legacy/shadow builds."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT/'build/predictive-verification'
BUILD.mkdir(parents=True, exist_ok=True)
includes = ['tests/dji_chassis_power_rtt/stubs', 'modules/chassis_power', 'modules/algorithm',
            'modules/telemetry', 'modules/super_cap', 'modules/power_meter', 'modules/referee']
sources = [str(p.relative_to(ROOT)) for p in (ROOT/'modules/chassis_power').glob('*.c')
           if p.name != 'chassis_power_force.c']
sources += [str(p.relative_to(ROOT)) for p in (ROOT/'modules/predictive_power').glob('*.c')]
sources += ['modules/algorithm/'+n+'.c' for n in
            ('power_model', 'chassis_power_transient', 'chassis_power_budget', 'chassis_power_outer_loop', 'chassis_spin')]
sources += ['modules/telemetry/rtt_telemetry.c']
outputs = {}
for mode in (0, 1):
    exe = BUILD/f'speed-mode-{mode}.exe'
    command = ['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic', '-O2', '-fno-fast-math',
               '-DCHASSIS_SPIN_MODE=1', '-DPP_TRACE_ACTUATION=1', f'-DCHASSIS_PREDICTIVE_POWER_MODE={mode}',
               *('-I'+directory for directory in includes), 'tests/chassis_power_module/test_spin_policy.c',
               *sources, '-lm', '-o', str(exe)]
    subprocess.run(command, cwd=ROOT, check=True)
    for scenario in ('', 'direct50', 'biased', 'demand50'):
        run = subprocess.run([str(exe), *([scenario] if scenario else [])], cwd=ROOT,
                             capture_output=True, text=True, check=True)
        key = scenario or 'closed-loop'
        (BUILD/f'{key}-{mode}.txt').write_text(run.stdout, encoding='utf-8')
        if mode == 0:
            outputs[key] = run.stdout
        elif outputs[key] != run.stdout:
            raise AssertionError(f'Final CAN output or diagnostics differ: {key}')
        if mode == 1:
            print(f'PASS: {key}: {run.stdout.count("final,")} final CAN records identical')

for invalid in ('2', '-1', '3'):
    result = subprocess.run(['gcc', '-x', 'c', '-c', '-', '-o', str(BUILD/'invalid.o'),
                             '-Imodules/predictive_power', f'-DCHASSIS_PREDICTIVE_POWER_MODE={invalid}'],
                            input='#include "predictive_power_config.h"\n', text=True,
                            capture_output=True, cwd=ROOT)
    if result.returncode == 0 or 'Predictive takeover is not released' not in result.stderr:
        raise AssertionError('Unsupported mode was not rejected: '+invalid)
print('PASS: unsupported takeover/build modes rejected')
