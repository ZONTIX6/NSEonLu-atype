/* Synthetic host test geometry and filter parameters. */
static const ChassisVelocityFusionCalibration_s chassis_velocity_fusion_calibration = {
    .geometry = {
        .yaw_origin_b_m = { .02f, -.04f, .10f },
        .pitch_origin_yaw_m = { .10f, 0, .01f },
        .imu_origin_head_m = { .20f, 0, 0 },
        .sensor_to_head_quaternion = { 1, 0, 0, 0 },
    },
    .filter = {
        .wheel_variance = .01f,
        .accel_variance = .04f,
        .jerk_density = .25f,
        .max_velocity_residual_mps = .5f,
        .max_age_us = 40000,
        .max_skew_us = 3000,
        .max_gap_us = 60000,
    },
    .joint_zero_ecd = { 1000, 2000 },
    .joint_sign = { 1, -1 },
};
