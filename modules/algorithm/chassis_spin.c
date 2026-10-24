#include "chassis_spin.h"
#include "chassis_spin_config.h"

#include <math.h>
#include <stddef.h>

bool ChassisSpinGuardAllowsIncrease(const ChassisPowerTransientOutput_s *guard)
{
    return guard != NULL && guard->guard_scale == 1.0f && !guard->fault &&
           !guard->growth_fallback && !guard->guard_infeasible && !guard->guard_search_exhausted;
}

bool ChassisSpinTransientAllowsIncrease(const ChassisPowerTransientOutput_s *transient)
{
#if CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_FIXED
    /* A tracked low-load target can have no drive demand. Qualify that healthy
     * state so startup and a braking trough can initiate the next bounded rise. */
    return transient != NULL && !transient->fault && transient->meter_fresh &&
        transient->feedback_fresh && transient->timing_valid && transient->brake_mask == 0 &&
        (transient->mode == CHASSIS_POWER_TRANSIENT_RUN ||
         transient->mode == CHASSIS_POWER_TRANSIENT_IDLE) &&
        (transient->positive_allowed || transient->drive_mask == 0);
#else
    return transient != NULL && transient->positive_allowed && transient->brake_mask == 0;
#endif
}

#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
static void QualifyGrowth(const ChassisSpinConfig_s *c, ChassisSpinInput_s *i,
                          ChassisSpinState_s *s)
{
    uint32_t reasons = 0;
    if (!i->spin_enabled || !i->control_enabled) reasons |= CHASSIS_SPIN_BLOCK_CONTROL;
    if (!i->feedback_valid && !i->meter_stale) reasons |= CHASSIS_SPIN_BLOCK_FEEDBACK;
    if (i->meter_stale) reasons |= CHASSIS_SPIN_BLOCK_METER;
    if (i->model_limited) reasons |= CHASSIS_SPIN_BLOCK_MODEL;
    if (i->current_saturated) reasons |= CHASSIS_SPIN_BLOCK_CURRENT;
    if (i->guard_limited || !i->increase_allowed) reasons |= CHASSIS_SPIN_BLOCK_GUARD;
    if (i->braking) reasons |= CHASSIS_SPIN_BLOCK_BRAKE;
    if (i->energy_recovering) reasons |= CHASSIS_SPIN_BLOCK_RECOVERY;
    if (s->applied_wz >= c->max_wz && !i->extension_path)
        reasons |= CHASSIS_SPIN_BLOCK_MAX_SPEED;
    bool tracking = true, new_samples = true, accelerated = true, improved = false;
    for (unsigned w = 0; w < 4; ++w)
    {
        const float speed = fabsf(i->actual_rpm[w]);
        const float error = fabsf(i->target_rpm[w] - i->actual_rpm[w]);
        if (!isfinite(speed) || !isfinite(error) || i->feedback_sequence[w] == 0 ||
            (uint32_t)(i->now_ms - i->feedback_tick_ms[w]) > 5U)
            reasons |= CHASSIS_SPIN_BLOCK_FEEDBACK;
        tracking &= error <= CHASSIS_SPIN_TRACKING_ERROR_RPM;
        new_samples &= i->feedback_sequence[w] != s->trend_sequence[w];
        accelerated &= speed > s->trend_speed[w] + 1 && error <= s->trend_error[w] + 5 &&
            i->actual_rpm[w] * i->target_rpm[w] > 0;
        improved |= error < s->trend_error[w] - 1;
    }
    if (reasons & (CHASSIS_SPIN_BLOCK_CONTROL | CHASSIS_SPIN_BLOCK_FEEDBACK | CHASSIS_SPIN_BLOCK_METER))
    {
        s->trend_initialized = s->trend_allowed = false;
        s->trend_no_improvement_s = 0;
    }
    else if (!s->trend_initialized ||
             (new_samples && (uint32_t)(i->now_ms - s->trend_tick_ms) >= 50U))
    {
        const uint32_t elapsed_ms = i->now_ms - s->trend_tick_ms;
        s->trend_allowed = s->trend_initialized && elapsed_ms <= 60U && accelerated;
        s->trend_no_improvement_s = !s->trend_initialized || improved ? 0 :
            s->trend_no_improvement_s + elapsed_ms * .001f;
        if (s->trend_no_improvement_s >= .100f) s->trend_allowed = false;
        s->trend_initialized = true;
        s->trend_tick_ms = i->now_ms;
        for (unsigned w = 0; w < 4; ++w)
        {
            s->trend_sequence[w] = i->feedback_sequence[w];
            s->trend_speed[w] = fabsf(i->actual_rpm[w]);
            s->trend_error[w] = fabsf(i->target_rpm[w] - i->actual_rpm[w]);
        }
    }
    if ((uint32_t)(i->now_ms - s->trend_tick_ms) > 60U) s->trend_allowed = false;
    i->trend_growth = !tracking && s->trend_allowed;
    if (!tracking && !i->trend_growth) reasons |= CHASSIS_SPIN_BLOCK_TRACKING;
    s->block_reasons = reasons;
    i->increase_allowed = reasons == 0;
}

