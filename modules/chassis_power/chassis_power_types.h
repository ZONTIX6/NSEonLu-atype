#ifndef CHASSIS_POWER_TYPES_H
#define CHASSIS_POWER_TYPES_H

#include "power_model.h"
#include "chassis_power_budget.h"
#include "chassis_power_outer_loop.h"
#include "chassis_power_transient.h"
#include "rtt_telemetry.h"
#include "super_cap_types.h"
#include "power_meter_types.h"
#include "referee_power.h"

typedef struct
{
    MotorPowerModelConfig_s models[4];
    ChassisPowerAlgorithmConfig_s algorithm;
    ChassisPowerBudgetConfig_s budget;
    ChassisPowerOuterLoopConfig_s outer;
    ChassisPowerTransientConfig_s transient;
    uint8_t stage;
    bool use_supercap, meter_enabled, buffer_enabled;
    bool force_control;
} ChassisPower_Init_Config_s;

typedef struct
{
    uint32_t version, tick_ms, config_version;
    float nominal_budget_w, target_power_w, slow_correction_w, legacy_budget_w;
    float power_ceiling_w, power_deadband_w;
    float target_rpm[4];
    bool control_enabled, autotune_permitted, zero_force;
    ChassisPowerControlTelemetry_s telemetry;
} ChassisPowerControlSnapshot_s;

typedef struct
{
    ChassisPowerBudgetOutput_s budget;
    ChassisPowerOuterLoopOutput_s outer;
    ChassisPowerControlSnapshot_s control;
    SuperCapCommand_s cap_command;
    bool cap_command_valid, valid, buffer_estimate_valid;
    float physical_load_target_w, confirmed_input_target_w;
    bool force_growth_allowed, input_confirmed;
    uint32_t confirmed_age_ms, cap_send_failures, cap_confirmation_timeouts;
} ChassisPowerPolicyOutput_s;

typedef struct
{
    bool registered;
    ChassisPowerOutput_s power;
    ChassisPowerTelemetry_s telemetry;
    uint32_t control_version, control_tick_ms, tick_ms, config_version;
    float raw_current[4], feedback_current[4], proposed_current[4];
    uint32_t feedback_sequence[4], feedback_tick_ms[4];
    uint8_t online_mask;
    bool base_limited, numeric_error;
    float measured_power_w;
    uint32_t meter_tick_ms;
    bool meter_online;
    ChassisPowerTransientOutput_s transient;
    bool cycle_valid, final_valid, can_submitted;
    uint32_t final_tick_ms;
    int16_t final_current[4];
    ChassisPowerPolicyOutput_s policy;
    uint32_t policy_tick_ms;
} ChassisPowerSnapshot_s;

typedef struct
{
    uint32_t now_ms;
    float dt_s;
    RefereePowerSnapshot_s referee;
    SuperCapSnapshot_s cap;
    PowerMeterStatus_s meter;
    ChassisPowerSnapshot_s previous;
    bool previous_valid, zero_force, bench_enabled, hold_positive_learning;
    bool adaptive_spin, spin_capability_limited, spin_hard_margin;
    float bench_limit_w;
    bool force_backend, force_operating_valid, force_offer_valid;
    float force_operating_demand_w, force_offered_demand_w;
} ChassisPowerPolicyInput_s;

typedef struct
{
    ChassisPowerControlSnapshot_s control;
    float raw_current[4], actual_rpm[4], target_rpm[4], feedback_current[4];
    uint32_t feedback_tick_ms[4], feedback_sequence[4];
    uint8_t online_mask, enabled_mask;
    PowerMeterStatus_s meter;
    uint32_t now_ms;
    float dt_s;
} ChassisPowerCycleInput_s;

typedef struct
{
    float current[4];
    bool valid;
    uint32_t control_version;
    ChassisPowerTransientOutput_s transient;
    ChassisPowerOutput_s model;
} ChassisPowerCycleOutput_s;

typedef struct
{
    uint32_t now_ms, control_version;
    int16_t current_command[4];
    bool can_submitted;
    uint8_t enabled_mask;
} ChassisPowerFinalInput_s;

#endif
