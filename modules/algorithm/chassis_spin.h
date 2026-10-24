#ifndef CHASSIS_SPIN_H
#define CHASSIS_SPIN_H

#include <stdbool.h>
#include <stdint.h>
#include "chassis_power_transient.h"

typedef struct
{
    float fixed_wz, min_wz, max_wz;
    float gain_wz_per_w_s, rise_wz_per_s, fall_wz_per_s;
    float power_lpf_tau_s, power_deadband_w, settle_time_s;
    float budget_drop_reset_w; /* Adaptive mode only; sine never rescales on budget changes. */
    float sine_amplitude_wz, sine_period_s, sine_ramp_s;
    float sine_rise_wz_per_s, sine_fall_wz_per_s;
    int8_t sine_direction;
} ChassisSpinConfig_s;

enum
{
    CHASSIS_SPIN_BLOCK_CONTROL = 1U << 0,
    CHASSIS_SPIN_BLOCK_FEEDBACK = 1U << 1,
    CHASSIS_SPIN_BLOCK_MODEL = 1U << 2,
    CHASSIS_SPIN_BLOCK_CURRENT = 1U << 3,
    CHASSIS_SPIN_BLOCK_GUARD = 1U << 4,
    CHASSIS_SPIN_BLOCK_BRAKE = 1U << 5,
    CHASSIS_SPIN_BLOCK_TRACKING = 1U << 6,
    CHASSIS_SPIN_BLOCK_MAX_SPEED = 1U << 7,
    CHASSIS_SPIN_BLOCK_RECOVERY = 1U << 8,
    CHASSIS_SPIN_BLOCK_METER = 1U << 9,
};

typedef struct
{
    bool spin_enabled, control_enabled, feedback_valid, increase_allowed;
    float target_power_w, measured_power_w;
    bool power_limited, energy_recovering;
    bool evaluate_tracking, model_limited, current_saturated, guard_limited, braking, meter_stale;
    uint32_t now_ms, feedback_tick_ms[4], feedback_sequence[4];
    float actual_rpm[4], target_rpm[4];
    bool trend_growth; /* Produced internally when evaluate_tracking is enabled. */
} ChassisSpinInput_s;

typedef struct
{
    float target_wz, filtered_power_w, settled_s, previous_target_power_w;
    bool initialized;
    float sine_phase_s, sine_center_wz, sine_amplitude_wz;
    int8_t sine_direction;
    bool sine_direction_locked;
    float applied_wz;
    bool retuning;
    uint32_t block_reasons, trend_tick_ms, trend_sequence[4];
    float trend_speed[4], trend_error[4], trend_no_improvement_s;
    bool trend_initialized, trend_allowed;
} ChassisSpinState_s;

/* Limits the target only; existing power/current protection remains authoritative. */
bool ChassisSpinConfigIsValid(const ChassisSpinConfig_s *config);
/* Reset learning but slew from the last emitted command; direction stays locked. */
bool ChassisSpinRetune(const ChassisSpinConfig_s *config, ChassisSpinState_s *state);
bool ChassisSpinUpdate(const ChassisSpinConfig_s *config,
                       const ChassisSpinInput_s *input,
                       float dt_s,
                       ChassisSpinState_s *state,
                       float *target_wz);

/* Applied stage 2/3 current guards must also permit a larger target. */
bool ChassisSpinGuardAllowsIncrease(const ChassisPowerTransientOutput_s *guard);

/* Sine may establish drive demand from a healthy, non-braking low-load state. */
bool ChassisSpinTransientAllowsIncrease(const ChassisPowerTransientOutput_s *transient);

#endif
