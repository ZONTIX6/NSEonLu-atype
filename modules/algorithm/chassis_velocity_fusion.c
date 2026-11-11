#include "chassis_velocity_fusion.h"

#include <math.h>

#define HALF_CLOCK_RANGE (UINT32_C(1) << 31)

static bool ValidConfig(const ChassisVelocityFusionConfig_s *c)
{
    return c && isfinite(c->wheel_variance) && c->wheel_variance > 0 &&
        isfinite(c->accel_variance) && c->accel_variance > 0 &&
        isfinite(c->jerk_density) && c->jerk_density > 0 &&
        isfinite(c->max_velocity_residual_mps) && c->max_velocity_residual_mps > 0 &&
        c->max_age_us > 0 && c->max_age_us < HALF_CLOCK_RANGE &&
        c->max_skew_us < HALF_CLOCK_RANGE &&
        c->max_gap_us > 0 && c->max_gap_us < HALF_CLOCK_RANGE;
}

static bool ValidAxis(const ChassisVelocityFusionAxis_s *axis)
{
    return isfinite(axis->velocity) && isfinite(axis->accel) &&
        isfinite(axis->p_vv) && axis->p_vv > 0 && isfinite(axis->p_va) &&
        isfinite(axis->p_aa) && axis->p_aa > 0;
}

static bool Advance(ChassisVelocityFusion_s *s, const ChassisVelocityFusionConfig_s *c,
                    uint32_t time_us)
{
    const uint32_t elapsed_us = time_us - s->state_us;
    if (elapsed_us > c->max_gap_us) return false;
    if (!elapsed_us) return true;

    const float dt = (float)elapsed_us * 1e-6f;
    const float q_aa = c->jerk_density * dt;
    const float q_va = .5f * dt * q_aa;
    const float q_vv = (dt / 3.f) * dt * q_aa;
    for (unsigned i = 0; i < 2; ++i) {
        ChassisVelocityFusionAxis_s *a = &s->axis[i];
        a->velocity += dt * a->accel;
        a->p_vv += dt * (2.f * a->p_va + dt * a->p_aa) + q_vv;
        a->p_va += dt * a->p_aa + q_va;
        a->p_aa += q_aa;
        if (!ValidAxis(a)) return false;
    }
    s->state_us = time_us;
    return true;
}

static bool Observe(ChassisVelocityFusion_s *s, const float measurement[2],
                    float variance, bool wheel)
{
    for (unsigned i = 0; i < 2; ++i) {
        ChassisVelocityFusionAxis_s *a = &s->axis[i];
        float *value = wheel ? &a->velocity : &a->accel;
        float *other = wheel ? &a->accel : &a->velocity;
        float *p_value = wheel ? &a->p_vv : &a->p_aa;
        float *p_other = wheel ? &a->p_aa : &a->p_vv;
        const float innovation = measurement[i] - *value;
        const float innovation_variance = *p_value + variance;
        if (!isfinite(innovation_variance) || innovation_variance <= 0) return false;
        const float k_value = *p_value / innovation_variance;
        const float k_other = a->p_va / innovation_variance;
        const float keep = variance / innovation_variance;
        *value += k_value * innovation;
        *other += k_other * innovation;

        /* Scalar Joseph update. Store the off-diagonal once to maintain
         * symmetry; compute 1-KH as R/S to avoid cancellation. */
        *p_other += k_other * (k_other * *p_value - 2.f * a->p_va) +
            k_other * k_other * variance;
        a->p_va = keep * (a->p_va - k_other * *p_value) + k_value * k_other * variance;
        *p_value = keep * keep * *p_value + k_value * k_value * variance;
        if (!ValidAxis(a)) return false;
    }
    return true;
}

static bool WheelAgrees(const ChassisVelocityFusion_s *s,
                        const ChassisVelocityFusionConfig_s *c,
                        const ChassisVelocityFusionInput_s *in)
{
    const float dx = s->axis[0].velocity - in->wheel_n_mps[0];
    const float dy = s->axis[1].velocity - in->wheel_n_mps[1];
    return hypotf(dx, dy) <= c->max_velocity_residual_mps;
}

