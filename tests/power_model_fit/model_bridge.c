#include "power_model.h"
#include "chassis_power_model_config.h"
#include "chassis_power_transient.h"
#include <math.h>

void FitBaseline(MotorPowerModelConfig_s *model)
{
    *model = (MotorPowerModelConfig_s)CHASSIS_SPEED_POWER_MODEL_INITIALIZER;
}

float FitPredict(const MotorPowerModelConfig_s *model, const float command[4], const float rpm[4])
{
    float total = 0;
    for (unsigned w = 0; w < 4; ++w)
        total += PowerModelPredict(model, command[w], rpm[w]);
    return total;
}

/* Return numeric validity separately from physical-budget feasibility. */
int FitLimit(const MotorPowerModelConfig_s *model, const float command[4], const float rpm[4],
             float budget, float final[4], float *total, unsigned *guard_flags)
{
    MotorPowerModelConfig_s models[4];
    ChassisPowerInput_s input = {.referee_power_limit_w = budget, .attenuation = 1};
    const ChassisPowerAlgorithmConfig_s config = {.safety_factor = 1,
        .small_error_threshold_rpm = 500, .reserved_power_threshold_w = 54,
        .per_motor_reserved_power_w = 8, .max_current_command = 15000};
    for (unsigned w = 0; w < 4; ++w) {
        models[w] = *model;
        input.desired_current[w] = command[w];
        input.speed_rpm[w] = rpm[w];
        input.speed_error_rpm[w] = 1000;
    }
    ChassisPowerOutput_s output;
    if (!PowerModelApply(models, &config, &input, &output)) return 0;
    /* Stage 3 applies a total-power guard after the intermediate allocation.
     * Even a globally feasible budget can leave one wheel's passive loss above
     * its allocated share; PowerModelApply alone is not the final constraint. */
    ChassisPowerTransientConfig_s guard_config = ChassisPowerTransientDefaultConfig();
    guard_config.eta = 1;
    ChassisPowerTransientState_s state = {0};
    ChassisPowerTransientInput_s guard_input = {.budget_w = budget, .target_w = budget,
        .online_mask = 15, .enabled_mask = 15, .control_enabled = true};
    ChassisPowerTransientOutput_s guard;
    for (unsigned w = 0; w < 4; ++w) {
        guard_input.raw_current[w] = command[w];
        guard_input.actual_rpm[w] = rpm[w];
        guard_input.target_rpm[w] = rpm[w] + (command[w] == 0 ? 0 : copysignf(1000, command[w]));
    }
    /* Two actual Update calls establish the production masks/freshness and
     * zero acceleration. This is a steady-state check, not dynamic replay. */
    for (unsigned step = 1; step <= 2; ++step) {
        guard_input.now_ms = 98 + 2*step;
        for (unsigned w = 0; w < 4; ++w) {
            guard_input.feedback_tick_ms[w] = guard_input.now_ms;
            guard_input.feedback_sequence[w] = step;
        }
        if (!ChassisPowerTransientUpdate(&guard_config, &guard_input, .002f, &state, &guard))
            return 0;
    }
    (void)ChassisPowerTransientGuard(&guard_config, models, &guard_input,
                                    output.limited_current, &state, &guard);
    if (guard.fault) return 0;
    *guard_flags = (guard.guard_infeasible ? 1U : 0U) | (guard.guard_search_exhausted ? 2U : 0U);
    for (unsigned w = 0; w < 4; ++w) {
        if (!isfinite(guard.guarded_current[w]) || fabsf(guard.guarded_current[w]) > 15000)
            return 0;
        final[w] = truncf(guard.guarded_current[w]);
    }
    *total = FitPredict(model, final, rpm);
    return isfinite(*total);
}
