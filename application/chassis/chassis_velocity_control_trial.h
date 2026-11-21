#ifndef CHASSIS_VELOCITY_CONTROL_TRIAL_H
#define CHASSIS_VELOCITY_CONTROL_TRIAL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float max_speed_mps;
    float proportional_gain;
    float max_correction_mps;
    float deadband_mps;
    uint32_t max_sample_age_ms;
} ChassisVelocityControlTrialConfig_s;

typedef struct {
    bool enabled;
    bool speed_limit_allowed;
    bool feedback_allowed;
    bool spin_mode;
    bool fused_snapshot_valid;
    bool fused_status;
    bool wheel_valid;
    uint32_t now_ms;
    uint32_t fused_tick_ms;
    float requested_vx_mps, requested_vy_mps;
    float fused_vx_mps, fused_vy_mps;
    /* Spin reference excludes feed-forward phase lead, in current chassis axes. */
    float reference_vx_mps, reference_vy_mps;
    /* Public wheel yaw sign: historical velocity maps to current axes by +wz*age. */
    float wheel_wz_radps;
} ChassisVelocityControlTrialInput_s;

typedef struct {
    bool command_limited;
    bool feedback_active;
    float vx_mps, vy_mps;
    float correction_vx_mps, correction_vy_mps;
} ChassisVelocityControlTrialOutput_s;

/* Converts the existing motor-degree/second translation command to m/s. */
bool ChassisVelocityControlTrialCommandScale(float wheel_radius_m,
    float reduction_ratio, float *mps_per_command_unit);

/* The test limit remains active when fusion is unavailable; only the bounded
 * proportional correction falls back to the existing feed-forward path. */
bool ChassisVelocityControlTrialApply(const ChassisVelocityControlTrialConfig_s *config,
    const ChassisVelocityControlTrialInput_s *input,
    ChassisVelocityControlTrialOutput_s *output);

#endif
