# Shadow J-Scope implementation plan

**Goal:** Display shadow diagnostics live in J-Scope using RTT channel 2.

**Architecture:** An opt-in low-priority observer reads the published shadow snapshot and emits one complete nonblocking binary packet. Channel 0 logs remain unchanged; channel 1 keeps its data format but is not advertised as JScope while this feature is enabled, because J-Scope selects the first matching channel.

**Constraints:** Default disabled, 50 Hz, configurable 1–100 Hz, selected wheel 0–3. At most twelve fields including a target timestamp. No control changes, no hardware operation. Invalid values carry explicit flags; failed writes count dropped packets. Default build has no added task or buffer.

- [x] Add failing host tests for packet layout, units, validity, stale/duplicate snapshots, rate limiting, wraparound, failed initialization/writes and channel ownership.
- [x] Implement application-level shadow reader/RTT writer and configuration; reuse the low-priority observation task independently of USB.
- [x] Verify default and enabled firmware builds, existing J-Scope and shadow regressions, and architecture boundaries.
- [x] Document field order, configuration and J-Scope setup; complete independent review.

**Files:** `application/runtime/shadow_jscope.*`, `modules/jscope/shadow_scope_config.h`, `application/runtime/robot_task.c`, `modules/jscope/jscope.c`, `modules/chassis_power/chassis_power_shadow.*`, Make/CMake configuration, `tests/shadow_jscope`, and `docs/shadow-jscope.md`.

## Validation

- Host tests pass for selected LF/RB wheels, disabled linking without transport, full 48-byte layout, invalid/paused estimates, time wrap, stale/duplicate snapshots and failed RTT writes. Existing J-Scope tests cover both advertised names and preserved channel 1 payload/interrupt behavior.
- Predictive core, shadow isolation, KF and timeout suites pass. Measurement-valid regression covers disabled, exactly 20 ms fresh, 21 ms stale, NaN and offline meter inputs.
- Architecture: ten checks pass; zero boundary violations.
- Independent reviewer found a low-priority preemption race between the caller clock and snapshot retrieval. Added a failing regression, then fixed it by reading freshness time after snapshot copy; re-review has no remaining actionable findings.
- Make builds pass: `build/shadow-jscope-default`, `build/shadow-jscope`, `build/shadow-jscope-kf-check`. Last uses synthetic KF configuration only for compilation and disables USB to verify independent task assembly. CMake source registration updated, not built.
- No firmware flashing, physical motor operation or J-Scope hardware validation performed. No claim of measured board timing or loss rate.