static bool ObserveWheel(ChassisVelocityFusion_s *s, const ChassisVelocityFusionConfig_s *c,
                         const ChassisVelocityFusionInput_s *in)
{
    return Advance(s, c, in->wheel_us) && WheelAgrees(s, c, in) &&
        Observe(s, in->wheel_n_mps, c->wheel_variance, true);
}

static bool ObserveImu(ChassisVelocityFusion_s *s, const ChassisVelocityFusionConfig_s *c,
                       const ChassisVelocityFusionInput_s *in)
{
    return Advance(s, c, in->imu_us) && Observe(s, in->accel_n_mps2, c->accel_variance, false);
}

static void Seed(ChassisVelocityFusion_s *s, const ChassisVelocityFusionConfig_s *c,
                 const ChassisVelocityFusionInput_s *in)
{
    for (unsigned i = 0; i < 2; ++i) {
        s->axis[i] = (ChassisVelocityFusionAxis_s){
            .velocity = in->wheel_n_mps[i], .accel = in->accel_n_mps2[i],
            .p_vv = c->wheel_variance, .p_aa = c->accel_variance,
        };
    }
    s->state_us = in->wheel_us;
    s->initialized = true;
}

void ChassisVelocityFusionReset(ChassisVelocityFusion_s *s)
{
    if (s) *s = (ChassisVelocityFusion_s){0};
}

void ChassisVelocityFusionUpdate(ChassisVelocityFusion_s *s,
                                const ChassisVelocityFusionConfig_s *c,
                                const ChassisVelocityFusionInput_s *in,
                                ChassisVelocityFusionOutput_s *out)
{
    if (!out) return;
    *out = (ChassisVelocityFusionOutput_s){0};
    if (!s) return;
    if (!ValidConfig(c) || !in || !in->wheel_valid ||
        in->now_us - in->wheel_us > c->max_age_us ||
        !isfinite(in->wheel_n_mps[0]) || !isfinite(in->wheel_n_mps[1]) ||
        (s->have_wheel_time && in->wheel_us - s->last_wheel_us >= HALF_CLOCK_RANGE)) {
        s->initialized = false;
        return;
    }

    *out = (ChassisVelocityFusionOutput_s){
        .velocity_n_mps = {in->wheel_n_mps[0], in->wheel_n_mps[1]},
        .valid = true, .sample_us = in->wheel_us,
    };
    const bool new_wheel = !s->have_wheel_time || in->wheel_us != s->last_wheel_us;
    s->have_wheel_time = true;
    s->last_wheel_us = in->wheel_us;
    const uint32_t wheel_age = in->now_us - in->wheel_us;
    const uint32_t imu_age = in->now_us - in->imu_us;
    const bool imu_first = imu_age > wheel_age;
    const uint32_t skew = imu_first ? imu_age - wheel_age : wheel_age - imu_age;
    if (!in->imu_valid || imu_age > c->max_age_us || skew > c->max_skew_us ||
        !isfinite(in->accel_n_mps2[0]) || !isfinite(in->accel_n_mps2[1]) ||
        (s->have_imu_time && in->imu_us - s->last_imu_us >= HALF_CLOCK_RANGE)) {
        s->initialized = false;
        return;
    }
    const bool new_imu = !s->have_imu_time || in->imu_us != s->last_imu_us;
    s->have_imu_time = true;
    s->last_imu_us = in->imu_us;

    bool accepted = true;
    if (!s->initialized) {
        /* Reacquire only from a new pair after a bad interval. */
        if (!new_wheel || !new_imu) return;
        Seed(s, c, in);
        if (!imu_first) accepted = Advance(s, c, in->imu_us);
    } else if (imu_first) {
        accepted = (!new_imu || ObserveImu(s, c, in)) &&
            (!new_wheel || ObserveWheel(s, c, in));
    } else {
        accepted = (!new_wheel || ObserveWheel(s, c, in)) &&
            (!new_imu || ObserveImu(s, c, in));
    }
    if (!accepted || !WheelAgrees(s, c, in)) {
        s->initialized = false;
        return;
    }

    out->velocity_n_mps[0] = s->axis[0].velocity;
    out->velocity_n_mps[1] = s->axis[1].velocity;
    out->sample_us = s->state_us;
    out->fused = true;
}
