# Adaptive Chassis Spin Implementation Plan

> **For agentic workers:** Execute the following tasks in this session. Use test-driven-development for the controller and requesting-code-review before the final commit.

**Goal:** Add a macro-selected spin target controller that uses spare chassis power after the existing target is tracked.

**Architecture:** A pure C controller integrates the deadbanded power error into a bounded target with asymmetric slew limits. The chassis application supplies fresh power and motor diagnostics; existing power/current controllers remain authoritative.

**Tech Stack:** C11 host tests, STM32F407 firmware, GNU Make, ARM GNU 13.3.1.

## Global Constraints

- Preserve the four pre-existing dirty files and commit only this task's changes.
- Default macro value is 0, retaining fixed target 4000.
- Adaptive limits and rates are configurable in a new independent header.
- No CAN structure or RTT column changes; both power meter compile modes must build.
- Never increase the target while the existing model/current limiter is active or motor tracking is poor.

## Task 1: Pure controller and tests

Files: create `modules/algorithm/chassis_spin.h`, `chassis_spin.c`, `chassis_spin_config.h`, and `tests/chassis_spin/Makefile`, `test_chassis_spin.c`.

Interface: `bool ChassisSpinUpdate(const ChassisSpinConfig_s *, const ChassisSpinInput_s *, float dt_s, ChassisSpinState_s *, float *target_wz)`; input carries `spin_enabled`, `control_enabled`, `feedback_valid`, `increase_allowed`, `target_power_w`, `measured_power_w`.

- [ ] Write real controller tests. A tracked 4000 target with 38 W target and 20 W load must rise after the stable delay, without exceeding the rise rate. A 60 W load must lower it; feedback loss and leaving spin must reset it to 4000.
- [ ] Run `make -C tests/chassis_spin test` and observe failure before the controller exists.
- [ ] Implement power lowpass, continuous deadband, bounded integration, settling gate, budget-drop reset and fallback. Configuration validation rejects nonfinite values and inconsistent bounds. Invalid dt includes values greater than 20 ms.
- [ ] Run host tests, including macro 0/1 behavior and invalid macro rejection.

## Task 2: Application, builds and delivery

Files: modify `application/chassis/chassis.c`, `Makefile`, `docs/chassis.md`; no changes to user-edited `robot_def.h`, power configuration, motor driver or RTT header.

- [ ] Under macro 1, build inputs from existing budget and power snapshots; require all four feedback ages at most 5 ms, power snapshot age at most 5 ms and meter age at most 20 ms. Growth additionally requires transient RUN, no braking/limiting and speed errors at most 500 rpm.
- [ ] Reset when leaving spin; select fixed/adaptive `wz` in the existing mode switch. Add algorithm source to Makefile; existing CMake source glob needs no change.
- [ ] Document switching the macro, experimental parameter values and comparable on-board measurements.
- [ ] Run relevant host tests and ARM builds with macro 0/1, power meter 0/1 and power stages 0/3.
- [ ] Request read-only code review, fix material findings, and commit only task files with `feat: add macro-selectable power-adaptive chassis spin`.
