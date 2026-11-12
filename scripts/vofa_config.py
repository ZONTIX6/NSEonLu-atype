"""Generate and validate effective power configuration evidence using production C.

An artifact is only usable after matching its fingerprint, version and source to
the captured cycle. Generating compiled defaults does not identify a live board.
"""
import argparse
import functools
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIELDS_FILE = ROOT/'modules/chassis_power/chassis_power_config_fields.def'
MODEL_KEYS = ('k0', 'k1', 'k2', 'k3', 'k4', 'k5', 'current_conversion')
SOURCE_NAMES = {'compiled': 0, 'file': 1, 'flash': 2, 'runtime': 3}
_TEMP = tempfile.TemporaryDirectory(prefix='vofa-config-')


def _fields():
    return re.findall(r'VOFA_POWER_(FLOAT|U32)\(([^)]+)\)', FIELDS_FILE.read_text())


@functools.lru_cache(maxsize=8)
def _exporter(defines):
    directory = Path(_TEMP.name)/str(len(list(Path(_TEMP.name).iterdir())))
    directory.mkdir()
    executable = directory/'config-export.exe'
    sources = [ROOT/'scripts/vofa_config_export.c', ROOT/'modules/chassis_power/chassis_power.c',
               ROOT/'config/20_chassis_power/chassis_power_config.c',
               *[ROOT/'modules/algorithm'/name for name in ('power_model.c',
                  'chassis_power_transient.c', 'chassis_power_budget.c', 'chassis_power_outer_loop.c')]]
    include_dirs = ('modules/chassis_power', 'modules/algorithm', 'modules/telemetry',
                    'modules/super_cap', 'modules/power_meter', 'modules/referee')
    command = ['gcc', '-std=c11', '-O2', '-fno-fast-math', '-Wall', '-Wextra', '-Werror',
               '-ffunction-sections', '-fdata-sections',
               *['-I'+str(ROOT/p) for p in include_dirs],
               *['-D'+key+'='+value for key, value in defines],
               *map(str, sources), '-Wl,--gc-sections', '-lm', '-o', str(executable)]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise ValueError('configuration exporter build failed: '+result.stderr.strip())
    return executable


def _run(defines=(), arguments=()):
    result = subprocess.run([str(_exporter(defines)), *arguments], capture_output=True, text=True)
    if result.returncode:
        raise ValueError('effective configuration rejected by production C (mode, values or file override)')
    return json.loads(result.stdout)


