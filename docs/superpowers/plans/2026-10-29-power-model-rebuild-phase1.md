# Power Model Rebuild Phase 1 Implementation Plan

> **For agentic workers:** Execute the tasks below with independent implementation and review. Use checkbox steps to record verification; the user has approved implementation and local commits.

**Goal:** Correct failed-submission history, share the production speed-model parameters, and deliver reproducible offline static identification without changing calibrated coefficients.

**Architecture:** Keep the existing speed backend. A small shared parameter header feeds the speed application, C tests and offline C bridge. The offline tool validates explicit experiment manifests, imports legacy captures, fits five nonnegative features, and verifies candidates with the production C predictor/limiter.

**Tech Stack:** C11/GCC, Arm GNU, Python 3.12+, NumPy 2.2.6, SciPy 1.15.3, unittest. Python dependencies are isolated in `build/power-model-venv`.

## Global Constraints

- Scope is the approved first batch of `../specs/2026-10-29-chassis-power-model-rebuild-design.md`.
- Preserve existing uncommitted PID, spin, force comments and generated-index changes. Stage only this task's hunks.
- No flashing, motor activation, force-backend change, F334 change or predictive takeover.
- Retain current coefficients bit-for-bit and `current_conversion=1000`; label this as command normalization, not amperes.
- Legacy captures and synthetic data remain analysis-only; every exported model has `hardware_release=false`.
- Fit/train, margin calibration and final test groups are explicit disjoint whole experiments; no random row splitting.

## Task 1: Failed submission history

**Files:** `modules/chassis_power/chassis_power_limiter.c`, `tests/chassis_power_module/test_submission_history.c`, `tests/chassis_power_module/Makefile`.

**Interfaces:** Existing `ChassisPowerLimit`, `ChassisPowerRecordFinal`, snapshots and `ChassisPowerTransientRecordSent`; no new public interface required.

- [x] Add a real module regression: one feedback frame, 2 ms cycles, 12000 demand and ample power; first candidate is at most 6000. Record failure, request again and assert no cumulative growth. Cover multiple failures, successful recovery, opposite sign, failed stop and timestamp wrap.
- [x] Run the new executable before the fix and observe the failed-candidate history assertion.
- [x] Change only the successful history write:

```c
if (i->can_submitted)
    ChassisPowerTransientRecordSent(&p->transient, current);
```

- [x] Re-run module/transient/integration tests, preserving failed bytes in diagnostics. Aggregate false also covers partial-group submission failure.

## Task 2: Shared production model

**Files:** create `modules/algorithm/chassis_power_model_config.h`; change `application/chassis/chassis.c`, `tests/power_model/test_power_model.c`, its Makefile and relevant production-coefficient fixtures only.

**Interfaces:** Pure-data macro `CHASSIS_SPEED_POWER_MODEL_INITIALIZER` expands to a `MotorPowerModelConfig_s` initializer. Header includes no application or HAL dependencies.

- [x] Add tests consuming this initializer and asserting meaningful real-model behavior: finite output over the configured command/speed range, low-speed nonmonotonicity, feasible roots, insufficient passive-loss budget and integer-current power checks. Keep historical model tests named as historical.
- [x] Confirm tests cannot build without the shared definition, then extract unchanged production values into the header and use it in the application and tests.

```c
static const MotorPowerModelConfig_s m3508_power_model =
    CHASSIS_SPEED_POWER_MODEL_INITIALIZER;
```

- [x] Compile/run the actual production-C model tests. Avoid changing force configuration or user PID hunks.

## Task 3: Offline data and fitting

**Files:** create `scripts/power_model_data.py`, `scripts/power_model_fit.py`, `scripts/power_model_c.py`, `scripts/rebuild-power-model.py`, `scripts/power-model-requirements.txt`, `tests/power_model_fit/test_rebuild.py`, `tests/power_model_fit/test_data.py`, `tests/power_model_fit/fixtures.py` and `tests/power_model_fit/model_bridge.c`.

