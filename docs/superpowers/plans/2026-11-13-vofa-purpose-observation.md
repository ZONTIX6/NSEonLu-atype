# VOFA Purpose Observation Implementation Plan

> **For agentic workers:** Implement the tasks below with focused implementers and independent review. Steps use checkboxes to track the work in this chat.

**Goal:** Implement the power/supercapacitor and Kalman observation profiles and analyze unchanged VOFA CSV exports.

**Architecture:** Capture control-cycle evidence at its producer; encode fixed, versioned JustFloat layouts from immutable snapshots. Share channel definitions with the host parser. Replay only continuous recorded filter inputs through the production C filter.

**Tech Stack:** STM32F427, C11, Make/CMake, PowerShell, Python unittest, existing host GCC tests.

## Global Constraints

- UART7, 230400 / 8N1, 20 ms target slots, at most 96 channels per new profile.
- Profile 1 is power, profile 2 is Kalman; default daily board configuration is Kalman. Mode 0/1 and explicit legacy profile 0 retain their existing layouts.
- New schemas 4/5 use physical SI units and raw DJI command counts; schema 2/3 keep legacy units. Wheel order is LF/RF/LB/RB.
- Split wrapping uint32 clocks, identities and counters into high/low 16-bit fields. Unknown ages are -1. Validity controls interpretation of zero values.
- Preserve asynchronous buffer ownership, whole-frame busy skips and delimiter collision handling.
- Keep fusion read-only. Do not change physical tuning constants without vehicle data. Do not flash or push.
- Preserve unrelated untracked plans, reference-data and shadow-observations.

## Task 1: Producer evidence

**Files:** `modules/chassis_power/chassis_power_types.h`, `chassis_power_limiter.c`, `chassis_power.c`, `chassis_power_policy.c`, `chassis_power_telemetry.c`, `modules/super_cap/super_cap.c`, `super_cap_types.h`, `application/chassis/chassis_tuning.c`, `chassis_init.c`; matching `tests/chassis_power_module` and `tests/super_cap` tests.

**Interfaces:** Extend `ChassisPowerSnapshot_s` with `cycle_control`, `cycle_config`, submitted/successful CAN group masks. Extend `SuperCapSnapshot_s` with actual last command, attempt identity/time and submission result. Expose a canonical fingerprint of effective configuration, excluding structure padding.

- [x] Add failing assertions that advancing policy after a motor cycle cannot alter that cycle's adopted budget/configuration, and that a failed capacitor send preserves the attempted target and failure result.
- [x] Copy the complete adopted control snapshot in `ChassisPowerLimit`; publish it with final integer CAN commands. Capture effective config values/source at the same point. Preserve policy's independent version/time/configuration.
- [x] Record capacitor command attempts at the send boundary, including failed sends. Submission is never called execution confirmation.
- [x] Run `make -C tests/chassis_power_module test` and `make -C tests/super_cap test`, plus affected command/history regression targets.

## Task 2: Actual filter attempts

**Files:** `modules/algorithm/chassis_remote_fusion.h`, `chassis_remote_fusion.c`, `tests/chassis_remote_fusion/test_chassis_remote_fusion.c`.

**Interfaces:** `ChassisRemoteFusionAttempt_s` contains the exact `ChassisVelocityFusionInput_s`, `ChassisVelocityFusionOutput_s`, full effective remote configuration, attempt sequence, receiver call/middle times, reset count before the call, timing spans and initialization state. Expose it as `velocity.remote.attempt`.

- [x] Add failing tests for a nonidentity navigation rotation, rejected filter input, duplicate source sample and pipeline reset.
- [x] Immediately before the production call, retain the input/configuration and advance attempt sequence; immediately after it retain the result. Store outside `held`, so pipeline reset cannot erase evidence.

```c
state->attempt.input = input;
state->attempt.config = *config;
ChassisVelocityFusionUpdate(&state->filter, &config->filter, &input, &filtered);
state->attempt.output = filtered;
```

