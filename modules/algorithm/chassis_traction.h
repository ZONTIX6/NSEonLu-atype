#ifndef CHASSIS_TRACTION_H
#define CHASSIS_TRACTION_H

#include "chassis_velocity.h"

typedef struct {
    float vx_mps, vy_mps, wz_radps;
    uint32_t tick_ms, gyro_tick_ms;
    bool valid; /* FUSED translation and independent fixed-chassis gyro only. */
} ChassisTractionReference_s;

typedef struct {
    ChassisVelocityConfig_s geometry;
    unsigned mode; /* 0 off, 1 shadow, 2 torque reduction, 3 bounded spin braking. */
    float absolute_slip_mps, relative_slip, min_wheel_mps, min_ground_mps;
    float release_ratio, recovery_s, accel_allowance_mps2, yaw_accel_allowance_radps2;
    float brake_gain, max_brake_current;
    uint32_t enter_ms, release_ms, max_reference_age_ms, max_wheel_age_ms;
} ChassisTractionConfig_s;

typedef struct {
    ChassisTractionReference_s reference;
    float raw_current[4], actual_rpm[4], max_current[4];
    uint32_t feedback_tick_ms[4], feedback_sequence[4], now_ms;
    uint8_t online_mask, enabled_mask;
    float dt_s;
    bool drive_enabled, targets_match;
} ChassisTractionInput_s;

typedef enum {
    CHASSIS_TRACTION_READY,
    CHASSIS_TRACTION_OFF,
    CHASSIS_TRACTION_BAD_CONFIG,
    CHASSIS_TRACTION_DRIVE_GATED,
    CHASSIS_TRACTION_NO_REFERENCE,
    CHASSIS_TRACTION_STALE_REFERENCE,
    CHASSIS_TRACTION_BAD_WHEELS,
    CHASSIS_TRACTION_BAD_TIMING
} ChassisTractionReason_e;

typedef struct {
    float current[4], proposed_current[4];
    float actual_mps[4], expected_mps[4], slip_mps[4], threshold_mps[4], scale[4];
    uint8_t spin_mask, lock_mask, intervened_mask, active_brake_mask;
    ChassisTractionReason_e reason;
    bool valid;
} ChassisTractionOutput_s;

typedef struct {
    uint32_t sequence, tick_ms;
    float spin_enter_s, spin_release_s, lock_enter_s, lock_release_s;
    float drive_scale, brake_scale, spin_sign, lock_sign;
    bool seen, spinning, locking;
} ChassisTractionWheel_s;

typedef struct { ChassisTractionWheel_s wheel[4]; } ChassisTractionState_s;

ChassisTractionConfig_s ChassisTractionDefaultConfig(const ChassisVelocityConfig_s *geometry);
/* All failures remove only this optional intervention and clear its history.
 * Existing motor validity, current bounds, power and stop gates remain required. */
bool ChassisTractionUpdate(const ChassisTractionConfig_s *config,
    const ChassisTractionInput_s *input, ChassisTractionState_s *state,
    ChassisTractionOutput_s *output);

#endif
