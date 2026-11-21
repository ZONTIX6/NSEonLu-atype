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

    output->feedback_active = true;
    float error_x = demand_x - input->fused_vx_mps;
    float error_y = demand_y - input->fused_vy_mps;
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
