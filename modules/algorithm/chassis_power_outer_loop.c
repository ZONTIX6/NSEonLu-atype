#include "chassis_power_outer_loop.h"

#include <math.h>
#include <stddef.h>

static float ClampFloat(float value, float minimum, float maximum)
{
    if (value < minimum)
        return minimum;
    if (value > maximum)
        return maximum;
    return value;
}

static float MoveTowardZero(float value, float maximum_delta)
{
    if (value > 0.0f)
        return fmaxf(0.0f, value - maximum_delta);
    if (value < 0.0f)
        return fminf(0.0f, value + maximum_delta);
    return 0.0f;
}

static void ClearOutput(ChassisPowerOuterLoopOutput_s *output)
{
    if (output == NULL)
        return;

    *output = (ChassisPowerOuterLoopOutput_s){
        .mode = CHASSIS_POWER_OUTER_OFFLINE,
    };
}

bool ChassisPowerOuterLoopConfigIsValid(
    const ChassisPowerOuterLoopConfig_s *config)
{
    if (config == NULL)
        return false;

    if (!isfinite(config->kp) || !isfinite(config->ki) ||
        !isfinite(config->measurement_lpf_tau_s) ||
        !isfinite(config->activation_margin_w) ||
        !isfinite(config->error_deadband_w) ||
        !isfinite(config->max_positive_correction_w) ||
        !isfinite(config->positive_slew_rate_w_per_s) ||
        !isfinite(config->budget_drop_reset_w))
    {
        return false;
    }

    return config->kp >= 0.0f &&
           config->ki >= 0.0f &&
           config->measurement_lpf_tau_s > 0.0f &&
           config->activation_margin_w >= 0.0f &&
           config->error_deadband_w >= 0.0f &&
           config->max_positive_correction_w > 0.0f &&
           config->positive_slew_rate_w_per_s > 0.0f &&
           config->budget_drop_reset_w >= 0.0f;
}

void ChassisPowerOuterLoopReset(ChassisPowerOuterLoopState_s *state)
{
    if (state == NULL)
        return;

    *state = (ChassisPowerOuterLoopState_s){0};
}

bool ChassisPowerOuterLoopUpdate(
    const ChassisPowerOuterLoopConfig_s *config,
    const ChassisPowerOuterLoopInput_s *input,
    float dt_s,
    ChassisPowerOuterLoopState_s *state,
    ChassisPowerOuterLoopOutput_s *output)
{
    if (state == NULL || output == NULL)
    {
        ChassisPowerOuterLoopReset(state);
        ClearOutput(output);
        return false;
    }

    ClearOutput(output);

    if (!ChassisPowerOuterLoopConfigIsValid(config) || input == NULL ||
        !isfinite(input->nominal_budget_w) || input->nominal_budget_w <= 0.0f ||
        !isfinite(input->target_power_w) || input->target_power_w <= 0.0f ||
        !isfinite(input->predicted_unlimited_power_w) ||
        input->predicted_unlimited_power_w < 0.0f)
    {
        ChassisPowerOuterLoopReset(state);
        return false;
    }

    if (!input->control_enabled)
    {
        ChassisPowerOuterLoopReset(state);
        output->mode = CHASSIS_POWER_OUTER_STANDBY;
        return true;
    }

    if (!input->meter_online || !isfinite(input->measured_power_w))
    {
        state->measurement_initialized = false;
        state->integral_w = 0.0f;
        if (state->correction_w > 0.0f)
            state->correction_w = 0.0f;
        else
            state->correction_w = MoveTowardZero(
                state->correction_w,
                config->positive_slew_rate_w_per_s * dt_s);
        output->mode = CHASSIS_POWER_OUTER_OFFLINE;
    }
    else
    {
        if (!state->measurement_initialized)
        {
            state->filtered_power_w = input->measured_power_w;
            state->measurement_initialized = true;
        }
        else
        {
            const float alpha = dt_s /
                                (config->measurement_lpf_tau_s + dt_s);
            state->filtered_power_w +=
                alpha * (input->measured_power_w - state->filtered_power_w);
        }

        const bool demand_near_limit =
            input->predicted_unlimited_power_w >=
                input->target_power_w - config->activation_margin_w ||
            state->filtered_power_w >=
                input->target_power_w - config->activation_margin_w ||
            input->measured_power_w > input->nominal_budget_w;

        if (!demand_near_limit)
        {
            state->integral_w = 0.0f;
            if (state->correction_w > 0.0f)
                state->correction_w = 0.0f;
            else
                state->correction_w = MoveTowardZero(
                    state->correction_w,
                    config->positive_slew_rate_w_per_s * dt_s);
            output->mode = CHASSIS_POWER_OUTER_STANDBY;
        }
        else
        {
            state->integral_w = 0.0f;
            state->correction_w = 0.0f;
            output->mode = CHASSIS_POWER_OUTER_ACTIVE;
        }
    }

    state->previous_nominal_budget_w = input->nominal_budget_w;
    output->correction_w = state->correction_w;
    output->filtered_power_w = state->filtered_power_w;
    output->corrected_budget_w = ClampFloat(
        input->nominal_budget_w + state->correction_w,
        0.0f,
        input->nominal_budget_w + config->max_positive_correction_w);

    return true;
}
