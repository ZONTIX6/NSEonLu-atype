"""Five-term, nonnegative, command-proxy static power identification."""
import hashlib
import math
import platform
import subprocess

import numpy as np
import scipy
from scipy.optimize import lsq_linear

from power_model_c import Model, ROOT
from power_model_data import SPLITS

NAMES = ['P0', 'R', 'Km', 'K1', 'K2']
RAD_PER_RPM = math.pi/30


def features(samples):
    current = np.asarray([s['command'] for s in samples], dtype=float)/1000
    omega = np.asarray([s['rpm'] for s in samples], dtype=float)*RAD_PER_RPM
    return np.column_stack((np.ones(len(samples)), np.sum(current**2, axis=1),
        np.sum(np.maximum(0, current*omega), axis=1), np.sum(abs(omega), axis=1),
        np.sum(omega**2, axis=1)))


def legacy_coefficients(beta):
    return dict(k0=float(beta[0]/4), k1=0.0, k2=float(beta[3]*RAD_PER_RPM),
        k3=float(beta[2]*RAD_PER_RPM), k4=float(beta[1]),
        k5=float(beta[4]*RAD_PER_RPM**2), current_conversion=1000.0)


def metrics(actual, predicted, margin=0):
    error = np.asarray(predicted)-np.asarray(actual)
    under = np.maximum(0, -error)
    return dict(samples=len(error), rmse_w=float(np.sqrt(np.mean(error**2))),
        mae_w=float(np.mean(abs(error))), bias_w=float(np.mean(error)),
        p99_underestimate_w=float(np.quantile(under, .99)),
        max_underestimate_w=float(np.max(under)),
        maximum_upper_bound_violation_w=float(max(0, np.max(under)-margin)),
        upper_bound_coverage=float(np.mean(under <= margin+1e-5)),
        mean_upper_headroom_w=float(np.mean(error+margin)))


def source_identity(bridge):
    def git(*args):
        result = subprocess.run(['git', *args], cwd=ROOT, capture_output=True, text=True)
        return result.stdout.strip() if result.returncode == 0 else None
    sources = bridge.sources + [ROOT/'scripts'/name for name in
        ('rebuild-power-model.py', 'power_model_fit.py', 'power_model_data.py', 'power_model_c.py')]
    return dict(commit=git('rev-parse', 'HEAD'), dirty=bool(git('status', '--porcelain')),
        source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
        python=platform.python_version(), numpy=np.__version__, scipy=scipy.__version__,
        gcc=subprocess.run(['gcc', '--version'], capture_output=True, text=True, check=True).stdout.splitlines()[0])


