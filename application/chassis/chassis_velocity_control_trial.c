#include "chassis_velocity_control_trial.h"

#include <math.h>

#define TWO_PI 6.2831853071795864769f

static bool ConfigValid(const ChassisVelocityControlTrialConfig_s *config)
{
    return config && isfinite(config->max_speed_mps) && config->max_speed_mps > 0.0f &&
        isfinite(config->proportional_gain) && config->proportional_gain >= 0.0f &&
        config->proportional_gain <= 1.0f &&
        isfinite(config->max_correction_mps) && config->max_correction_mps >= 0.0f &&
        config->max_correction_mps <= config->max_speed_mps &&
        isfinite(config->deadband_mps) && config->deadband_mps >= 0.0f &&
        config->max_sample_age_ms > 0U;
}

static void LimitVector(float *x, float *y, float limit)
{
    const float magnitude = hypotf(*x, *y);
    if (magnitude > limit) {
        const float scale = limit / magnitude;
        *x *= scale;
        *y *= scale;
    }
}

bool ChassisVelocityControlTrialCommandScale(float wheel_radius_m,
    float reduction_ratio, float *mps_per_command_unit)
{
    if (!mps_per_command_unit || !isfinite(wheel_radius_m) || wheel_radius_m <= 0.0f ||
        !isfinite(reduction_ratio) || reduction_ratio <= 0.0f) {
        return false;
    }
    const float scale = wheel_radius_m * TWO_PI / (360.0f * reduction_ratio);
    if (!isfinite(scale) || scale <= 0.0f) return false;
    *mps_per_command_unit = scale;
    return true;
}

bool ChassisVelocityControlTrialApply(const ChassisVelocityControlTrialConfig_s *config,
    const ChassisVelocityControlTrialInput_s *input,
    ChassisVelocityControlTrialOutput_s *output)
{
    if (!output) return false;
    *output = (ChassisVelocityControlTrialOutput_s){0};
    if (!input) return false;

    output->vx_mps = input->requested_vx_mps;
    output->vy_mps = input->requested_vy_mps;
    if (!input->enabled || !input->speed_limit_allowed) return true;

    output->command_limited = true;
    if (!ConfigValid(config) || !isfinite(input->requested_vx_mps) ||
        !isfinite(input->requested_vy_mps)) {
        /* Fail closed in an explicitly enabled trial build. */
        output->vx_mps = 0.0f;
        output->vy_mps = 0.0f;
        return false;
    }

    float demand_x = input->requested_vx_mps;
    float demand_y = input->requested_vy_mps;
    LimitVector(&demand_x, &demand_y, config->max_speed_mps);
    output->vx_mps = demand_x;
    output->vy_mps = demand_y;

    const uint32_t sample_age_ms = input->now_ms - input->fused_tick_ms;
    if (!input->feedback_allowed || !input->fused_snapshot_valid || !input->fused_status ||
        !input->wheel_valid || sample_age_ms > config->max_sample_age_ms ||
        !isfinite(input->fused_vx_mps) || !isfinite(input->fused_vy_mps)) {
        return true;
    }

    float reference_x = demand_x, reference_y = demand_y;
    float feedback_x = input->fused_vx_mps, feedback_y = input->fused_vy_mps;
    if (input->spin_mode) {
        if (!isfinite(input->reference_vx_mps) || !isfinite(input->reference_vy_mps) ||
            !isfinite(input->wheel_wz_radps)) return true;
        reference_x = input->reference_vx_mps;
        reference_y = input->reference_vy_mps;
        LimitVector(&reference_x, &reference_y, config->max_speed_mps);
        /* Rotate the historical sample into current body axes. This assumes
         * constant angular speed; it does not predict linear acceleration. */
        const float angle = input->wheel_wz_radps * ((float)sample_age_ms * .001f);
        const float co = cosf(angle), si = sinf(angle);
        feedback_x = co * input->fused_vx_mps - si * input->fused_vy_mps;
        feedback_y = si * input->fused_vx_mps + co * input->fused_vy_mps;
        if (!isfinite(feedback_x) || !isfinite(feedback_y)) return true;
    }

    output->feedback_active = true;
    output->reference_vx_mps = reference_x;
    output->reference_vy_mps = reference_y;
    output->feedback_vx_mps = feedback_x;
    output->feedback_vy_mps = feedback_y;
    float error_x = reference_x - feedback_x;
    float error_y = reference_y - feedback_y;
    if (hypotf(error_x, error_y) <= config->deadband_mps) return true;

    float correction_x = config->proportional_gain * error_x;
    float correction_y = config->proportional_gain * error_y;
    LimitVector(&correction_x, &correction_y, config->max_correction_mps);
    output->vx_mps = demand_x + correction_x;
    output->vy_mps = demand_y + correction_y;
    LimitVector(&output->vx_mps, &output->vy_mps, config->max_speed_mps);
    output->correction_vx_mps = output->vx_mps - demand_x;
    output->correction_vy_mps = output->vy_mps - demand_y;
    return true;
}

