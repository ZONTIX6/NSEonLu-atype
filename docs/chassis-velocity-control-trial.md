# Remote-fusion velocity-control trial

The trial adds a bounded chassis-frame translation loop around the existing wheel-speed motor loops. The normal build keeps the trial off with `CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE=0`. It does not change the wheel motor feedback source, power limiter, or safety gates.

With the option enabled, translation has a 2.70 m/s ceiling. This exceeds the current full-scale two-axis RC command (about 2.61 m/s with the configured 40-unit gain, 76 mm wheel radius, and 19:1 reduction), so normal stick range is preserved. The proportional correction uses gain 0.25, a 0.02 m/s vector deadband, and a 0.05 m/s correction limit. The final translation vector remains capped at 2.70 m/s. There is no integrator. The correction is eligible in no-follow, gimbal-follow, and spin modes. It changes translation only; spin angular-speed generation and the existing wheel motor loops remain unchanged.

In spin mode, feed-forward retains the existing phase lead. The feedback reference uses the same operator command and static trim with zero phase lead, so intentional feed-forward compensation is not counted as velocity error. The historical fused chassis velocity is rotated into the current chassis axes using `wheel_wz_radps * sample_age_s` with the public wheel-yaw sign. This assumes approximately constant angular speed over the sample age; it does not predict translational acceleration or remove the filter's dynamic response lag. The existing 40 ms freshness gate still applies. Missing/nonfinite spin references or yaw rate remove the correction immediately.

Fusion correction requires a valid snapshot, `CHASSIS_VELOCITY_FUSED`, valid wheel feedback, and a fused sample no older than 40 ms. Any failed gate immediately removes the correction and retains the original feed-forward command, subject to the 2.70 m/s ceiling. Thus a stale or rejected estimate cannot hold an old correction.

For profile 2 telemetry, I5 bit 12 indicates that the trial translation cap is active; bit 13 indicates that the fused feedback gate is active at the captured cycle. The debugger symbols `chassis_velocity_control_trial_limited`, `chassis_velocity_control_trial_feedback_active`, `chassis_velocity_control_trial_correction_vx_mps`, and `chassis_velocity_control_trial_correction_vy_mps` expose the same state and correction. A zero correction is normal inside the deadband even when feedback is active. Event-FIFO rows are primarily captured on filter updates, not every control cycle; a zero bit 13 in all saved rows does not exclude feedback between those rows. Check the live variables together with `chassis_cmd_recv.chassis_mode` (1 = spin).

Build a separate trial image; do not reuse the normal board build directory:

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/board-1-spin-kalman ONE_BOARD=1 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 ATYPE_BENCH_MODE=0 IMU_CAPTURE_ONLY=0 IMU_CAPTURE_CAN_TX_ENABLE=0 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE=1 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2 VOFA_OBSERVATION_PROFILE=2
```

The command only builds firmware. The trial remains unvalidated on hardware until low-speed step/stop tests, remote-IMU loss and recovery, output-age behavior, and task timing/resource checks pass. `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` remains a separate audit record and is not changed by enabling this test option.
