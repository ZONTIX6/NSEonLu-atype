"""Reconstruct an analysis-only static power model from explicit whole experiments."""
import argparse
import json
from pathlib import Path
import subprocess
import sys


def markdown_report(report):
    lines = ['# Static power model analysis', '',
             '**Analysis only. No hardware release or parameter installation.**', '']
    if 'error' in report:
        lines += ['Analysis failed: '+report['error'], '']
        for name, quality in report.get('data_quality', {}).items():
            lines.append(f"- {name}: accepted {quality['accepted_rows']}/{quality['total_rows']}; rejected {quality['rejected']}")
        return '\n'.join(lines + [''])
    lines += [f"Validation passed: {report['validation_passed']}", '',
              '| Split | Candidate RMSE (W) | Baseline RMSE (W) | Upper violation (W) |',
              '|---|---:|---:|---:|']
    for split, metric in report['metrics'].items():
        c, b = metric['candidate'], metric['baseline']
        lines.append(f"| {split} | {c['rmse_w']:.6g} | {b['rmse_w']:.6g} | {c['maximum_upper_bound_violation_w']:.6g} |")
    lines += ['', 'Release blockers:', ''] + ['- '+s for s in report['release_blockers']]
    lines += ['', 'Validation failures:', ''] + ['- '+s for s in report['validation_failures']]
    lines += ['', 'Data quality:', '', '| Experiment | Total rows | Steady accepted | Rejections |',
              '|---|---:|---:|---|']
    for name, q in report['data_quality'].items():
        lines.append(f"| {name} | {q['total_rows']} | {q['accepted_rows']} | {q['rejected']} |")
    lines += ['', 'The JSON report includes each experiment, empirical margin, observed feature ranges,',
              'production C checks, source hashes, units, solver and dependency versions.',
              'Legacy missing events cannot establish submission success or sample-time alignment.', '']
    return '\n'.join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True,
                        help='must not already exist, preventing stale-result reuse')
    parser.add_argument('--build-dir', type=Path, default=Path(__file__).resolve().parents[1]/'build/static-power-replay')
    parser.add_argument('--emit-c', action='store_true', help='emit an explicitly analysis-only initializer if validation passes')
    args = parser.parse_args(argv)
    if args.output_dir.exists():
        parser.error('output directory already exists; choose a new result directory')
    model, dataset, exit_code = None, None, 2
    try:
        from power_model_data import load_manifest
        from power_model_c import Bridge
        from power_model_fit import fit_model
        dataset = load_manifest(args.manifest)
        report, model = fit_model(dataset, Bridge(args.build_dir))
        exit_code = 0 if report['validation_passed'] else 2
    except (ValueError, KeyError, TypeError, OSError, subprocess.CalledProcessError, ImportError) as error:
        message = str(error)
        if isinstance(error, subprocess.CalledProcessError):
            message += '\n'+(error.stderr or '')
        report = dict(schema=1, status='rejected', hardware_release=False,
                      validation_passed=False, error=message)
        if dataset is not None:
            report.update(manifest_sha256=dataset['manifest_sha256'],
                data_quality={e['id']: e['quality'] for e in dataset['experiments']},
                experiments=[{k: e[k] for k in ('id', 'session', 'split', 'format',
                                               'synthetic', 'source', 'sha256')}
                             for e in dataset['experiments']])
    args.output_dir.mkdir(parents=True, exist_ok=False)
    def write_json(name, data):
        (args.output_dir/name).write_text(json.dumps(data, indent=2, allow_nan=False)+'\n', encoding='utf-8')
    write_json('report.json', report)
    (args.output_dir/'report.md').write_text(markdown_report(report), encoding='utf-8')
    if model is not None:
        write_json('model.json', model)
        if args.emit_c and report['validation_passed']:
            body = '/* ANALYSIS ONLY: not approved for hardware. Static motoring command proxy. */\n{\n'
            for key, value in model['legacy_coefficients'].items():
                literal = format(value, '.9e')
                body += f'    .{key} = {literal}f,\n'
            body += '}\n'
            (args.output_dir/'candidate.inc').write_text(body, encoding='utf-8')
    print(f"{report['status']}: {args.output_dir / 'report.json'}")
    if 'error' in report:
        print(report['error'], file=sys.stderr)
    return exit_code


if __name__ == '__main__':
    raise SystemExit(main())
