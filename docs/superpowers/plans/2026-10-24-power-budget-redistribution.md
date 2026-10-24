# Power Budget Redistribution Implementation Plan

> **For agentic workers:** Use test-driven development for this scoped fix; request an independent read-only code review before completion.

**Goal:** Reuse idle wheel allocations without increasing the total model power budget.

**Architecture:** Keep the existing allocation policy, predict clamped wheel demand first, then transfer unused allocations proportionally to unsatisfied conservative demand. Keep all downstream current and power guards.

**Tech Stack:** C11, existing GCC host tests, STM32 ARM GCC, Make.

## Global Constraints

- Follow the user-approved spec `docs/superpowers/specs/2026-10-24-power-budget-redistribution-design.md`.
- Preserve current hard limits, energy gates, interfaces and telemetry formats.
- Do not stage the user's existing chassis.c or chassis_spin_config.h differences.

## Task 1: Reproduce and fix idle allocations

**Files:** `tests/power_model/test_power_model.c`, `modules/algorithm/power_model.c`.

- [x] Add a 49 W / 10,6,15,11 W behavioral regression using a linear 1 W/current-unit model. Assert original currents and 42 W total remain intact.
- [x] Run `make -B -C tests/power_model CC=gcc test`; record the third wheel failing at 12.25 rather than 15 current units.
- [x] Predict and validate clamped demand before allocation. Retain the existing initial allocation policy and transfer idle power to shortages, capped at each wheel's conservative need and the donated pool.
- [x] Test 49 W / 10,6,40,11 W: first, second and fourth demands are preserved; third gets the remaining 22 W; total remains 49 W.
- [x] Cover multiple receivers, both error-weighted branches, current hard clamp, zero-current loss and negative/nonfinite predictions. Update old tests which intentionally required stranding idle power.
- [x] Run the power model host regression and independently verify final predicted power stays within feasible budgets (2,000 signed-current / speed / budget cases with the calibrated model).

## Task 2: Integrate, verify and commit

**Files:** `tests/chassis_power_module/test_idle_power.c`, `tests/chassis_power_module/Makefile`, `docs/chassis-power-priority.md`, this plan.

- [x] Add production-module regressions for the asymmetric sufficient-budget case and a genuinely insufficient budget. Check the published `base_limited` state and final power/current constraints.
- [x] Run the existing host validation script, including buffer, adaptive/fixed/sine spin, transient guards, outer loop and DJI stage variants; investigate only failures related to this change.
- [x] Build the default A-board vehicle firmware and run `scripts/check-atype-artifacts.py` on its output.
- [x] Request an independent read-only review with the approved spec and exact changed files; address important findings.
- [x] Document reproduction, numerical before/after results and validation limits. Review/stage only task files and create a local Git commit.

## Verification record

- Observed the new minimal regression fail against the unchanged allocator: third wheel 12.25 W / requested 15 W, final total 39.25 W / requested 42 W. It passes with redistribution: 15 W and 42 W respectively.
- Insufficient 49 W budget with 67 W total demand still limits the third wheel to 22 W. Multiple receivers and both error-weighted policies pass. Existing single-wheel quadratic/linear scaling fixtures now use genuinely insufficient total budgets, preserving their current-solver checks.
- The 2,000-case calibrated-model sweep passes aggregate allocation, final model power, signed-current magnitude and hard-current constraints. Production-module stage 0/1/2/3 tests publish `base_limited=false` for sufficient asymmetric demand and `true` for genuine limiting.
- `scripts/validate-atype.ps1 -Python <bundled-python>`: 51/51 commands passed, including all host/portable suites, DJI stages 0/1/2/3 and meter-disabled variant, board configuration, architecture and Node tests. Logs: `build/validation/`.
- `make -j8 PROFILE=atype-vehicle` and `scripts/check-atype-artifacts.py build/atype-vehicle` passed. Flash 163456 B; static RAM 108848 B; RAM including reserved heap/stack 149808 B. Build log: `build/power-redistribution-vehicle-build.log`.
- BIN SHA256: `1680630b894bd0d84a4a048f0257f14d6afa2ae0e2ec4895f52d207b37dcc695`.
- Independent read-only review: Ready to commit; no Critical, Important or Minor findings.
- No flashing or real-vehicle operation was performed. Firmware includes the user's existing PID/spin configuration; those two source differences are excluded from the commit. A-board remeasurement and task timing remain hardware validation work.