**Interfaces:** `load_manifest(path)` returns validated metadata and experiment samples; `fit_model(dataset, bridge)` returns JSON-serializable report/model; CLI takes `--manifest`, `--output-dir`, optional `--emit-c` and `--build-dir`.

Manifest schema 1 declares `boundary` (`chassis_load` or `unknown`), units (`dji_command_raw`, `rotor_rpm`, `W`), instrument error, steady-window thresholds, and experiments with unique IDs, independent `session` identifiers, split (`train`, `calibration`, `test`), file, format (`legacy26`, `cpc1`, `aligned_static`), and `synthetic`. Reused files/content or sessions across splits are rejected. Each file represents a complete experiment; hashes identify exact inputs. Unit mismatch and unsupported schemas fail before fitting.

`aligned_static` input columns: `tick_ms,meter_sequence,submitted,meter_online,meter_age_ms,enabled_mask,online_mask,load_w,command0..3,rpm0..3`. This is an offline data contract, not a claim that default firmware emits it. Legacy adapters preserve missing-evidence flags and never fabricate submission success.

- [x] Write tests first for known five-feature parameter recovery and production-C consistency, rank deficiency, invalid units/NaN, mixed braking, stale/duplicate/out-of-order/wrapped events, all-wheel health, stable idle, disjoint split enforcement, synthetic release rejection and CLI output/exit statuses.
- [x] Run against the missing tool to establish failure; then implement input validation, adapters and rejection counters. Invalid events break stable windows; normal duplicate measurements maintain control-state continuity without adding fit samples. Control/event high-water marks cannot rewind.
- [x] Use entire stable windows with configured duration and maximum command/rpm ranges. Keep zero-command stationary baseline; reject individual-wheel braking and ambiguous moving zero-command states. Don't discard high-power rows by residual magnitude.
- [x] Form `[1, sum(i²), sum(max(0,i*omega)), sum(abs(omega)), sum(omega²)]` with command/1000. Scale columns, compute SVD rank/condition and solve nonnegative least squares with `lsq_linear`. Reject deficient or ill-conditioned data rather than inventing coefficients.
- [x] Fit only training rows, set empirical margin from calibration positive residual maximum plus declared instrument error, and report test RMSE/MAE/bias/underestimate tail/upper-bound violations per experiment and split. Compare current production model on identical rows.
- [x] Compile C bridge from `power_model.c` and shared parameter header. Verify converted coefficients and predictor outputs, run limiter constraints at representative budgets, explicitly report infeasible passive-power budgets. No parameter installation.
- [x] Export `model.json`, `report.json`, `report.md`; optional `candidate.inc` clearly marked analysis-only. Include file/source hashes, git identity/dirty state, dependency versions, normalized units, parameter domain, quality reasons and `hardware_release=false`. Failed analysis must return nonzero and cannot leave a stale successful candidate.

## Task 4: Documentation, review and verification

**Files:** create `docs/power-model-rebuild.md`; update approved spec status and this checklist. Do not rewrite unrelated generated indexes.

- [x] Document isolated environment setup, manifest contract, both legacy adapters, example synthetic workflow, all outputs/exit codes, steady-window selection and real-data collection requirements.
- [x] Run targeted C model/module/transient/outer-loop/DJI tests, predictive C/Python tests and legacy/shadow final-command comparison.
- [x] Build default speed and shadow firmware into new independent directories with `make PROFILE=atype-vehicle ...`.
- [x] Obtain independent review, fix actionable findings and rerun affected checks only.
- [x] Verify diff/staged hunks exclude user's changes, commit the validated first batch, and report commit and remaining real-data dependency.

## Execution result

Completed with independent review; the exact-scope verification snapshot passed 32 offline tests, the targeted C suites, both firmware builds and the 7000-command legacy/shadow comparison. See `../verification/2026-10-29-power-model-rebuild-phase1.md`. No physical calibration or hardware deployment was performed.
