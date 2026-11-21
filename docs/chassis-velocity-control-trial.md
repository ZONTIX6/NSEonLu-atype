# Remote-fusion velocity-control trial

The trial adds a bounded chassis-frame translation loop around the existing wheel-speed motor loops. The normal build keeps the trial off with `CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE=0`. It does not change the wheel motor feedback source, power limiter, or safety gates.

With the option enabled, translation is capped at 0.30 m/s. The proportional correction uses gain 0.25, a 0.02 m/s vector deadband, and a 0.05 m/s correction limit. The final translation vector remains capped at 0.30 m/s. There is no integrator. The correction is eligible only in no-follow and gimbal-follow modes; the existing spin command is not modified.

Fusion correction requires a valid snapshot, `CHASSIS_VELOCITY_FUSED`, valid wheel feedback, and a fused sample no older than 40 ms. Any failed gate immediately removes the correction and retains the capped feed-forward command. Thus a stale or rejected estimate cannot increase the trial speed cap or hold an old correction.

For profile 2 telemetry, I5 bit 12 indicates that the trial translation cap is active; bit 13 indicates that the fused feedback gate is active. The debugger symbols `chassis_velocity_control_trial_limited`, `chassis_velocity_control_trial_feedback_active`, `chassis_velocity_control_trial_correction_vx_mps`, and `chassis_velocity_control_trial_correction_vy_mps` expose the same state and correction.

Build a separate trial image; do not reuse the normal board build directory:

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/board-1-velocity-trial ONE_BOARD=1 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 ATYPE_BENCH_MODE=0 IMU_CAPTURE_ONLY=0 IMU_CAPTURE_CAN_TX_ENABLE=0 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE=1 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2 VOFA_OBSERVATION_PROFILE=2
```

The command only builds firmware. The trial remains unvalidated on hardware until low-speed step/stop tests, remote-IMU loss and recovery, output-age behavior, and task timing/resource checks pass. `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` remains a separate audit record and is not changed by enabling this test option.
