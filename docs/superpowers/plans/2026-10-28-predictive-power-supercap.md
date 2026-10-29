# Predictive power and supercapacitor implementation plan

**Goal:** Implement the A-board software path of the approved architecture, with explicit evidence gates before actuator takeover.

**Architecture:** Keep the existing speed controller as the executable baseline. Add hardware-independent prediction, command history, constrained four-wheel allocation and energy trajectory checking, then run these through a separate shadow adapter. No assumed physical calibration is installed. Force control and F334 firmware remain outside this change.

**Tech stack:** C11, fixed-size state, GCC host tests, ARM GNU firmware builds, Python standard-library replay tooling.

## Constraints and delivery boundary

- Physical watts and rotor rad/s in new algorithms; CAN units only at the boundary.
- Legacy, shadow and future takeover are distinct from Stage 0–3. Default remains legacy; takeover is rejected until measured release gates are met.
- No allocation in periodic algorithms, no new CAN frame IDs or increased traffic.
- Failed submissions never become applied commands or evidence for demand growth.
- Quantized candidates must independently pass original constraints. Zero output can still be physically infeasible.
- Missing topology, parameter identification, F334 identity, capacitor calibration and timing evidence is recorded as missing, never filled with plausible values.
- Hardware stages 3–7 cannot be released from host tests. This delivery builds the executable software foundation and shadow evaluation; it does not claim calibrated MPC performance or vehicle acceptance.

## Task 1: Submission and data-quality infrastructure

Files: `modules/chassis_power/chassis_power_policy.c`, `application/chassis/chassis.c`, `modules/power_meter/power_meter_types.h`, `modules/power_meter/power_meter.c`, host tests.

- [x] Add a regression with valid motor demand but `can_submitted=false`; budget growth must be refused.
- [x] Add a speed-path cap-send failure regression; same-cycle physical/nominal ceilings must shrink and positive correction must be removed.
- [x] Route send results before publishing the application control snapshot; preserve force semantics.
- [x] Publish power-meter receive event counts so repeated/same-tick frames are distinguishable.
- [x] Run module, device and force regression suites and commit only task changes.

## Task 2: Pure prediction and allocation core

Files: new `modules/predictive_power/` headers/sources and `tests/predictive_power/`.

- [x] Define versioned parameter, measurement, final-actuation, prediction, allowance and result contracts.
- [x] Test then implement parameter validation, delayed successful-command history, finite-input rejection, wraparound and duplicate-frame handling.
- [x] Test then implement current/rotor dynamics and conservative fixed-speed power envelopes within a declared domain. Braking has no regeneration credit.
- [x] Test then implement normalized convex QCQP allocation with bounded dual search, box constraints and independent integer recheck.
- [x] Compare optimizer against independent double-precision reference cases including coupled priorities, non-monotonic power and infeasible passive losses.

## Task 3: Energy and supervision

Files: `energy_scheduler.[ch]`, `power_supervisor.[ch]`, core tests.

- [x] Implement physical energy rollout with pessimistic input/load intervals, efficiency, reserves and terminal checks; reject uncertified capacitor energy and unconfirmed first-step supply.
- [x] Produce conservative bounded rolling allowances; record that these are a feasibility baseline, not the proposed full 10-variable QP MPC.
- [x] Check original energy equations independently, allowance age/version/capability, disable/fault precedence and fallback candidates.
- [x] Keep full QP MPC and online adaptation gated behind identification and hardware evidence; publish honest status instead of placeholder success.

## Task 4: Shadow integration and reproducibility

Files: shadow adapter under `modules/chassis_power/`, build options, replay tool, commissioning documentation.

- [x] Integrate shadow calculation before final publication without altering executable legacy currents or PID state.
- [x] Record actual final bytes/submission, actual dt, versions, solver result and timing through a bounded snapshot API.
- [x] Add reproducible host replay plus a baseline manifest/evidence checklist; do not label synthetic test parameters as calibrated.
- [x] Test legacy/shadow output equivalence, bad build modes, missing model and failed-send behavior; leave force work outside user scope.
- [x] Build speed legacy/shadow firmware, inspect RAM/stack reports, request code review, resolve findings, commit and report remaining measured release gates.

## Verification commands

```powershell
make -C tests/chassis_power_module test
make -C tests/power_meter test
python tests/predictive_power/verify_speed.py
make -C tests/predictive_power test
make PROFILE=atype-vehicle -j8
```

Baseline source commit: `99f9cc2`. Existing user modifications in six files are retained and excluded from task commits.

## Delivery record (2026-10-29)

Software/shadow scope above implemented and verified. Details, exact build numbers, test commands and remaining architecture capabilities are recorded in [speed delivery notes](../../predictive-power-speed.md). Full QP MPC, model identification/online adaptation and actuator takeover are not implemented by this delivery. User clarified that subsequent work must exclude force control; no further force changes or validation were performed.