/* With a feasible zero-spin translation, |offset + gain * Wz| <= bound
 * defines an upper bound for nonnegative Wz. No averaging of wheel constraints. */
static float AffineUpper(float offset, float gain, float bound)
{
    if (!isfinite(offset) || !isfinite(gain) || !isfinite(bound) || bound < 0 ||
        fabsf(offset) > bound) return 0;
    if (gain == 0) return INFINITY;
    return fmaxf(0, ((gain > 0 ? bound : -bound) - offset) / gain);
}

static float NewLimitSample(const ChassisSpinInput_s *i, ChassisSpinState_s *s)
{
    bool fresh = i->power_tick_ms != s->limit_power_tick_ms;
    for (unsigned w=0;w<4;++w)
        fresh &= i->feedback_sequence[w] != s->limit_sequence[w];
    if (s->limit_sample_valid && !fresh) return 0;
    const uint32_t delta=i->power_tick_ms-s->limit_power_tick_ms;
    const float elapsed=s->limit_sample_valid && delta<=20U ? delta*.001f : 0;
    if (s->limit_sample_valid && delta>20U) s->extension_confirm_s=s->tracking_hold_s=0;
    s->limit_sample_valid=true;
    s->limit_power_tick_ms=i->power_tick_ms;
    for(unsigned w=0;w<4;++w) s->limit_sequence[w]=i->feedback_sequence[w];
    return elapsed;
}

