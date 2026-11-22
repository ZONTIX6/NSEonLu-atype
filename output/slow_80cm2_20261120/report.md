# VOFA observation analysis

No physical parameters have been installed. Hardware validation has not been performed.

Profile: kalman_gimbal_control_turn_count (schema 8).
Capture mode: event_fifo.
Rows: 2843; numeric-valid: 2843; invalid: 0.
Duplicate frames: 0; missing capture records: 183; restart/order boundaries: 1.

Unique recorded filter calls: 2811; missing filter calls between comparable recorded endpoints: 183.
FIFO overflow observed: True. The sticky flag does not encode the exact number of dropped records.

Production C replay: 2811 calls; 929 after warmup; 1 segments with a known initial state.
Recorded-output mismatches: 0; fallback/rejections: 0.
Stationary noise available: False.

Limitations:

- capture_fifo_overflow_observed_exact_drop_count_not_recorded
- hardware_validation_not_performed
- no_parameter_optimization_claim
- restart_and_out_of_order_cannot_always_be_distinguished
- unknown_initial_state_replay_is_warmed_up_not_exact
- unknown_sensor_latency_not_identifiable
- wheel_odometry_is_not_independent_velocity_truth

The JSON report preserves source hashes, data-quality details, configuration evidence,
continuous replay ranges and analysis results. Compare independent repeat trials before installation.
