# Chassis direct force control implementation plan

**Goal:** Implement the approved 2026-10-25 direct-force design while retaining the default speed firmware and its wire layout.

**Architecture:** Select the application and adapter at build time. Pure C modules own calibrated geometry, force shaping, power-driven spin torque, bounded allocation and freshness/state checks. The existing DJI driver owns the only motor CAN sender; the existing energy policy owns referee and capacitor budgets.

**Tech stack:** C11, STM32F427/FreeRTOS, GCC/Make/CMake, native C and Python contract tests.

**Packaging update:** Per the user's implementation review, all pure force algorithms and the intent codec live in `modules/chassis_force/`. Vehicle configuration and build constraints live in `application/board_chassis_force.[ch]`. `application/chassis/` contains only the force application wiring in addition to the existing speed application. File names below describe responsibilities; use these final module boundaries when locating them.

## Constraints

- Default backend SPEED=0; FORCE=1 requires OMNI_X, stage 3, and valid measured calibration. Uncalibrated force builds stay locked.
- No chassis speed PID, speed-reference conversion or speed-error state in force control.
- Control/source/yaw age <=10 ms, wheel age <=5 ms, dt in (0,5 ms]; first cycle establishes time with zero output.
- Separate input/load power, verified capacitor confirmation, bounded offered demand, and final integer-current validation.
- Preserve existing uncommitted chassis PID/spin changes and unrelated documentation changes.
- Hardware calibration, flashing, resource timing and driving acceptance require the actual vehicle; report those separately from software checks.

## Work packages

1. Configuration, geometry, protocol and allocator.
   - Create `chassis_control_config.h`, `chassis_force_config.[ch]`, `chassis_force_geometry.[ch]`, `chassis_force_allocator.[ch]`, `application/chassis/chassis_force_intent.[ch]`.
   - Interfaces: `ChassisForceGeometryInit`, `ChassisForceToCurrent`, `ChassisForceFromCurrent`, `ChassisForceAllocate`, `ChassisForceIntentEncode/Decode/Accept`.
   - Write native tests before implementation: wrench roundtrip, H pseudo-inverse, radius/K_out units, invalid calibration, 44-byte golden layout, replay/age/wrap handling, independently scanned nonmonotonic power bounds.
   - Verify with `make -C tests/chassis_force test`; initially expect missing interfaces, then all assertions pass at O0 and optimized no-fast-math.
2. Command, spin-power and runtime safety.
   - Create `chassis_force_command.[ch]`, `chassis_force_spin_power.[ch]`, `chassis_force_control.[ch]`.
   - Interfaces consume immutable intent, feedback, budget and final submission records; expose typed force snapshots.
   - Test identical input at different non-limited speeds, radial shaping, G-frame slew, direction reversal, one-shot brake, meter duplicate/stale/recovery, final torque projection, overspeed and fault recovery.
3. Energy-policy integration.
   - Extend `modules/chassis_power/*` with typed force operating/offered demand, physical target, frozen fast budget and candidate recording.
   - Preserve existing speed API. Validate 051/052 separately at 20/21 ms; record 061 result and only confirm a target with a post-send frame.
   - Test bounded offers, failed commands, separate input/load accounting, raw referee fields and budget conversion once. Run existing power/buffer/supercap regression tests.
4. Driver and application assembly.
   - Add generic group current ownership, optional `prepare_current` and pre-send validation to `dji_motor.[ch]`; add `dji_force_adapter.[ch]` and `chassis_force_app.c`.
   - Select separate force-v1 pub/sub and explicit dual-board payload from raw inputs in `robot_cmd.c`.
   - Exclude force wheels from autotune ownership. Copy real feedback outside PID branches. Assert PID call count=0 for force wheels, other-motor behavior and byte-accurate final CAN records.
5. Build selection, telemetry, documentation and review.
   - Add four macros to `config/atype-options.txt`; resolve backend=0 before source filtering in Make/CMake. Compile numeric safety sources with trailing `-fno-fast-math`.
   - Publish a separate force-v1 diagnostic stream; expose model wrench, torque and speed limits without fictitious speed targets.
   - Run native suites and Make/CMake Debug/Release backend/energy/exit matrices, reject illegal combinations, inspect symbols and compile options.
   - Use requesting-code-review for independent review, resolve significant findings, document software evidence and outstanding hardware checks, commit only task changes.
