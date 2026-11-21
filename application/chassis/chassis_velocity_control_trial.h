#ifndef CHASSIS_VELOCITY_CONTROL_TRIAL_H
#define CHASSIS_VELOCITY_CONTROL_TRIAL_H

#include <stdbool.h>
#include <stdint.h>
#include "../../modules/motor/DJImotor/dji_motor_diagnostics.h"

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
    float reference_vx_mps, reference_vy_mps;
    float feedback_vx_mps, feedback_vy_mps;
} ChassisVelocityControlTrialOutput_s;

enum {
    CHASSIS_TRIAL_ENABLED = 1U << 0,
    CHASSIS_TRIAL_MODE_ALLOWED = 1U << 1,
    CHASSIS_TRIAL_SNAPSHOT_VALID = 1U << 2,
    CHASSIS_TRIAL_FUSED = 1U << 3,
    CHASSIS_TRIAL_WHEEL_VALID = 1U << 4,
    CHASSIS_TRIAL_FRESH = 1U << 5,
    CHASSIS_TRIAL_FEEDBACK_ACTIVE = 1U << 6,
    CHASSIS_TRIAL_CORRECTION_NONZERO = 1U << 7,
    CHASSIS_TRIAL_PUBLISHED = 1U << 8,
    CHASSIS_TRIAL_DRIVE_ENABLED = 1U << 9,
    CHASSIS_TRIAL_LAST_CORRECTION = 1U << 10
};

/* Updated on every chassis cycle, including cycles between VOFA captures.
 * Last correction is retained only after publishing its wheel targets. */
typedef struct {
    bool available;
    uint16_t flags;
    uint32_t tick_ms, fused_age_ms, control_version;
    uint32_t control_cycles, feedback_cycles, correction_cycles;
    float requested_vx_mps, requested_vy_mps;
    float reference_vx_mps, reference_vy_mps;
    float feedback_vx_mps, feedback_vy_mps;
    float correction_vx_mps, correction_vy_mps;
    float target_rpm[4];
    float last_correction_vx_mps, last_correction_vy_mps;
    uint32_t last_correction_tick_ms, last_correction_control_version;
    float last_correction_target_rpm[4];
    DJIVelocityFeedbackEvidence_s motor;
} ChassisVelocityControlTrace_s;

void ChassisVelocityControlTrialRecord(ChassisVelocityControlTrace_s *trace,
    const ChassisVelocityControlTrialConfig_s *config,
    const ChassisVelocityControlTrialInput_s *input,
    const ChassisVelocityControlTrialOutput_s *output,
    uint32_t now_ms, uint32_t control_version, const float target_rpm[4],
    bool published, bool drive_enabled);

/* Converts the existing motor-degree/second translation command to m/s. */
bool ChassisVelocityControlTrialCommandScale(float wheel_radius_m,
    float reduction_ratio, float *mps_per_command_unit);

/* The test limit remains active when fusion is unavailable; only the bounded
 * proportional correction falls back to the existing feed-forward path. */
bool ChassisVelocityControlTrialApply(const ChassisVelocityControlTrialConfig_s *config,
    const ChassisVelocityControlTrialInput_s *input,
    ChassisVelocityControlTrialOutput_s *output);

#endif