/* Returns the absolute candidate bound; soft withdrawal is handled by the slew. */
static float UpdateLimits(const ChassisSpinConfig_s *c, const ChassisSpinInput_s *i,
                           float dt, ChassisSpinState_s *s, bool *withdraw, float *lead_upper,
                           uint32_t *hard_reason)
{
    *withdraw=false;*lead_upper=INFINITY;
    *hard_reason=CHASSIS_SPIN_BLOCK_MAX_SPEED;
    s->power_gap_w=i->target_power_w-s->filtered_power_w;
    s->effective_max_wz=c->max_wz;s->hard_margin=false;
    if (!i->extension_path) {
        s->extension_qualified=false;s->extension_confirm_s=0;
        s->limit_state=CHASSIS_SPIN_LIMIT_NORMAL;
        return c->max_wz;
    }
    const bool calibrated=c->body_hard_rps>0 && c->wheel_hard_rpm>0 && c->hard_wz>=c->max_wz;
    const float hard=c->hard_wz>0 ? c->hard_wz : c->max_wz;
    float upper=hard;
    uint32_t bound_reason=CHASSIS_SPIN_BLOCK_MAX_SPEED;
    bool overspeed=false, actual_at_limit=false;
    bool margin=s->applied_wz<hard*.95f;
    bool tracking=true;
    s->max_tracking_error_rpm=0;
    const float body_target=i->body_wz_offset+i->body_wz_per_wz*s->applied_wz;
    if(isfinite(i->actual_body_wz_rps)) s->peak_body_rps=fmaxf(s->peak_body_rps,fabsf(i->actual_body_wz_rps));
    const float body_band=fmaxf(.05f,.05f*fabsf(body_target));
    s->body_tracking_error_rps=fabsf(body_target-i->actual_body_wz_rps);
    tracking=isfinite(s->body_tracking_error_rps) && s->body_tracking_error_rps<=body_band;
    if (calibrated) {
        const float body_upper=AffineUpper(i->body_wz_offset,i->body_wz_per_wz,c->body_hard_rps);
        if(body_upper<upper) {upper=body_upper;bound_reason=CHASSIS_SPIN_BLOCK_BODY_SPEED;}
        overspeed=fabsf(i->actual_body_wz_rps)>c->body_hard_rps;
        actual_at_limit=fabsf(i->actual_body_wz_rps)>=c->body_hard_rps;
        if(actual_at_limit) s->block_reasons|=CHASSIS_SPIN_BLOCK_BODY_SPEED;
        margin &= fabsf(body_target)<c->body_hard_rps*.95f &&
            fabsf(i->actual_body_wz_rps)<c->body_hard_rps*.95f;
    }
    /* Candidate lead bound may be infeasible at Wz=0 (already rotating).
     * Only its upper endpoint is needed for positive growth. */
    if (i->body_wz_per_wz!=0)
        *lead_upper=fmaxf(0,(i->actual_body_wz_rps+copysignf(body_band,i->body_wz_per_wz)-
            i->body_wz_offset)/i->body_wz_per_wz);
    for(unsigned w=0;w<4;++w) {
        if(isfinite(i->actual_rpm[w])) s->peak_wheel_rpm=fmaxf(s->peak_wheel_rpm,fabsf(i->actual_rpm[w]));
        const float candidate=i->wheel_rpm_offset[w]+i->wheel_rpm_per_wz[w]*s->applied_wz;
        const float error=fmaxf(fabsf(i->target_rpm[w]-i->actual_rpm[w]),
            fabsf(candidate-i->actual_rpm[w]));
        s->max_tracking_error_rpm=fmaxf(s->max_tracking_error_rpm,error);
        tracking &= isfinite(candidate) && isfinite(error) && error<=CHASSIS_SPIN_TRACKING_ERROR_RPM;
        if(i->wheel_rpm_per_wz[w]!=0) {
            const float lead=(i->actual_rpm[w]+copysignf(CHASSIS_SPIN_TRACKING_ERROR_RPM,
                i->wheel_rpm_per_wz[w])-i->wheel_rpm_offset[w])/i->wheel_rpm_per_wz[w];
            *lead_upper=fminf(*lead_upper,fmaxf(0,lead));
        }
        if(calibrated) {
            const float wheel_upper=AffineUpper(i->wheel_rpm_offset[w],i->wheel_rpm_per_wz[w],c->wheel_hard_rpm);
            if(wheel_upper<upper) {upper=wheel_upper;bound_reason=CHASSIS_SPIN_BLOCK_WHEEL_SPEED;}
            overspeed |= fabsf(i->actual_rpm[w])>c->wheel_hard_rpm;
            actual_at_limit |= fabsf(i->actual_rpm[w])>=c->wheel_hard_rpm;
            if(fabsf(i->actual_rpm[w])>=c->wheel_hard_rpm) s->block_reasons|=CHASSIS_SPIN_BLOCK_WHEEL_SPEED;
            margin &= fabsf(candidate)<c->wheel_hard_rpm*.95f &&
                fabsf(i->actual_rpm[w])<c->wheel_hard_rpm*.95f;
        }
    }
    const bool data_valid=i->velocity_valid && isfinite(i->body_wz_per_wz) &&
        i->body_wz_per_wz!=0 && isfinite(i->body_wz_offset) &&
        isfinite(i->translation_mps) && isfinite(i->actual_body_wz_rps) &&
        (uint32_t)(i->now_ms-i->velocity_tick_ms)<=20U &&
        (uint32_t)(i->now_ms-i->power_tick_ms)<=20U;
    if(!data_valid) s->block_reasons|=CHASSIS_SPIN_BLOCK_FEEDBACK;
    const float elapsed=data_valid ? NewLimitSample(i,s) : 0;
    if(!data_valid) s->limit_sample_valid=false;
    const bool near=s->applied_wz>=c->max_wz*.99f;
    if(!tracking && (near || s->extension_qualified)) s->block_reasons|=CHASSIS_SPIN_BLOCK_TRACKING;
    const uint32_t protection=s->block_reasons & (CHASSIS_SPIN_BLOCK_CONTROL |
        CHASSIS_SPIN_BLOCK_FEEDBACK | CHASSIS_SPIN_BLOCK_MODEL | CHASSIS_SPIN_BLOCK_CURRENT |
        CHASSIS_SPIN_BLOCK_GUARD | CHASSIS_SPIN_BLOCK_BRAKE | CHASSIS_SPIN_BLOCK_RECOVERY |
        CHASSIS_SPIN_BLOCK_METER | CHASSIS_SPIN_BLOCK_TRACKING);
    const bool healthy=!protection && !i->power_limited && tracking && data_valid;
    s->tracking_hold_s=healthy ? 0 : fminf(.1f,s->tracking_hold_s+elapsed);
    const bool translation=i->translation_mps>.10f;
    if(translation) s->block_reasons|=CHASSIS_SPIN_BLOCK_TRANSLATION;
    const bool reset=!data_valid || i->braking || i->energy_recovering ||
        (protection & (CHASSIS_SPIN_BLOCK_FEEDBACK | CHASSIS_SPIN_BLOCK_METER)) ||
        s->previous_target_power_w>i->target_power_w+c->budget_drop_reset_w;
    const bool hit=s->applied_wz>=upper || actual_at_limit;
    const bool hard_hold=hit || (s->limit_state==CHASSIS_SPIN_LIMIT_HARD && !margin);
    *hard_reason=bound_reason;
    if(s->applied_wz>=upper) s->block_reasons|=bound_reason;
    if(hard_hold) {
        s->limit_state=CHASSIS_SPIN_LIMIT_HARD;s->extension_qualified=false;
        s->extension_confirm_s=0;
    }
    if(reset || translation || !c->extension_enabled || !calibrated || s->tracking_hold_s>=.1f) {
        s->extension_qualified=false;s->extension_confirm_s=0;
        *withdraw=true;
    }
    if(!c->extension_enabled || !calibrated) s->block_reasons|=CHASSIS_SPIN_BLOCK_CONFIG;
    if(overspeed) {s->extension_qualified=false;*withdraw=true;}
    s->hard_margin=calibrated && margin && healthy && !translation;
    const bool can_qualify=c->extension_enabled && calibrated && healthy && !translation &&
        body_target*i->actual_body_wz_rps>0 &&
        !reset && !hard_hold && upper>s->applied_wz+c->extension_rise_wz_per_s*dt &&
        *lead_upper>=s->applied_wz+c->extension_rise_wz_per_s*dt &&
        isfinite(i->power_ceiling_w) && i->measured_power_w>=0 && i->measured_power_w<i->power_ceiling_w;
    if(!s->extension_qualified && !hard_hold) {
        if(near && can_qualify && s->power_gap_w>fmaxf(3,.05f*i->target_power_w)) {
            s->limit_state=CHASSIS_SPIN_LIMIT_QUALIFY;
            s->extension_confirm_s=fminf(.2f,s->extension_confirm_s+elapsed);
            if(s->extension_confirm_s>=.2f) s->extension_qualified=true;
        } else {
            s->extension_confirm_s=0;
            s->limit_state=(near || s->tracking_hold_s>=.1f) && !healthy ?
                CHASSIS_SPIN_LIMIT_TRACKING_HOLD : CHASSIS_SPIN_LIMIT_NORMAL;
        }
    }
    if(s->extension_qualified)
        s->limit_state=healthy ? CHASSIS_SPIN_LIMIT_EXTENDED : CHASSIS_SPIN_LIMIT_TRACKING_HOLD;
    if(!s->extension_qualified && near && !hard_hold) s->block_reasons|=CHASSIS_SPIN_BLOCK_SOFT_WAIT;
    /* Brief hold and hard-bound hysteresis hold the last output; ordinary
     * revocation descends to soft, while actual overspeed descends below it. */
    s->effective_max_wz=s->extension_qualified ? upper : fminf(upper,c->max_wz);
    s->effective_max_wz=fminf(s->effective_max_wz,fmaxf(s->applied_wz,*lead_upper));
    if(s->tracking_hold_s>=.1f && !tracking)
        s->effective_max_wz=fminf(s->effective_max_wz,*lead_upper);
    if(hard_hold || !healthy) *lead_upper=fminf(*lead_upper,s->applied_wz);
    if(overspeed) s->effective_max_wz=0;
    return upper;
}
#endif

