#include "chassis_power_transient.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static float Clamp(float v, float lo, float hi) { return fminf(hi, fmaxf(lo, v)); }
ChassisPowerTransientConfig_s ChassisPowerTransientDefaultConfig(void)
{
    return (ChassisPowerTransientConfig_s){.eta = .95f, .fast_gain = .5f,
        .release_w_per_s = 60, .stale_release_w_per_s = 20,
        .meter_fresh_ms = 20, .feedback_fresh_ms = 5,
        .release_confirm_ms = 10, .stable_run_ms = 100};
}
void ChassisPowerTransientReset(ChassisPowerTransientState_s *s)
{
    if (s != NULL) memset(s, 0, sizeof(*s));
}
void ChassisPowerTransientRecordSent(ChassisPowerTransientState_s *s, const float current[4])
{
    if (s != NULL && current != NULL) memcpy(s->sent_current, current, sizeof(s->sent_current));
}
bool ChassisPowerTransientUpdate(const ChassisPowerTransientConfig_s *c,
    const ChassisPowerTransientInput_s *i, float dt, ChassisPowerTransientState_s *s,
    ChassisPowerTransientOutput_s *o)
{
    if (o == NULL || s == NULL) return false;
    memset(o, 0, sizeof(*o));
    if (c == NULL || i == NULL || !isfinite(c->eta) || c->eta <= 0 || c->eta > 1 ||
        !isfinite(c->fast_gain) || c->fast_gain < 0 || !isfinite(c->release_w_per_s) ||
        c->release_w_per_s <= 0 || !isfinite(c->stale_release_w_per_s) ||
        c->stale_release_w_per_s <= 0 || c->meter_fresh_ms == 0 || c->feedback_fresh_ms == 0) {
        ChassisPowerTransientReset(s); o->fault = true; return false;
    }
    if (!i->control_enabled) { ChassisPowerTransientReset(s); return true; }
    bool valid = isfinite(i->budget_w) && i->budget_w > 0 && isfinite(i->target_w) &&
                 i->target_w > 0 && isfinite(i->slow_w) && isfinite(s->fast_w);
    o->feedback_fresh = i->online_mask == 15 && i->enabled_mask == 15;
    for (unsigned w = 0; w < 4; ++w) {
        valid &= isfinite(i->raw_current[w]) && isfinite(i->actual_rpm[w]) && isfinite(i->target_rpm[w]);
        o->feedback_fresh &= i->feedback_sequence[w] != 0 &&
            (uint32_t)(i->now_ms - i->feedback_tick_ms[w]) <= c->feedback_fresh_ms;
        const float direction = fabsf(i->actual_rpm[w]) < 50 ? i->target_rpm[w] : i->actual_rpm[w];
        if (i->raw_current[w] * direction > 0) o->drive_mask |= (uint8_t)(1U << w);
        if (i->raw_current[w] * direction < 0) o->brake_mask |= (uint8_t)(1U << w);
    }
    if (!valid) { ChassisPowerTransientReset(s); o->fault = true; return false; }
    o->timing_valid = isfinite(dt) && dt > 0 && dt <= .005f;
    if (!o->timing_valid) dt = 0;
    o->meter_fresh = i->meter_enabled && i->meter_online && isfinite(i->meter_w) &&
        (uint32_t)(i->now_ms - i->meter_tick_ms) <= c->meter_fresh_ms;
    const bool new_meter = o->meter_fresh && (!s->meter_initialized || s->meter_tick_ms != i->meter_tick_ms);
    if (!o->meter_fresh) {
        s->meter_initialized = false; s->fresh_samples = s->below_samples = 0; s->below_valid = false;
        s->mode = CHASSIS_POWER_TRANSIENT_OFFLINE;
        s->fast_w = i->meter_enabled ? fminf(0, s->fast_w + c->stale_release_w_per_s * dt) : 0;
    } else {
        if (new_meter) {
            s->meter_initialized = true; s->meter_tick_ms = i->meter_tick_ms;
            if (s->fresh_samples < 2) ++s->fresh_samples;
        }
        if (i->meter_w > i->budget_w) {
            const float request = Clamp(-c->fast_gain * (i->meter_w - i->target_w) / c->eta, -i->budget_w, 0);
            s->fast_w = fminf(s->fast_w, request);
            s->below_valid = false; s->below_samples = 0;
            s->mode = CHASSIS_POWER_TRANSIENT_CLAMP;
        } else if (s->fast_w < 0) {
            if (i->meter_w < i->target_w - .5f) {
                if (new_meter) {
                    if (!s->below_valid) { s->below_valid = true; s->below_since_ms = i->now_ms; }
                    if (s->below_samples < 2) ++s->below_samples;
                }
                if (s->below_valid && s->below_samples >= 2 &&
                    (uint32_t)(i->now_ms - s->below_since_ms) >= c->release_confirm_ms) {
                    s->mode = CHASSIS_POWER_TRANSIENT_RECOVERY;
                    s->fast_w = fminf(0, s->fast_w + c->release_w_per_s * dt);
                } else s->mode = CHASSIS_POWER_TRANSIENT_CLAMP;
            } else { s->below_valid = false; s->below_samples = 0; s->mode = CHASSIS_POWER_TRANSIENT_CLAMP; }
            if (s->fast_w == 0) s->mode = CHASSIS_POWER_TRANSIENT_RUN;
        } else s->mode = s->fresh_samples >= 2 ? CHASSIS_POWER_TRANSIENT_RUN : CHASSIS_POWER_TRANSIENT_OFFLINE;
    }
    if (!o->feedback_fresh && s->mode == CHASSIS_POWER_TRANSIENT_RUN)
        s->mode = CHASSIS_POWER_TRANSIENT_OFFLINE;
    if (s->mode == CHASSIS_POWER_TRANSIENT_RUN) {
        if (!s->run_valid) { s->run_valid = true; s->run_since_ms = i->now_ms; }
    } else s->run_valid = false;
    o->positive_allowed = s->mode == CHASSIS_POWER_TRANSIENT_RUN &&
        o->feedback_fresh && o->brake_mask == 0 && i->meter_w >= 0 && o->timing_valid;
    o->learning_allowed = o->positive_allowed &&
        (uint32_t)(i->now_ms - s->run_since_ms) >= c->stable_run_ms;
    o->slow_applied_w = i->meter_enabled ? (o->positive_allowed ? i->slow_w : fminf(i->slow_w, 0)) : 0;
    s->fast_w = Clamp(s->fast_w, -i->budget_w, 0);
    o->fast_w = s->fast_w;
    o->model_budget_w = Clamp(i->budget_w + o->slow_applied_w + o->fast_w, 0, i->budget_w + 8);
    o->effective_w = c->eta * o->model_budget_w; o->mode = s->mode;
    return true;
}