def _defines(defines):
    result = tuple(sorted((str(k), str(v)) for k, v in (defines or {}).items()))
    if any(not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', k) or not v or '\n' in v for k, v in result):
        raise ValueError('invalid compiler definition')
    return result


def _reply(text):
    if not isinstance(text, str):
        raise ValueError('an original power GET reply is required')
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if len(lines) != 1:
        raise ValueError('exactly one power GET reply is required')
    cells = lines[0].split(',')
    if len(cells) != 26 or cells[0] != '@T1' or cells[2:4] != ['OK', 'power']:
        raise ValueError('expected the original @T1 power reply with 19 float32 values')
    if any(not re.fullmatch(r'[0-9]+', cells[i]) for i in (1, 4, 5)):
        raise ValueError('invalid tuning reply identity')
    if any(int(cells[i]) > 0xffffffff for i in (1, 4, 5)):
        raise ValueError('tuning reply identity exceeds u32')
    if any(not re.fullmatch(r'[0-9a-fA-F]{8}', word) for word in cells[6:25]):
        raise ValueError('invalid tuning float32 word')
    if cells[25] not in ('unsupported', 'saved', 'pending', 'error', 'volatile'):
        raise ValueError('invalid persistence status')
    return int(cells[4]), int(cells[5]), cells[6:25]


def _model_list(parameters):
    return [{key: parameters[f'models[{w}].{key}'] for key in MODEL_KEYS} for w in range(4)]


def build_power_manifest(*, defines=None, tuning_reply=None, source=0, config_version=0):
    """Build evidence for one observed configuration; source/version come from CSV."""
    source = SOURCE_NAMES.get(source, source)
    if source not in range(4) or isinstance(config_version, bool) or not isinstance(config_version, int) or not 0 <= config_version <= 0xffffffff:
        raise ValueError('invalid configuration source/version')
    definitions = _defines(defines)
    revision = None
    arguments = []
    if source in (2, 3):
        if not tuning_reply:
            raise ValueError('Flash/runtime source needs an original power GET snapshot')
        revision, mode, words = _reply(tuning_reply)
        arguments = ['--tuning', str(mode), *words]
    elif tuning_reply is not None:
        raise ValueError('a power GET snapshot must be identified as Flash or runtime evidence')
    elif source == 1:
        arguments = ['--file']
    result = _run(definitions, arguments)
    paths = [ROOT/'scripts/vofa_config_export.c', ROOT/'modules/chassis_power/chassis_power.c', FIELDS_FILE,
             ROOT/'config/20_chassis_power/chassis_power_defaults.h',
             ROOT/'config/20_chassis_power/chassis_power_config.c',
             ROOT/'config/20_chassis_power/chassis_power_config.h',
             ROOT/'config/20_chassis_power/chassis_energy_config.h',
             ROOT/'config/20_chassis_power/chassis_power_model_config.h',
             ROOT/'modules/algorithm/chassis_power_transient.c',
             ROOT/'config/31_telemetry/tuning_overrides.h']
    result.update(schema=1, kind='vofa_power_configuration', config_version=config_version,
                  tuning_source=source, tuning_revision=revision,
                  models=_model_list(result['parameters']),
                  provenance={'defines': dict(definitions),
                    'source_sha256': {str(p.relative_to(ROOT)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                    'tuning_reply': tuning_reply,
                    'hardware_verified': False})
    return result


def validate_power_manifest(manifest):
    """Reconstruct every field and recompute the fingerprint in production C."""
    try:
        if manifest['schema'] != 1 or manifest['kind'] != 'vofa_power_configuration':
            return False
        parameters = manifest['parameters']
        fields = _fields()
        if set(parameters) != {key for _, key in fields} or manifest['models'] != _model_list(parameters):
            return False
        if manifest['tuning_source'] not in range(4) or not 0 <= manifest['config_version'] <= 0xffffffff:
            return False
        words = []
        for kind, name in fields:
            value = parameters[name]
            if isinstance(value, bool) or not isinstance(value, (float, int)) or not math.isfinite(value):
                return False
            if kind == 'FLOAT':
                words.append(struct.pack('>f', value).hex())
            elif value == int(value) and 0 <= value <= 0xffffffff:
                words.append(f'{int(value):08x}')
            else:
                return False
        actual = _run(arguments=['--words', *words])
        if actual['fingerprint'] != manifest['fingerprint'] or actual['power_values'] != manifest['power_values']:
            return False
        if manifest['tuning_source'] in (2, 3):
            revision, mode, reply_words = _reply(manifest['provenance']['tuning_reply'])
            expected_words = [struct.pack('>f', v).hex() for v in actual['power_values']]
            if mode != parameters['stage'] or revision != manifest['tuning_revision'] or [w.lower() for w in reply_words] != expected_words:
                return False
        return True
    except (KeyError, TypeError, ValueError, OverflowError, struct.error):
        return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source', choices=SOURCE_NAMES, default='compiled')
    parser.add_argument('--config-version', type=int, default=0)
    parser.add_argument('--tuning-reply', type=Path)
    parser.add_argument('--define', action='append', default=[], metavar='NAME=VALUE')
    args = parser.parse_args()
    definitions = dict(item.split('=', 1) for item in args.define)
    artifact = build_power_manifest(defines=definitions, source=args.source, config_version=args.config_version,
        tuning_reply=args.tuning_reply.read_text(encoding='utf-8-sig') if args.tuning_reply else None)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(artifact, indent=2)+'\n', encoding='utf-8')
    print(f"Configuration {artifact['fingerprint']:08x} saved to {args.output}; match it to the captured CSV before use.")


if __name__ == '__main__':
    main()