bool ChassisSpinConfigIsValid(const ChassisSpinConfig_s *c)
{
    const bool common = c != NULL && isfinite(c->fixed_wz) && isfinite(c->min_wz) &&
           isfinite(c->max_wz) && isfinite(c->gain_wz_per_w_s) &&
           isfinite(c->rise_wz_per_s) && isfinite(c->fall_wz_per_s) &&
           isfinite(c->power_lpf_tau_s) && isfinite(c->power_deadband_w) &&
           isfinite(c->settle_time_s) && isfinite(c->budget_drop_reset_w) &&
           c->fixed_wz > 0 && c->min_wz > 0 && c->min_wz <= c->fixed_wz &&
           c->max_wz >= c->fixed_wz && c->gain_wz_per_w_s > 0 &&
           c->rise_wz_per_s > 0 && c->fall_wz_per_s > 0 &&
           c->power_lpf_tau_s > 0 && c->power_deadband_w >= 0 &&
           c->settle_time_s >= 0 && c->budget_drop_reset_w >= 0;
    if (!common) return false;
    if (!isfinite(c->hard_wz) || !isfinite(c->body_hard_rps) || !isfinite(c->wheel_hard_rpm) ||
        !isfinite(c->extension_rise_wz_per_s) || c->hard_wz<0 || c->body_hard_rps<0 ||
        c->wheel_hard_rpm<0 || c->extension_rise_wz_per_s<0 ||
        (c->hard_wz>0 && c->hard_wz<c->max_wz)) return false;
    if (c->extension_enabled && (CHASSIS_SPIN_MODE!=CHASSIS_SPIN_MODE_ADAPTIVE ||
        c->hard_wz<c->max_wz || c->body_hard_rps<=0 || c->wheel_hard_rpm<=0 ||
        c->extension_rise_wz_per_s<=0 || c->extension_rise_wz_per_s>c->rise_wz_per_s)) return false;
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE
    return common && isfinite(c->sine_amplitude_wz) && c->sine_amplitude_wz >= 0 &&
        c->fixed_wz - c->sine_amplitude_wz >= c->min_wz &&
        c->fixed_wz + c->sine_amplitude_wz <= c->max_wz &&
        isfinite(c->sine_period_s) && c->sine_period_s > 0 &&
        isfinite(c->sine_ramp_s) && c->sine_ramp_s > 0 &&
        isfinite(c->sine_amplitude_wz / c->sine_ramp_s) &&
        isfinite(c->sine_rise_wz_per_s) && c->sine_rise_wz_per_s > 0 &&
        isfinite(c->sine_fall_wz_per_s) && c->sine_fall_wz_per_s > 0 &&
        (c->sine_direction == 1 || c->sine_direction == -1);
#else
    return common;
#endif
}

