#ifndef CHASSIS_POWER_OUTER_LOOP_H
#define CHASSIS_POWER_OUTER_LOOP_H

#include <stdbool.h>

typedef enum
{
    CHASSIS_POWER_OUTER_OFFLINE = 0,
    CHASSIS_POWER_OUTER_STANDBY,
    CHASSIS_POWER_OUTER_ACTIVE,
} ChassisPowerOuterLoopMode_e;

typedef struct
{
    float kp;
    float ki;
    float measurement_lpf_tau_s;
    float activation_margin_w;
    float error_deadband_w;
    float max_positive_correction_w;
    float positive_slew_rate_w_per_s;
    float budget_drop_reset_w;
} ChassisPowerOuterLoopConfig_s;

typedef struct
{
    float filtered_power_w;
    float integral_w;
    float correction_w;
    float applied_correction_w;
    float previous_nominal_budget_w;
    bool measurement_initialized;
} ChassisPowerOuterLoopState_s;

typedef struct
{
    float nominal_budget_w;
    float target_power_w;
    float measured_power_w;
    float predicted_unlimited_power_w;
    bool meter_online;
    bool allow_positive_correction;
    bool control_enabled;
    bool learning_allowed;
} ChassisPowerOuterLoopInput_s;

typedef struct
{
    float corrected_budget_w;
    float correction_w;
    float filtered_power_w;
    float error_w;
    ChassisPowerOuterLoopMode_e mode;
} ChassisPowerOuterLoopOutput_s;

bool ChassisPowerOuterLoopConfigIsValid(
    const ChassisPowerOuterLoopConfig_s *config);
void ChassisPowerOuterLoopReset(ChassisPowerOuterLoopState_s *state);
bool ChassisPowerOuterLoopUpdate(
    const ChassisPowerOuterLoopConfig_s *config,
    const ChassisPowerOuterLoopInput_s *input,
    float dt_s,
    ChassisPowerOuterLoopState_s *state,
    ChassisPowerOuterLoopOutput_s *output);

bool ChassisPowerOuterLoopUpdateIsolated(
    const ChassisPowerOuterLoopConfig_s *config,
    const ChassisPowerOuterLoopInput_s *input, float dt_s,
    ChassisPowerOuterLoopState_s *state, ChassisPowerOuterLoopOutput_s *output);

#endif
