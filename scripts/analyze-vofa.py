"""Analyze an original VOFA CSV with engineering-owned channel definitions.

CSV files are not edited. Optional metadata/configuration artifacts are prepared
by the analyst, not by asking the operator to rename columns or convert units.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

from vofa_csv import read_capture, quality_report


def markdown_report(report):
    lines = ['# VOFA observation analysis', '',
             'No physical parameters have been installed. Hardware validation has not been performed.', '']
    if report.get('error'):
        lines += ['Analysis stopped: ' + report['error'], '']
    if 'quality' in report:
        q = report['quality']
        sequence_label = {'capture_records': 'capture records', 'send_attempts': 'send attempts',
                          'mixed_capture_records_and_send_attempts': 'mixed sequence records'}[q['sequence_kind']]
        lines += [f"Profile: {q['profile']} (schema {q['schema']}).",
            f"Capture mode: {q['capture_mode']}.",
            f"Rows: {q['total_rows']}; numeric-valid: {q['valid_rows']}; invalid: {q['invalid_rows']}.",
            f"Duplicate frames: {q['duplicate_frames']}; missing {sequence_label}: {q['missing_attempts']}; "
            f"restart/order boundaries: {q['restart_or_order_boundaries']}.", '']
        if q['schema'] in (5, 6):
            lines += [f"Unique recorded filter calls: {q['unique_filter_calls']}; missing filter calls "
                      f"between comparable recorded endpoints: {q['missing_filter_calls']}.",
                      f"FIFO overflow observed: {q['capture_overflow_observed']}. "
                      "The sticky flag does not encode the exact number of dropped records.", '']
    analysis = report.get('analysis', {})
    if 'steady' in analysis:
        q = analysis['steady']
        lines += [f"Steady meter events: {q['accepted_rows']}; exclusions: {q['rejected']}.",
                  f"Actual power configuration matched: {analysis['actual_configuration_matched']}.", '']
    if 'replay' in analysis:
        r = analysis['replay']
        lines += [f"Production C replay: {r['attempts']} calls; {r['scored_attempts']} after warmup; "
                  f"{r['exact_initial_state_segments']} segments with a known initial state.",
                  f"Recorded-output mismatches: {r['output_mismatches']}; fallback/rejections: {r['rejected_or_fallback']}.",
                  f"Stationary noise available: {analysis['stationary_noise']['available']}.", '']
    if 'power_fit' in report:
        lines += [f"Independent-trial model validation passed: {report['power_fit']['validation_passed']}.",
                  'The model artifact is an analysis candidate; it is not a hardware-release claim.', '']
    limitations = sorted(set(report.get('quality', {}).get('limitations', []) + analysis.get('limitations', [])))
    if limitations:
        lines += ['Limitations:', ''] + ['- ' + item for item in limitations] + ['']
    lines += ['The JSON report preserves source hashes, data-quality details, configuration evidence,',
              'continuous replay ranges and analysis results. Compare independent repeat trials before installation.', '']
    return '\n'.join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=Path(__file__).resolve().parents[1]/'build/vofa-analysis-c')
    parser.add_argument('--stationary', action='store_true', help='analyst confirms this capture is physically stationary')
    parser.add_argument('--configuration', type=Path, help='effective power configuration artifact, validated against capture')
    parser.add_argument('--fusion-candidate', type=Path, help='JSON with explicit positive candidate fusion_* parameters')
    parser.add_argument('--power-trials', type=Path, help='analyst-prepared JSON describing whole independent trials')
    args = parser.parse_args(argv)
    if args.output_dir.exists():
        parser.error('output directory already exists; choose a new result directory')
    report = dict(schema='vofa-analysis/1', status='rejected', hardware_validated=False)
    model, exit_code = None, 2
    try:
        capture = read_capture(args.csv)
        report['capture'] = {key: capture[key] for key in
            ('path', 'sha256', 'schema', 'profile', 'channel_count', 'time_column')}
        report['quality'] = quality_report(capture)
        if capture['schema'] == 4:
            from vofa_power import analyze_power, fit_power_trials
            if args.fusion_candidate:
                raise ValueError('fusion candidate requires schema 5 or 6')
            configuration = json.loads(args.configuration.read_text(encoding='utf-8-sig')) if args.configuration else None
            report['analysis'] = analyze_power(capture, configuration)
            if args.power_trials:
                if configuration is None:
                    raise ValueError('power fitting requires a matched effective configuration artifact')
                metadata = json.loads(args.power_trials.read_text(encoding='utf-8-sig'))
                trials = metadata['trials']
                for trial in trials:
                    trial['path'] = str((args.power_trials.resolve().parent / trial['path']).resolve())
                report['power_fit'], model = fit_power_trials(trials, configuration, args.build_dir/'power',
                    instrument_error_w=metadata['instrument_error_w'])
                if not report['power_fit']['validation_passed']:
                    raise ValueError('independent-trial power model validation failed; inspect power_fit')
        elif capture['schema'] in (5, 6):
            from vofa_fusion import FusionBridge, analyze_fusion
            if args.power_trials or args.configuration:
                raise ValueError('power configuration/trials require schema 4')
            candidate = json.loads(args.fusion_candidate.read_text(encoding='utf-8-sig')) if args.fusion_candidate else None
            bridge = FusionBridge(args.build_dir/'fusion')
            try:
                report['analysis'] = analyze_fusion(capture, bridge, args.stationary, candidate)
            finally:
                bridge.close()
        elif args.configuration or args.power_trials or args.fusion_candidate:
            raise ValueError('legacy schema lacks actual-input/configuration evidence for fitting or replay')
        report['status'], exit_code = 'analyzed', 0
    except (ValueError, KeyError, TypeError, OSError, ImportError, subprocess.CalledProcessError) as error:
        report['error'] = str(error)
        if isinstance(error, subprocess.CalledProcessError):
            report['error'] += '\n' + (error.stderr or '')
    args.output_dir.mkdir(parents=True, exist_ok=False)
    (args.output_dir/'report.json').write_text(json.dumps(report, indent=2, allow_nan=False) + '\n', encoding='utf-8')
    (args.output_dir/'report.md').write_text(markdown_report(report), encoding='utf-8')
    if model is not None:
        (args.output_dir/'model.json').write_text(json.dumps(model, indent=2, allow_nan=False) + '\n', encoding='utf-8')
    print(f"{report['status']}: {args.output_dir/'report.json'}")
    if 'error' in report:
        print(report['error'], file=sys.stderr)
    return exit_code


if __name__ == '__main__':
    raise SystemExit(main())