#if CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_SINE
static void Reset(const ChassisSpinConfig_s *config, ChassisSpinState_s *state)
{
    *state = (ChassisSpinState_s){.target_wz = config->fixed_wz,
        .peak_wheel_rpm=state->peak_wheel_rpm,.peak_body_rps=state->peak_body_rps};
}
#else
static float Clamp(float value, float minimum, float maximum)
{
    return fminf(fmaxf(value, minimum), maximum);
}

static bool UpdateSine(const ChassisSpinConfig_s *c, const ChassisSpinInput_s *i,
                        float dt, ChassisSpinState_s *s, float *target)
{
    *target = 0;
    if (!i->spin_enabled || !i->control_enabled)
    {
        *s = (ChassisSpinState_s){0};
        return true;
    }
    if (!isfinite(dt) || dt <= 0 || dt > .020f ||
        !isfinite(i->target_power_w) || i->target_power_w <= 0 ||
        !isfinite(i->measured_power_w) ||
        (s->sine_direction_locked && s->sine_direction != 1 && s->sine_direction != -1) ||
        (s->initialized && (!isfinite(s->target_wz) || s->target_wz < c->min_wz ||
            s->target_wz > c->max_wz || !isfinite(s->filtered_power_w) ||
            !isfinite(s->settled_s) || s->settled_s < 0 ||
            !isfinite(s->sine_phase_s) ||
            s->sine_phase_s < 0 || !isfinite(s->sine_center_wz) ||
            s->sine_center_wz < c->min_wz || s->sine_center_wz > c->fixed_wz ||
            !isfinite(s->sine_amplitude_wz) || s->sine_amplitude_wz < 0 ||
            s->sine_amplitude_wz > c->sine_amplitude_wz)))
    {
        *s = (ChassisSpinState_s){0};
        return false;
    }
    const int8_t direction = s->sine_direction_locked ? s->sine_direction : c->sine_direction;
    if (!s->initialized || !i->feedback_valid)
    {
        *s = (ChassisSpinState_s){.target_wz = c->min_wz,
            .sine_center_wz = c->min_wz, .filtered_power_w = i->measured_power_w,
            .sine_direction = direction,
            .sine_direction_locked = true, .initialized = i->feedback_valid};
        if (!i->feedback_valid)
        {
            *target = (float)direction * c->min_wz;
            return true;
        }
    }
    const float alpha = dt / (c->power_lpf_tau_s + dt);
    s->filtered_power_w = (1 - alpha) * s->filtered_power_w + alpha * i->measured_power_w;
    s->sine_phase_s = fmodf(s->sine_phase_s + dt, c->sine_period_s);
    /* A demand-driven budget can fall when buffer boost is no longer needed.
     * Keep phase/envelope independent of that change; actual excess power and
     * protection flags below constrain the target through the normal slew. */
    const float excess_w = fmaxf(0, fmaxf(i->measured_power_w, s->filtered_power_w) -
        i->target_power_w - c->power_deadband_w);
    const bool restricted = i->energy_recovering || i->power_limited || excess_w > 0;
    const bool can_increase = i->increase_allowed && !restricted && i->measured_power_w >= 0;
    s->settled_s = can_increase ? fminf(s->settled_s + dt, c->settle_time_s) : 0;
    const bool stable = can_increase && s->settled_s >= c->settle_time_s;
    if (excess_w > 0)
        s->sine_center_wz = fmaxf(c->min_wz, s->sine_center_wz -
            fminf(c->gain_wz_per_w_s * excess_w, c->sine_fall_wz_per_s) * dt);
    else if (stable)
        s->sine_center_wz = fminf(c->fixed_wz, s->sine_center_wz + c->sine_rise_wz_per_s * dt);

    const float amplitude_step = c->sine_amplitude_wz / c->sine_ramp_s * dt;
    if (restricted)
        s->sine_amplitude_wz = fmaxf(0, s->sine_amplitude_wz - amplitude_step);
    else if (stable && s->sine_center_wz >= c->fixed_wz)
        s->sine_amplitude_wz = fminf(c->sine_amplitude_wz, s->sine_amplitude_wz + amplitude_step);
    const float requested = Clamp(s->sine_center_wz + s->sine_amplitude_wz *
        (s->sine_center_wz / c->fixed_wz) *
            sinf(6.28318530718f * (s->sine_phase_s / c->sine_period_s)), c->min_wz, c->max_wz);
    float change = requested - s->target_wz;
    if (change > 0 && !stable) change = 0;
    s->target_wz = Clamp(s->target_wz + Clamp(change,
        -c->sine_fall_wz_per_s * dt, c->sine_rise_wz_per_s * dt), c->min_wz, c->max_wz);
    *target = (float)direction * s->target_wz;
    return true;
}
#endif

