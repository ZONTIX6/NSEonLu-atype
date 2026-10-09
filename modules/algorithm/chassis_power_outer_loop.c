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

    if (!isfinite(dt_s) || dt_s <= 0.0f)
    {
        if (!isfinite(state->integral_w) || state->integral_w > 0.0f)
            state->integral_w = 0.0f;
        if (!isfinite(state->correction_w) || state->correction_w > 0.0f)
            state->correction_w = 0.0f;

        output->correction_w = state->correction_w;
        output->filtered_power_w = isfinite(state->filtered_power_w)
                                       ? state->filtered_power_w
                                       : 0.0f;
        output->corrected_budget_w = ClampFloat(
            input->nominal_budget_w + state->correction_w,
            0.0f,
            input->nominal_budget_w + config->max_positive_correction_w);
        return false;
    }

    if (!input->control_enabled)
    {
        ChassisPowerOuterLoopReset(state);
        output->mode = CHASSIS_POWER_OUTER_STANDBY;
        return true;
    }

    if (state->previous_nominal_budget_w > 0.0f &&
        input->nominal_budget_w <
            state->previous_nominal_budget_w - config->budget_drop_reset_w)
    {
        if (state->integral_w > 0.0f)
            state->integral_w = 0.0f;
        if (state->correction_w > 0.0f)
            state->correction_w = 0.0f;
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
            float control_power_w = state->filtered_power_w;
            if (input->measured_power_w > input->nominal_budget_w &&
                input->measured_power_w > control_power_w)
            {
                control_power_w = input->measured_power_w;
            }

            float error_w = input->target_power_w - control_power_w;
            if (fabsf(error_w) <= config->error_deadband_w)
                error_w = 0.0f;
            const float lower_limit_w = -input->nominal_budget_w;
            const float upper_limit_w =
                config->max_positive_correction_w;
            const float previous_integral_w = state->integral_w;
            float integral_candidate_w = ClampFloat(
                previous_integral_w + config->ki * error_w * dt_s,
                lower_limit_w,
                upper_limit_w);
            const float proportional_w = config->kp * error_w;
            float candidate_w = proportional_w + integral_candidate_w;
            if (!input->allow_positive_correction && candidate_w > 0.0f)
            {
                integral_candidate_w = previous_integral_w;
                candidate_w = proportional_w + integral_candidate_w;
            }
            else if ((candidate_w >= upper_limit_w && error_w > 0.0f) ||
                     (candidate_w <= lower_limit_w && error_w < 0.0f))
            {
                const float previous_candidate_w =
                    proportional_w + previous_integral_w;
                if (candidate_w >= upper_limit_w && error_w > 0.0f &&
                    previous_candidate_w < upper_limit_w)
                {
                    integral_candidate_w = upper_limit_w - proportional_w;
                }
                else if (candidate_w <= lower_limit_w && error_w < 0.0f &&
                         previous_candidate_w > lower_limit_w)
                {
                    integral_candidate_w = lower_limit_w - proportional_w;
                }
                else
                {
                    integral_candidate_w = previous_integral_w;
                }
                integral_candidate_w = ClampFloat(
                    integral_candidate_w, lower_limit_w, upper_limit_w);
                candidate_w = proportional_w + integral_candidate_w;
            }
            if (!input->allow_positive_correction && candidate_w > 0.0f)
                candidate_w = 0.0f;
            state->integral_w = integral_candidate_w;
            candidate_w = ClampFloat(
                candidate_w,
                lower_limit_w,
                upper_limit_w);
            if (candidate_w > state->correction_w)
            {
                const float maximum_increase_w =
                    config->positive_slew_rate_w_per_s * dt_s;
                state->correction_w = fminf(
                    candidate_w, state->correction_w + maximum_increase_w);
            }
            else
            {
                state->correction_w = candidate_w;
            }
            output->error_w = error_w;
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

bool ChassisPowerOuterLoopUpdateIsolated(const ChassisPowerOuterLoopConfig_s *c,
    const ChassisPowerOuterLoopInput_s *i, float dt, ChassisPowerOuterLoopState_s *s,
    ChassisPowerOuterLoopOutput_s *o)
{
    if (s == NULL || o == NULL) return false;
    ClearOutput(o);
    if (!ChassisPowerOuterLoopConfigIsValid(c) || i == NULL) {
        ChassisPowerOuterLoopReset(s); return false;
    }
    if (!i->control_enabled) { ChassisPowerOuterLoopReset(s); o->mode = CHASSIS_POWER_OUTER_STANDBY; return true; }
    if (!isfinite(i->nominal_budget_w) || i->nominal_budget_w <= 0 ||
        !isfinite(i->target_power_w) || i->target_power_w <= 0 ||
        !isfinite(s->integral_w) || !isfinite(s->correction_w) || !isfinite(s->applied_correction_w)) {
        ChassisPowerOuterLoopReset(s); return false;
    }
    const bool timing = isfinite(dt) && dt > 0 && dt <= .020f;
    if (!timing) dt = 0;
    if (s->previous_nominal_budget_w > i->nominal_budget_w + c->budget_drop_reset_w) {
        s->integral_w = fminf(s->integral_w, 0);
        s->correction_w = fminf(s->correction_w, 0);
        s->applied_correction_w = fminf(s->applied_correction_w, 0);
    }
    if (i->meter_online && isfinite(i->measured_power_w) && timing && i->learning_allowed) {
        if (!ChassisPowerOuterLoopUpdate(c, i, dt, s, o)) return false;
    } else {
        if (!i->meter_online) s->measurement_initialized = false;
        o->mode = i->meter_online ? CHASSIS_POWER_OUTER_STANDBY : CHASSIS_POWER_OUTER_OFFLINE;
    }
    float target = s->correction_w;
    if (!i->allow_positive_correction || !i->meter_online || !timing) target = fminf(target, 0);
    s->applied_correction_w = target > s->applied_correction_w ?
        fminf(target, s->applied_correction_w + c->positive_slew_rate_w_per_s * dt) : target;
    s->previous_nominal_budget_w = i->nominal_budget_w;
    o->correction_w = ClampFloat(s->applied_correction_w, -i->nominal_budget_w, c->max_positive_correction_w);
    o->filtered_power_w = s->filtered_power_w;
    o->corrected_budget_w = ClampFloat(i->nominal_budget_w + o->correction_w, 0, i->nominal_budget_w + c->max_positive_correction_w);
    return true;
}