- [x] Run `make -C tests/chassis_remote_fusion test` and observer/fusion regression tests.

## Task 3: Profiles, layouts and transport

**Files:** `config/32_capture/vofa_schema.json`, generated `application/runtime/vofa_schema.h`, new `vofa_observation.[ch]`, existing `vofa_telemetry.[ch]`, `robot_telemetry.c`, `config/90_board/board_config.h`, `config/00_flash/board_flash_config.h`, `config/99_build/atype-options.txt`, `scripts/build-board.ps1`, `Makefile`, `CMakeLists.txt`, `tests/vofa_telemetry`, `tests/board_flash`.

**Interfaces:** Keep legacy sender intact; add a dedicated observation sender accepting USB target metadata, cycle/velocity/capacitor/referee snapshots and receiver `now_ms`. JSON is the authoritative ordered channel contract; generated C indexes are checked against it.

- [x] Add a failing board-selection test for `VOFA_OBSERVATION_PROFILE=1/2` and serializer tests with literal physical values and final command evidence.
- [x] Add the profile option and explicit validation. Legacy tests select profile 0. New profiles require mode 2; legacy mode selection remains usable.
- [x] Encode each channel by named index. Reject nonfinite groups by clearing their validity bits and emitting finite placeholders.

```c
_Static_assert((96U + 1U) * 4U * 10U * 1000U <
               BOARD_UART7_BAUDRATE * 20U, "observation wire budget");
```

- [x] Test 20 ms boundaries, jitter, uint32 wrap, busy/error sequence gaps, unchanged active buffer and false frame tails at every byte alignment. Verify legacy formats and source freshness.

## Task 4: Unmodified CSV and production replay

**Files:** new `scripts/vofa_*.py`, `scripts/vofa-*.py`, `tests/vofa_analysis`; reuse `scripts/power_model_data.py`, `power_model_fit.py`, `power_model_c.py` as needed.

**Interfaces:** Host reader consumes the JSON layout; supports schema 4/5 and explicitly described schema 2/3. CLI accepts the original CSV path, emits quality/diagnostic JSON, and never silently guesses ambiguous mappings.

- [x] Write failing CSV fixtures for default/renamed headers, extra time column, missing columns, mixed schema, corrupt values, duplicates, clock rollover and restarts.
- [x] Detect one unique contiguous channel layout from schema/count/value constraints; validate integer metadata with float32 collision tolerance and keep physical floats unscaled for new schemas.
- [x] Build quality summaries and usable intervals before any model work. Deduplicate filter attempts, split on gaps/resets/config changes and prewarm new segments.
- [x] Compile/load the production `chassis_velocity_fusion.c` for replay, including recorded rejection inputs and effective configuration. Report stationary noise and segment sufficiency without claiming ground-truth speed accuracy.
- [x] Adapt eligible power samples to existing steady-state filtering/deduplication and production C comparison. Require independently identified trials and matching effective configuration before candidate fitting; do not substitute source defaults for Flash/runtime settings.
- [x] Run `python -m unittest discover -s tests/vofa_analysis -v` and affected power-model tests.

## Task 5: Integration, instructions and review

**Files:** `docs/vofa-telemetry.md`, `config/README.md`, this plan and the design status.

- [x] Explain the two daily profile choices, unchanged CSV export, provenance limits, channel units/validity, and compact separate acquisition action tables.
- [x] Run host regressions and separate main-board firmware builds for both profiles; inspect bandwidth and compiler flags. Verify Make and CMake option propagation where tools are available.
- [x] Independently review producer timing, wire contract, parsing ambiguity, replay segmentation and configuration provenance; resolve actionable findings.
- [x] Record actual validation results and remaining hardware-only checks. Create a local commit containing only this task's files after successful checks.

Hardware acceptance remains a later vehicle run: CPU/task latency, wireless throughput and attempt gaps, measured response, independent before/after trials. Host tests and firmware builds do not establish these results.

Validation details and hardware-only limits: [verification record](../verification/2026-11-13-vofa-purpose-observation.md).