void ChassisVelocityControlTrialRecord(ChassisVelocityControlTrace_s *trace,
    const ChassisVelocityControlTrialConfig_s *config,
    const ChassisVelocityControlTrialInput_s *input,
    const ChassisVelocityControlTrialOutput_s *output,
    uint32_t now_ms, uint32_t control_version, const float target_rpm[4],
    bool published, bool drive_enabled)
{
    if (!trace || !target_rpm) return;
    const bool active = output && output->feedback_active;
    const bool corrected = active && isfinite(output->correction_vx_mps) &&
        isfinite(output->correction_vy_mps) &&
        (output->correction_vx_mps != 0.0f || output->correction_vy_mps != 0.0f);
    trace->available = true;
    trace->tick_ms = now_ms;
    trace->control_version = control_version;
    ++trace->control_cycles;
    trace->feedback_cycles += active;
    trace->correction_cycles += corrected;
    trace->flags &= CHASSIS_TRIAL_LAST_CORRECTION;
    trace->flags |= (active ? CHASSIS_TRIAL_FEEDBACK_ACTIVE : 0U) |
        (corrected ? CHASSIS_TRIAL_CORRECTION_NONZERO : 0U) |
        (published ? CHASSIS_TRIAL_PUBLISHED : 0U) |
        (drive_enabled ? CHASSIS_TRIAL_DRIVE_ENABLED : 0U);
    trace->fused_age_ms = input ? input->now_ms - input->fused_tick_ms : 0U;
    if (input) {
        trace->flags |= (input->enabled ? CHASSIS_TRIAL_ENABLED : 0U) |
            (input->feedback_allowed ? CHASSIS_TRIAL_MODE_ALLOWED : 0U) |
            (input->fused_snapshot_valid ? CHASSIS_TRIAL_SNAPSHOT_VALID : 0U) |
            (input->fused_status ? CHASSIS_TRIAL_FUSED : 0U) |
            (input->wheel_valid ? CHASSIS_TRIAL_WHEEL_VALID : 0U) |
            (config && input->fused_snapshot_valid && trace->fused_age_ms <= config->max_sample_age_ms ?
                CHASSIS_TRIAL_FRESH : 0U);
    }
    trace->requested_vx_mps = input ? input->requested_vx_mps : 0.0f;
    trace->requested_vy_mps = input ? input->requested_vy_mps : 0.0f;
    trace->reference_vx_mps = active ? output->reference_vx_mps : 0.0f;
    trace->reference_vy_mps = active ? output->reference_vy_mps : 0.0f;
    trace->feedback_vx_mps = active ? output->feedback_vx_mps : 0.0f;
    trace->feedback_vy_mps = active ? output->feedback_vy_mps : 0.0f;
    trace->correction_vx_mps = corrected ? output->correction_vx_mps : 0.0f;
    trace->correction_vy_mps = corrected ? output->correction_vy_mps : 0.0f;
    for (unsigned i = 0; i < 4; ++i) trace->target_rpm[i] = target_rpm[i];
    if (corrected && published) {
        trace->flags |= CHASSIS_TRIAL_LAST_CORRECTION;
        trace->last_correction_vx_mps = output->correction_vx_mps;
        trace->last_correction_vy_mps = output->correction_vy_mps;
        trace->last_correction_tick_ms = now_ms;
        trace->last_correction_control_version = control_version;
        for (unsigned i = 0; i < 4; ++i) trace->last_correction_target_rpm[i] = target_rpm[i];
    }
}
