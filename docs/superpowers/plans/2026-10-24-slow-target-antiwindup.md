# Slow Target Switching Anti-Windup Implementation Plan

> **For agentic workers:** Use test-driven development for this scoped fix and request an independent read-only code review before completion.

**Goal:** Prevent stage-3 target changes from manufacturing negative model compensation while preserving genuine overpower correction and all output guards.

**Architecture:** Keep the existing P/I gains, output saturation and positive slew. Bound the back-calculation increment so it can unwind existing integral history toward zero without crossing zero; integrate the real deadband-adjusted power error separately. Keep the transient mask outside back-calculation and preserve genuine negative history during recovery.

**Tech Stack:** C11, GCC host tests, PowerShell validation, STM32 ARM GCC and Make.

## Global Constraints

- Preserve stages 0/1/2, persistent-overpower confirmation, fast protection, total power/current limits and telemetry/Flash interfaces.
- Keep the user's existing changes in `application/chassis/chassis.c` and `modules/algorithm/chassis_spin_config.h`; exclude them from this commit, include their current settings in the rebuilt firmware.
- Use existing tools and regression suites. Do not flash or operate hardware.

## Task 1: Reproduce target switching

**Files:** `tests/chassis_power_outer_loop/test_chassis_power_outer_loop.c`, `tests/chassis_power_overpower/test_chassis_power_overpower.c`.

- [x] Add a 49 -> 65 -> 49 W target regression with constant 49 W measured power, fresh samples and valid learning gates. Check correction never becomes negative and remains within its 8 W/60 W per second bounds. Cover changing nominal budget and changing only the target.
- [x] First learn a negative integral from actual overpower, then raise the target and verify that recovery preserves negative history and does not make it more negative.
- [x] Exercise the target round trip using the real transient and slow-loop functions together.
- [x] Run `make -B -C tests/chassis_power_outer_loop CC=gcc test` and `make -B -C tests/chassis_power_overpower CC=gcc test`; confirm behavioral failures with production code unchanged.

## Task 2: Bound back-calculation

**Files:** `modules/algorithm/chassis_power_outer_loop.c`, existing stage-3 expectations in the outer-loop tests.

- [x] Clamp the back-calculation increment to `[-max(integral_w, 0), -min(integral_w, 0)]` before adding the real-error integral increment. Retain final integral/output bounds and all learning gates.
- [x] Replace the existing expectation that a positive error creates negative integral with the corresponding positive real-error increment, and retain frozen-history assertions during transient protection.
- [x] Re-run both suites and verify the existing persistent bias converges with negative slow correction.

## Task 3: Verify, rebuild and commit

**Files:** `docs/chassis-power-priority.md`, this plan; firmware outputs under ignored `build/`.

- [x] Run `scripts/validate-atype.ps1 -Python <available-python>` and examine every command result, including transient, power-model, production-module and DJI stages 0/1/2/3.
- [x] Run `scripts/build-atype.ps1 -Profile atype-vehicle`, then `scripts/check-atype-artifacts.py build/atype-vehicle`; record firmware paths, memory layout and BIN hash.
- [x] Request independent read-only review with the user requirements, exact changed files and validation evidence. Resolve important findings.
- [x] Document measured host-test before/after behavior and hardware-validation limits; stage only task files and create a local Git commit.

## Verification record

- Before the fix, the target round trip manufactured a minimum integral of -6.291 W and left -5.775 W correction after returning to 49 W, for both changing and fixed nominal budgets. The real transient/slow-loop regression reproduced the same result with no fast clamp or negative-learning permission. Recovery from genuine negative history also moved the integral in the wrong direction on the raised target.
- After the fix, the minimum integral is 0 W in both target-switch cases. Returned correction is 0 W with changing nominal budget and +0.118 W with fixed nominal budget. Genuine negative history is preserved and begins recovering in the correct direction. The existing synthetic persistent-overpower response settles at 49.344 W for a 49 W target, with -3.732 W slow correction and -4.082 W fast suppression.
- The first full validation attempt passed all 49 preceding commands, including all power suites and DJI variants, then encountered a transient Windows linker `Permission denied` when the board-configuration test reused its temporary executable. An immediate standalone rerun passed all eight board-configuration tests without any source change. The complete rerun passed 51/51 commands, including 77/77 Node tests. Logs and provenance: `build/validation/`.
- `scripts/build-atype.ps1 -Profile atype-vehicle` and `scripts/check-atype-artifacts.py build/atype-vehicle` passed after the full validation completed. Build log: `build/slow-target-antiwindup-vehicle-build.log`. Outputs: `build/atype-vehicle/basic_framework.elf`, `.hex`, `.bin`.
- Flash image: 163496 B. Static RAM: 108848 B. RAM including reserved heap/stack: 149808 B. BIN SHA256: `90f14ccb57494114c4ebca51fcfaf4f9a897f5ab925ada243815202b67ccc0f7`.
- Independent read-only review found no Critical, Important or Minor issues and assessed the code ready to commit.
- The firmware includes the user's existing PID and adaptive-spin settings; those source modifications are excluded from this commit. No flashing or hardware operation was performed. Real-vehicle power transients, recovery timing and execution timing remain hardware validation work.
