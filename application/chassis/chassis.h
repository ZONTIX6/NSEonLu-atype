#ifndef CHASSIS_H
#define CHASSIS_H

#include <stdbool.h>
#include <stdint.h>
#include "chassis_velocity.h"

/* Stable read-only data pointer for debugger/watch use.
 * Volatile pointer access keeps this debug anchor in the linked firmware.
 * Use the snapshot function for coherent cross-task reads and freshness checks. */
extern const ChassisVelocityFeedback_s *const volatile chassis_velocity;
/* Debug symbols for the opt-in bounded remote-fusion control trial. */
extern volatile bool chassis_velocity_control_trial_limited;
extern volatile bool chassis_velocity_control_trial_feedback_active;
extern volatile float chassis_velocity_control_trial_correction_vx_mps;
extern volatile float chassis_velocity_control_trial_correction_vy_mps;
/* Returns actual.valid; target retains its own independent validity. */
bool ChassisGetVelocitySnapshot(ChassisVelocityFeedback_s *snapshot);

typedef struct {
    bool valid;
    uint32_t tick_ms;
    uint8_t mode;
    float vx_cmd, vy_cmd, wz_cmd;
    float offset_angle; /* Yaw encoder relative to the chassis alignment, degrees. */
    ChassisVelocityFeedback_s velocity;
    float nominal_budget_w, target_power_w;
    bool buffer_control_enabled, buffer_estimate_valid, buffer_recovering;
    float buffer_lower_j, buffer_predicted_j;
    uint32_t spin_phase, spin_block_reasons;
    float spin_base_power_w, spin_buffer_request_w, spin_buffer_grant_w;
    float spin_base_confirm_s, spin_filtered_power_w, spin_power_ceiling_w, slow_correction_w;
    float spin_gain, spin_rise, spin_fall, spin_lpf_s, spin_settle_s;
    float spin_soft_wz, spin_hard_wz, spin_effective_wz, spin_body_hard_rps, spin_wheel_hard_rpm;
    uint32_t spin_limit_state;
    bool spin_extension_enabled, spin_extension_qualified, spin_capability_latched;
    float spin_extension_confirm_s, spin_power_gap_w, spin_max_tracking_error_rpm;
    float spin_body_tracking_error_rps, spin_capability_release_s;
    float spin_peak_wheel_rpm, spin_peak_body_rps;
} ChassisTelemetrySnapshot_s;

bool ChassisGetTelemetrySnapshot(ChassisTelemetrySnapshot_s *snapshot);

/**
 * @brief 底盘应用初始化,请在开启rtos之前调用(目前会被RobotInit()调用)
 * 
 */
void ChassisInit();

/**
 * @brief 底盘应用任务,放入实时系统以一定频率运行
 * 
 */
void ChassisTask();

#endif // CHASSIS_H
