# Supercapacitor CAN repair implementation plan

**Goal:** Repair F334 feedback transmission and A-board CAN bus-off recovery.

**Architecture:** Keep the existing standard CAN frames and 5 ms feedback period.
F334 owns register mailbox 0 for status and mailbox 1 for input power, checks the
matching empty flag before writing, and leaves a busy mailbox untouched until a
later feedback period. Enable bxCAN automatic bus-off management on both A-board
CAN controllers, including the CubeMX source configuration.

**Tech stack:** STM32F334 C++14, STM32F427 C11, STM32 HAL, host GCC and ARM GCC.

## Constraints

- User authorized fixes for findings 2 and 3; leave `CHASSIS_POWER_SOURCE_MODE`
  and all power budgets unchanged.
- Modify `RM2024-SuperCapacitorController-master`, the audited F334 project.
- Preserve pre-existing staged F334 build artifacts and do not flash hardware.
- Commit only task files in each repository after verification.

## F334 feedback

- [x] Compile the real `Core/Src/Communication.cpp` against host CAN register and
  power-state stubs in `tests/communication_feedback`.
- [x] Demonstrate failure when a 5 ms call must enqueue both `0x051` and `0x052`.
  Test literal frame payloads, each mailbox busy independently, both busy,
  subsequent recovery, the feedback period and invalid input measurements.
- [x] Replace HAL mailbox masks used as array indices with indices 0 and 1.
  Wrap status writes in the matching empty check and remove the abort request.
- [x] Run `make -C tests/communication_feedback test` and
  `make -C tests/input_power_policy test`; build ARM firmware in an isolated
  output directory with `HARDWARE_ID=201` for compile verification only.

## A-board recovery

- [x] Set `hcan1.Init.AutoBusOff` and `hcan2.Init.AutoBusOff` to `ENABLE` in
  `Src/can.c`; add `CAN1.ABOM=ENABLE` and `CAN2.ABOM=ENABLE` to
  `basic_framework.ioc` and their `IPParameters` lists.
- [x] Build the default vehicle configuration and an isolated supercapacitor
  configuration using `CHASSIS_POWER_SOURCE_MODE=1` only as a build override.
- [x] Run existing supercapacitor protocol, device, command-link and power-policy
  tests. Verify the power-source configuration file has no changes.

## Completion

- [x] Independent read-only code review; address actionable findings.
- [x] Record verification and commit each repository's task files separately.

Hardware follow-up: capture both feedback IDs at 200 Hz and induce/recover a
bus-off condition on a bench. Host tests and firmware builds do not establish
physical bus recovery or identify which firmware is installed on the robot.

## Verification record (2026-10-30)

- The new host test compiled successfully before the fix and failed on absent
  `0x052`, wrong mailbox selection, abort requests and busy-mailbox writes.
  It passes after the production fix, compiling the same `Communication.cpp`.
- Passed both F334 suites: `communication_feedback`, `input_power_policy`.
- Passed A-board suites: `super_cap_protocol`, `super_cap`, `chassis_power_link`,
  `chassis_power_module` (32 configuration combinations plus policy tests),
  and `chassis_buffer_budget`.
- ARM GCC 13.3.1 built A-board firmware in `build/can-recovery-default` and
  `build/can-recovery-supercap`; the latter uses the command-line override
  `CHASSIS_POWER_SOURCE_MODE=1`. The source default is unchanged.
- F334 ARM build completed in `build/can-feedback-check` with `HARDWARE_ID=201`.
  On Windows the Makefile needs Git's `usr/bin` on PATH and
  `SHELL=D:/gitee/Git/usr/bin/sh.exe`, `ECHO=@echo`,
  `BUILD_SUCCESS=@echo Build success` overrides. No build scripts were changed.
- F334 build reports an unused `load` variable in unchanged `PowerManager.cpp`,
  missing newlib syscall stubs and an RWX linker segment; no compiler warning
  was emitted for the modified communication file.
- Hardware was not flashed. Physical CAN feedback cadence and bus-off recovery
  remain bench verification items.

Independent read-only review found no actionable defects in the repairs or tests.