static bool UpdateTarget(const ChassisSpinConfig_s *config,
                       const ChassisSpinInput_s *input,
                       float dt_s,
                       ChassisSpinState_s *state,
                       float *target_wz)
{
    if (target_wz != NULL)
        *target_wz = 0;
    if (!ChassisSpinConfigIsValid(config) || input == NULL || state == NULL || target_wz == NULL)
    {
        if (state != NULL) *state = (ChassisSpinState_s){0};
        return false;
    }
    *target_wz = config->fixed_wz;

#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_FIXED
    (void)dt_s;
    Reset(config, state);
    return true;
#elif CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE
    return UpdateSine(config, input, dt_s, state, target_wz);
#else
    *target_wz = 0;
    if (!input->spin_enabled || !input->control_enabled || !input->feedback_valid)
    {
        Reset(config, state);
        if(!input->spin_enabled || !input->control_enabled)
            state->peak_wheel_rpm=state->peak_body_rps=0;
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
        const float previous = state->applied_wz;
        const uint32_t reasons = state->block_reasons;
        Reset(config, state);
        state->block_reasons = reasons;
        state->applied_wz = previous;
        state->target_wz = fminf(fmaxf(previous, 0), config->max_wz);
        state->filtered_power_w = input->measured_power_w;
        state->initialized = true;
    }
    else
    {
        state->filtered_power_w += dt_s / (config->power_lpf_tau_s + dt_s) *
            (input->measured_power_w - state->filtered_power_w);
    }
    state->target_wz=state->applied_wz;
    bool withdraw=false;
    float lead_upper=INFINITY;
    uint32_t hard_reason=0;
    const float hard_upper=UpdateLimits(config,input,dt_s,state,&withdraw,&lead_upper,&hard_reason);
    if (state->previous_target_power_w >
        input->target_power_w + config->budget_drop_reset_w)
    {
        state->target_wz = fminf(state->target_wz, config->fixed_wz);
        state->settled_s = 0;
    }
    state->previous_target_power_w = input->target_power_w;
    const bool can_increase = input->increase_allowed && !input->power_limited &&
        !input->energy_recovering && input->measured_power_w >= 0 &&
        (!input->extension_path || !(state->block_reasons & (CHASSIS_SPIN_BLOCK_FEEDBACK |
            CHASSIS_SPIN_BLOCK_TRACKING | CHASSIS_SPIN_BLOCK_BODY_SPEED | CHASSIS_SPIN_BLOCK_WHEEL_SPEED)));
    state->settled_s = can_increase ?
        fminf(state->settled_s + dt_s, config->settle_time_s) : 0;

    const float error = input->target_power_w - state->filtered_power_w;
    const float outside_deadband = copysignf(
        fmaxf(fabsf(error) - config->power_deadband_w, 0), error);
    float rate = config->gain_wz_per_w_s * outside_deadband;
    if (rate > 0 && (!can_increase || state->settled_s < config->settle_time_s))
        rate = 0;
    float rise = config->rise_wz_per_s * (input->trend_growth ? .25f : 1);
    if(input->extension_path && state->target_wz+rise*dt_s>config->max_wz && config->extension_enabled)
        rise=fminf(rise,config->extension_rise_wz_per_s);
    rate = fminf(fmaxf(rate, -config->fall_wz_per_s), rise);
    if(withdraw && state->target_wz>state->effective_max_wz)
        rate=-config->fall_wz_per_s;
    const float minimum = state->target_wz < config->min_wz ||
        (withdraw && state->effective_max_wz<config->min_wz) ? 0 : config->min_wz;
    float candidate=fmaxf(state->target_wz+rate*dt_s,minimum);
    if(withdraw && state->target_wz>state->effective_max_wz)
        candidate=fmaxf(candidate,state->effective_max_wz);
    float growth_upper=state->extension_qualified ? hard_upper :
        fmaxf(config->max_wz,state->target_wz);
    if(input->extension_path)
        growth_upper=fminf(growth_upper,fmaxf(state->target_wz,lead_upper));
    state->target_wz=fminf(candidate,fminf(hard_upper,growth_upper));
    if(input->extension_path && state->target_wz>=hard_upper) {
        state->block_reasons|=hard_reason;
        state->limit_state=CHASSIS_SPIN_LIMIT_HARD;
        state->extension_qualified=false;state->extension_confirm_s=0;
    }
    *target_wz = state->target_wz;
    return true;
#endif
}

