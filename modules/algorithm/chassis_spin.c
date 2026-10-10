#include "chassis_spin.h"
#include "chassis_spin_config.h"

#include <math.h>
#include <stddef.h>

bool ChassisSpinGuardAllowsIncrease(const ChassisPowerTransientOutput_s *guard)
{
    return guard != NULL && guard->guard_scale == 1.0f && !guard->fault &&
           !guard->growth_fallback && !guard->guard_infeasible && !guard->guard_search_exhausted;
}

static bool ConfigIsValid(const ChassisSpinConfig_s *c)
{
    return c != NULL && isfinite(c->fixed_wz) && isfinite(c->min_wz) &&
           isfinite(c->max_wz) && isfinite(c->gain_wz_per_w_s) &&
           isfinite(c->rise_wz_per_s) && isfinite(c->fall_wz_per_s) &&
           isfinite(c->power_lpf_tau_s) && isfinite(c->power_deadband_w) &&
           isfinite(c->settle_time_s) && isfinite(c->budget_drop_reset_w) &&
           c->fixed_wz > 0 && c->min_wz > 0 && c->min_wz <= c->fixed_wz &&
           c->max_wz >= c->fixed_wz && c->gain_wz_per_w_s > 0 &&
           c->rise_wz_per_s > 0 && c->fall_wz_per_s > 0 &&
           c->power_lpf_tau_s > 0 && c->power_deadband_w >= 0 &&
           c->settle_time_s >= 0 && c->budget_drop_reset_w >= 0;
}

static void Reset(const ChassisSpinConfig_s *config, ChassisSpinState_s *state)
{
    *state = (ChassisSpinState_s){.target_wz = config->fixed_wz};
}

bool ChassisSpinUpdate(const ChassisSpinConfig_s *config,
                       const ChassisSpinInput_s *input,
                       float dt_s,
                       ChassisSpinState_s *state,
                       float *target_wz)
{
    if (target_wz != NULL)
        *target_wz = 0;
    if (!ConfigIsValid(config) || input == NULL || state == NULL || target_wz == NULL)
        return false;
    *target_wz = config->fixed_wz;

#if !CHASSIS_SPIN_ADAPTIVE_ENABLE
    (void)dt_s;
    Reset(config, state);
    return true;
#else
    if (!input->spin_enabled || !input->control_enabled || !input->feedback_valid)
    {
        Reset(config, state);
        return true;
    }
    if (!isfinite(dt_s) || dt_s <= 0 || dt_s > .020f ||
        !isfinite(input->target_power_w) || input->target_power_w <= 0 ||
        !isfinite(input->measured_power_w) ||
        (state->initialized &&
         (!isfinite(state->target_wz) || !isfinite(state->filtered_power_w) ||
          !isfinite(state->settled_s) || state->settled_s < 0 ||
          !isfinite(state->previous_target_power_w))))
    {
        Reset(config, state);
        return false;
    }
    if (!state->initialized)
    {
        Reset(config, state);
        state->filtered_power_w = input->measured_power_w;
        state->initialized = true;
    }
    else
    {
        state->filtered_power_w += dt_s / (config->power_lpf_tau_s + dt_s) *
            (input->measured_power_w - state->filtered_power_w);
    }
    if (state->previous_target_power_w >
        input->target_power_w + config->budget_drop_reset_w)
    {
        state->target_wz = fminf(state->target_wz, config->fixed_wz);
        state->settled_s = 0;
    }
    state->previous_target_power_w = input->target_power_w;
    const bool can_increase = input->increase_allowed && input->measured_power_w >= 0;
    state->settled_s = can_increase ?
        fminf(state->settled_s + dt_s, config->settle_time_s) : 0;

    const float error = input->target_power_w - state->filtered_power_w;
    const float outside_deadband = copysignf(
        fmaxf(fabsf(error) - config->power_deadband_w, 0), error);
    float rate = config->gain_wz_per_w_s * outside_deadband;
    if (rate > 0 && (!can_increase || state->settled_s < config->settle_time_s))
        rate = 0;
    rate = fminf(fmaxf(rate, -config->fall_wz_per_s), config->rise_wz_per_s);
    state->target_wz = fminf(fmaxf(state->target_wz + rate * dt_s, config->min_wz),
                            config->max_wz);
    *target_wz = state->target_wz;
    return true;
#endif
}
