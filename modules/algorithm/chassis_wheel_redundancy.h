#ifndef CHASSIS_WHEEL_REDUNDANCY_H
#define CHASSIS_WHEEL_REDUNDANCY_H

#include <stdbool.h>
#include <stdint.h>

#include "chassis_kinematics.h"
#include "chassis_velocity.h"

typedef enum {
    CHASSIS_WHEEL_REDUNDANCY_OFF,
    CHASSIS_WHEEL_REDUNDANCY_NO_REFERENCE,
    CHASSIS_WHEEL_REDUNDANCY_NO_OUTLIER,
    CHASSIS_WHEEL_REDUNDANCY_AMBIGUOUS,
    CHASSIS_WHEEL_REDUNDANCY_CORRECTED,
    CHASSIS_WHEEL_REDUNDANCY_INVALID,
} ChassisWheelRedundancyReason_e;

typedef struct {
    bool enabled;
    float residual_threshold_mps;
    float dominance_ratio;
    float max_reference_error_mps;
} ChassisWheelRedundancyConfig_s;

typedef struct {
    bool valid;
    uint8_t online_mask;
    bool reference_valid;
    float reference_vx_mps, reference_vy_mps, reference_wz_radps;
} ChassisWheelRedundancyInput_s;

typedef struct {
    bool valid;
    bool corrected;
    uint8_t excluded_wheel;
    uint8_t used_wheel_mask;
    ChassisWheelRedundancyReason_e reason;
    float all_vx_mps, all_vy_mps, all_wz_radps;
    float selected_vx_mps, selected_vy_mps, selected_wz_radps;
    float wheel_residual_mps[CHASSIS_WHEEL_COUNT];
    float selected_reference_error_mps;
} ChassisWheelRedundancyOutput_s;

/* Estimate chassis velocity from all wheels and test leave-one-out solutions
 * against a time-aligned independent body-velocity reference. Per-wheel speeds
 * already include the configured direction signs. */
bool ChassisEstimateVelocityWithWheelRedundancy(
    const ChassisVelocityConfig_s *kinematics,
    const ChassisWheelRedundancyConfig_s *config,
    const ChassisWheelRedundancyInput_s *input,
    const float wheel_mps[CHASSIS_WHEEL_COUNT],
    ChassisWheelRedundancyOutput_s *output);

#endif