bool ChassisSpinRetune(const ChassisSpinConfig_s *config, ChassisSpinState_s *state)
{
    if (!state || !ChassisSpinConfigIsValid(config)) return false;
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE
    if (state->sine_direction_locked && config->sine_direction != state->sine_direction)
        return false;
#endif
    const float previous = state->applied_wz;
    const int8_t direction = state->sine_direction;
    const bool locked = state->sine_direction_locked;
    const float peak_wheel=state->peak_wheel_rpm, peak_body=state->peak_body_rps;
    *state = (ChassisSpinState_s){.applied_wz = previous, .retuning = previous != 0,
        .peak_wheel_rpm=peak_wheel,.peak_body_rps=peak_body,
        .sine_direction = direction, .sine_direction_locked = locked};
    return true;
}

bool ChassisSpinUpdate(const ChassisSpinConfig_s *config,
                       const ChassisSpinInput_s *input, float dt_s,
                       ChassisSpinState_s *state, float *target_wz)
{
    const float previous = state ? state->applied_wz : 0;
    const bool retuning = state && state->retuning;
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
    ChassisSpinInput_s qualified;
    if (config && input && state && input->evaluate_tracking)
    {
        qualified = *input;
        QualifyGrowth(config, &qualified, state);
        input = &qualified;
    }
#endif
    bool ok = UpdateTarget(config, input, dt_s, state, target_wz);
    if (!state || !target_wz) return ok;
    state->retuning = false;
    if (ok && retuning && (CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_ADAPTIVE || !input->extension_path) &&
        input->spin_enabled && input->control_enabled &&
        input->feedback_valid && isfinite(dt_s) && dt_s > 0 && dt_s <= .020f) {
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE
        float rate = fabsf(*target_wz) > fabsf(previous) ?
            config->sine_rise_wz_per_s : config->sine_fall_wz_per_s;
#else
        float rate = fabsf(*target_wz) > fabsf(previous) ?
            config->rise_wz_per_s : config->fall_wz_per_s;
#endif
        if (fabsf(*target_wz) > fabsf(previous) &&
            (!input->increase_allowed || input->power_limited || input->energy_recovering ||
             input->measured_power_w > input->target_power_w + config->power_deadband_w))
            rate = 0;
        const float change = *target_wz - previous;
        const float step = fminf(fmaxf(change, -rate * dt_s), rate * dt_s);
        state->retuning = fabsf(change - step) > .0001f;
        *target_wz = previous + step;
    }
    state->applied_wz = *target_wz;
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
    state->target_wz=*target_wz;
#endif
    return ok;
}