def fit_model(dataset, bridge, baseline_models=None):
    experiments = dataset['experiments']
    groups = {split: [s for e in experiments if e['split'] == split for s in e['samples']]
              for split in SPLITS}
    for split, samples in groups.items():
        minimum = 10 if split == 'train' else 5
        if len(samples) < minimum:
            raise ValueError(f'{split}: insufficient steady samples ({len(samples)} < {minimum})')
    for experiment in experiments:
        if not experiment['samples']:
            raise ValueError(f"{experiment['id']}: no valid steady samples; inspect capture quality")
    matrices = {split: features(samples) for split, samples in groups.items()}
    matrix = matrices['train']
    scale = np.linalg.norm(matrix, axis=0)
    if np.any(scale == 0):
        raise ValueError('training design rank deficient: missing excitation/features')
    normalized = matrix/scale
    singular = np.linalg.svd(normalized, compute_uv=False)
    rank = int(np.linalg.matrix_rank(normalized))
    condition = float(singular[0]/singular[-1]) if singular[-1] > 0 else math.inf
    if rank < 5:
        raise ValueError(f'training design rank deficient ({rank}/5); collect independent current/speed conditions')
    if condition > 1e6:
        raise ValueError(f'training design condition {condition:g} exceeds 1e6 after column scaling')
    labels = {split: np.asarray([s['power'] for s in samples]) for split, samples in groups.items()}
    result = lsq_linear(normalized, labels['train'], bounds=(0, np.inf),
                        method='bvls', tol=1e-12, max_iter=1000)
    if not result.success or not np.all(np.isfinite(result.x)):
        raise ValueError('bounded least squares did not converge to finite coefficients')
    beta = result.x/scale
    converted = legacy_coefficients(beta)
    model_c = Model(**converted)
    current = bridge.baseline if baseline_models is None else baseline_models
    predicted = {split: np.asarray([bridge.predict(model_c, s) for s in samples])
                 for split, samples in groups.items()}
    baseline = {split: np.asarray([bridge.predict(current, s) for s in samples])
                for split, samples in groups.items()}
    max_difference = max(float(np.max(abs(predicted[split] - matrices[split]@beta))) for split in SPLITS)
    magnitude = max(float(np.max(abs(v))) for v in predicted.values())
    if not math.isfinite(max_difference) or max_difference > max(1e-4, magnitude*2e-6):
        raise ValueError('production C predictions disagree with normalized five-term model')
    margin = float(max(0, np.max(labels['calibration']-predicted['calibration'])) +
                   dataset['manifest']['instrument_error_w'])
    group_metrics = {split: dict(candidate=metrics(labels[split], predicted[split], margin),
                                  baseline=metrics(labels[split], baseline[split])) for split in SPLITS}
    experiment_metrics = {}
    for e in experiments:
        y = [s['power'] for s in e['samples']]
        experiment_metrics[e['id']] = dict(split=e['split'],
            candidate=metrics(y, [bridge.predict(model_c, s) for s in e['samples']], margin),
            baseline=metrics(y, [bridge.predict(current, s) for s in e['samples']]))
    failures = []
    for eid, scores in experiment_metrics.items():
        if scores['split'] != 'test':
            continue
        for metric in ('rmse_w', 'mae_w', 'p99_underestimate_w'):
            if scores['candidate'][metric] > scores['baseline'][metric] + 1e-4:
                failures.append(f'{eid}: candidate {metric} worse than baseline')
        if scores['candidate']['maximum_upper_bound_violation_w'] > 1e-4:
            failures.append(f'{eid}: empirical upper bound violated')
    low, high = np.min(matrix, axis=0), np.max(matrix, axis=0)
    outside = {split: int(np.count_nonzero(np.any(
        (m < low-1e-8) | (m > high+1e-8), axis=1))) for split, m in matrices.items()}
    for split in ('calibration', 'test'):
        if outside[split]:
            failures.append(f'{split}: samples outside training feature box')
    checks, infeasible, violations, guard_infeasible, search_exhausted = 0, 0, 0, 0, 0
    for sample in [s for samples in groups.values() for s in samples]:
        for budget in (1, 10, 20, 50, 80, 120):
            final, total, flags = bridge.limit(model_c, sample, budget)
            checks += 1
            guard_infeasible += bool(flags & 1)
            search_exhausted += bool(flags & 2)
            if not all(math.isfinite(v) and abs(v) <= min(15000, abs(u))+1e-5 and
                       v*u >= 0 for v, u in zip(final, sample['command'])):
                raise ValueError('production C limiter violated command bounds/sign')
            passive = bridge.predict(model_c, dict(command=[0]*4, rpm=sample['rpm']))
            if passive > budget+1e-4:
                infeasible += 1
            elif total > budget+1e-4:
                violations += 1
    if violations:
        failures.append(f'production C guard: {violations} budget violations despite passive power fitting budget')
    if search_exhausted:
        failures.append(f'production C guard exhausted its search in {search_exhausted} checks')
    blockers = ['hardware_validation_not_performed', 'steady_command_proxy_only',
                'joint_operating_domain_not_validated']
    if any(e['synthetic'] for e in experiments):
        blockers.append('synthetic_data')
    if any(e['quality']['missing_evidence'] for e in experiments):
        blockers.append('legacy_capture_missing_evidence')
    if dataset['manifest']['boundary'] == 'unknown':
        blockers.append('unknown_measurement_boundary')
        failures.append('unknown measurement boundary')
    if failures:
        blockers.append('validation_failed')
    metadata = [dict(id=e['id'], session=e['session'], split=e['split'], format=e['format'],
        synthetic=e['synthetic'], path=e['source'], sha256=e['sha256']) for e in experiments]
    model = dict(schema=1, kind='static_motoring_command_proxy', coefficient_names=NAMES,
        coefficients=beta.tolist(), legacy_coefficients=converted,
        command_scale=1000, speed_unit='rotor_rad_per_s',
        empirical_margin_w=margin, hardware_release=False, validation_passed=not failures,
        release_blockers=blockers, domain=dict(training_feature_min=low.tolist(),
            training_feature_max=high.tolist(), coverage='observed feature box, not a joint-domain guarantee'),
        manifest_sha256=dataset['manifest_sha256'])
    report = dict(schema=1, status='analyzed', hardware_release=False,
        validation_passed=not failures, validation_failures=failures, release_blockers=blockers,
        model=model, metrics=group_metrics, experiments=metadata, experiment_metrics=experiment_metrics,
        data_quality={e['id']: e['quality'] for e in experiments},
        fit=dict(rank=rank, normalized_condition=condition, maximum_condition=1e6,
            column_scale=scale.tolist(), method='scipy.optimize.lsq_linear/bvls',
            optimality=float(result.optimality), iterations=int(result.nit)),
        production_c=dict(max_prediction_difference_w=max_difference, limiter_checks=checks,
            infeasible_budgets=infeasible, budget_violations=violations,
            guard_infeasible_checks=guard_infeasible, guard_search_exhausted_checks=search_exhausted,
            limiter_config='static allocation plus two TransientUpdate samples and Stage 3 guard; safety=1, max_command=15000, error=1000 rpm each wheel, fresh feedback, zero acceleration, reset hysteresis, no startup/meter correction',
            baseline_coefficients=([{name: getattr(m, name) for name, _ in Model._fields_} for m in current]
                if isinstance(current, (tuple, list)) else
                {name: getattr(current, name) for name, _ in Model._fields_})),
        outside_training_feature_box=outside, manifest=dataset['manifest'],
        provenance=source_identity(bridge))
    return report, model
