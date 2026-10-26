# Direct force controller

Pure C, fixed-storage force backend. This module has no HAL, FreeRTOS, CAN,
application configuration or dynamic allocation dependency.

- `chassis_force_intent.h`: normalized input, local publication envelope and explicit 44-byte force-v1 codec.
- `chassis_force_config.h`: SI calibration schema, checksum and parameter validation. Zero physical defaults lock actuation.
- `chassis_force_control.h`: initialize once, step with one complete command/feedback/budget snapshot, validate integer current, then record the actual CAN payload and submission result.
- Geometry, shaping, spin power regulation and bounded allocation are module internals. All currents use final CAN signs; wrench reconstructed from sent current is a model estimate.

Application boundaries:

- `application/board/board_chassis_force.[ch]` owns vehicle calibration, energy parameters and legal firmware combinations.
- `application/chassis/chassis_force_app.c` wires messages, safety, policy and feedback into the existing robot task.
- `modules/chassis_power/chassis_power_force.[ch]` freezes the shared power budget once per motor cycle and records complete force results.
- `modules/motor/DJImotor/dji_force_adapter.[ch]` owns the four registered wheels, injects clocks/feedback, bypasses their software PID and gates the shared CAN enqueue.
- `modules/telemetry/chassis_force_telemetry.[ch]` emits a separate force-v1 stream from the slow USB task.

`CHASSIS_CONTROL_MODE` defaults to SPEED=0. FORCE=1 selects this backend at build time; it never switches into the old speed loop at runtime. In FORCE, FOLLOW has the same gimbal-oriented translation behavior as NO_FOLLOW and applies no automatic yaw torque.

Run `make -C tests/chassis_force test` and `make -C tests/vehicle_motion test_force`.
Hardware torque calibration, wiring, current limits, period/stack measurements and driving acceptance remain mandatory before enabling a physical vehicle.
