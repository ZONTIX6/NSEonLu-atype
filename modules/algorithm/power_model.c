#include "power_model.h"

#include <math.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POWER_MODEL_EPSILON 1.0e-6f
#define POWER_MODEL_DISCRIMINANT_EPSILON 1.0e-5f

static Chassis_Power_Control_Data_s *chassis_power_feedback;

static float ClampFloat(float value, float minimum, float maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

static void ResetOutput(ChassisPowerOutput_s *output, PowerModelStatus_e status)
{
    memset(output, 0, sizeof(*output));
    output->status = status;
}

static void ResetFeedback(Chassis_Power_Control_Data_s *feedback)
{
    memset(feedback, 0, sizeof(*feedback));
}

bool ChassisPowerFeedbackInit(void)
{
    if (chassis_power_feedback != NULL)
        return false;

    chassis_power_feedback = malloc(sizeof(*chassis_power_feedback));
    if (chassis_power_feedback == NULL)
        return false;

    ResetFeedback(chassis_power_feedback);
    return true;
}

void ChassisPowerFeedbackUpdate(bool registered,
                                const ChassisPowerOutput_s *power)
{
    if (chassis_power_feedback == NULL)
        return;

    ResetFeedback(chassis_power_feedback);
    if (!registered || power == NULL || power->status != POWER_MODEL_STATUS_OK)
        return;

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        chassis_power_feedback->predicted_unlimited_power_w +=
            power->predicted_unlimited_power_w[i];
        chassis_power_feedback->predicted_limited_power_w +=
            power->predicted_limited_power_w[i];
    }
    chassis_power_feedback->effective_limit_w = power->effective_limit_w;

    if (!isfinite(chassis_power_feedback->predicted_unlimited_power_w) ||
        !isfinite(chassis_power_feedback->predicted_limited_power_w) ||
        !isfinite(chassis_power_feedback->effective_limit_w))
        ResetFeedback(chassis_power_feedback);
}

bool ChassisPowerFeedbackGet(Chassis_Power_Control_Data_s *feedback)
{
    if (feedback == NULL)
        return false;

    ResetFeedback(feedback);
    if (chassis_power_feedback == NULL)
        return false;

    *feedback = *chassis_power_feedback;
    return true;
}

bool PowerModelConfigIsValid(const MotorPowerModelConfig_s *config)
{
    return config != NULL && isfinite(config->k0) && isfinite(config->k1) &&
           isfinite(config->k2) && isfinite(config->k3) && isfinite(config->k4) &&
           isfinite(config->k5) && isfinite(config->current_conversion) &&
           config->current_conversion > 0.0f;
}

bool ChassisPowerAlgorithmConfigIsValid(const ChassisPowerAlgorithmConfig_s *config)
{
    if (config == NULL || !isfinite(config->safety_factor) ||
        !isfinite(config->small_error_threshold_rpm) ||
        !isfinite(config->reserved_power_threshold_w) ||
        !isfinite(config->per_motor_reserved_power_w) ||
        !isfinite(config->max_current_command))
        return false;

    return config->safety_factor > 0.0f && config->safety_factor <= 1.0f &&
           config->small_error_threshold_rpm >= 0.0f &&
           config->reserved_power_threshold_w >= 0.0f &&
           config->per_motor_reserved_power_w >= 0.0f &&
           config->reserved_power_threshold_w >=
               POWER_MODEL_MOTOR_COUNT * config->per_motor_reserved_power_w &&
           config->max_current_command > 0.0f;
}

float PowerModelSelectLimit(float referee_limit_w, float fallback_limit_w)
{
    if (isfinite(referee_limit_w) && referee_limit_w > 0.0f)
        return referee_limit_w;
    if (isfinite(fallback_limit_w) && fallback_limit_w > 0.0f)
        return fallback_limit_w;
    return referee_limit_w;
}

float PowerModelPredict(const MotorPowerModelConfig_s *config,
                        float current_command,
                        float speed_rpm)
{
    if (!PowerModelConfigIsValid(config) || !isfinite(current_command) || !isfinite(speed_rpm))
        return NAN;

    const float current = fabsf(current_command / config->current_conversion);
    const float speed = fabsf(speed_rpm);
    return config->k0 + config->k1 * current + config->k2 * speed +
           config->k3 * current * speed + config->k4 * current * current +
           config->k5 * speed * speed;
}

static bool IsScaleInRange(float scale)
{
    return isfinite(scale) && scale >= 0.0f && scale <= 1.0f;
}

static float CalculateCurrentScale(const MotorPowerModelConfig_s *model,
                                   float desired_current,
                                   float speed_rpm,
                                   float power_limit_w)
{
    const float current = fabsf(desired_current / model->current_conversion);
    const float speed = fabsf(speed_rpm);
    const float a = model->k4 * current * current;
    const float b = (model->k1 + model->k3 * speed) * current;
    const float c = model->k0 + model->k2 * speed + model->k5 * speed * speed -
                    power_limit_w;

    if (fabsf(a) < POWER_MODEL_EPSILON)
    {
        if (fabsf(b) < POWER_MODEL_EPSILON)
            return c <= POWER_MODEL_EPSILON ? 1.0f : 0.0f;

        const float scale = -c / b;
        return IsScaleInRange(scale) ? scale : 0.0f;
    }

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < -POWER_MODEL_DISCRIMINANT_EPSILON)
        return 0.0f;
    if (discriminant < 0.0f)
        discriminant = 0.0f;

    const float root = sqrtf(discriminant);
    const float scale1 = (-b - root) / (2.0f * a);
    const float scale2 = (-b + root) / (2.0f * a);
    const bool scale1_valid = IsScaleInRange(scale1);
    const bool scale2_valid = IsScaleInRange(scale2);

    if (scale1_valid && scale2_valid)
        return scale1 > scale2 ? scale1 : scale2;
    if (scale1_valid)
        return scale1;
    if (scale2_valid)
        return scale2;
    return 0.0f;
}

static void AllocatePower(const ChassisPowerAlgorithmConfig_s *config,
                          const ChassisPowerInput_s *input,
                          ChassisPowerOutput_s *output)
{
    float total_error = 0.0f;
    float errors[POWER_MODEL_MOTOR_COUNT];

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        errors[i] = fabsf(input->speed_error_rpm[i]);
        total_error += errors[i];
    }

    if (total_error <= config->small_error_threshold_rpm)
    {
        const float share = output->effective_limit_w / POWER_MODEL_MOTOR_COUNT;
        for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
            output->allocated_power_w[i] = share;
        return;
    }

    if (output->effective_limit_w < config->reserved_power_threshold_w)
    {
        for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
            output->allocated_power_w[i] = errors[i] / total_error * output->effective_limit_w;
        return;
    }

    const float reserved_total = POWER_MODEL_MOTOR_COUNT * config->per_motor_reserved_power_w;
    const float proportional_power = output->effective_limit_w - reserved_total;
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        output->allocated_power_w[i] = config->per_motor_reserved_power_w +
                                       errors[i] / total_error * proportional_power;
    }
}

bool PowerModelApply(const MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT],
                      const ChassisPowerAlgorithmConfig_s *config,
                      const ChassisPowerInput_s *input,
                     ChassisPowerOutput_s *output)
{
    if (output == NULL)
        return false;
    if (models == NULL || config == NULL || input == NULL)
    {
        ResetOutput(output, POWER_MODEL_STATUS_INVALID_ARGUMENT);
        return false;
    }
    if (!ChassisPowerAlgorithmConfigIsValid(config))
    {
        ResetOutput(output, POWER_MODEL_STATUS_INVALID_CONFIG);
        return false;
    }

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        if (!PowerModelConfigIsValid(&models[i]))
        {
            ResetOutput(output, POWER_MODEL_STATUS_INVALID_CONFIG);
            return false;
        }
        if (!isfinite(input->desired_current[i]) || !isfinite(input->speed_rpm[i]) ||
            !isfinite(input->speed_error_rpm[i]))
        {
            ResetOutput(output, POWER_MODEL_STATUS_NUMERIC_ERROR);
            return false;
        }
    }

    if (!isfinite(input->referee_power_limit_w) || !isfinite(input->attenuation))
    {
        ResetOutput(output, POWER_MODEL_STATUS_NUMERIC_ERROR);
        return false;
    }

    ResetOutput(output, POWER_MODEL_STATUS_OK);
    output->effective_limit_w = input->referee_power_limit_w *
                                ClampFloat(input->attenuation, 0.0f, 1.0f) *
                                config->safety_factor;
    if (!isfinite(output->effective_limit_w) || output->effective_limit_w <= 0.0f)
    {
        ResetOutput(output, POWER_MODEL_STATUS_INVALID_BUDGET);
        return false;
    }

    AllocatePower(config, input, output);

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        const float desired_current = ClampFloat(input->desired_current[i],
                                                 -config->max_current_command,
                                                 config->max_current_command);
        output->predicted_unlimited_power_w[i] =
            PowerModelPredict(&models[i], desired_current, input->speed_rpm[i]);
        if (!isfinite(output->predicted_unlimited_power_w[i]))
        {
            ResetOutput(output, POWER_MODEL_STATUS_NUMERIC_ERROR);
            return false;
        }

        output->current_scale[i] = 1.0f;
        if (output->predicted_unlimited_power_w[i] > output->allocated_power_w[i])
        {
            output->current_scale[i] = CalculateCurrentScale(&models[i],
                                                             desired_current,
                                                             input->speed_rpm[i],
                                                             output->allocated_power_w[i]);
        }
        output->limited_current[i] = desired_current * output->current_scale[i];
        output->predicted_limited_power_w[i] =
            PowerModelPredict(&models[i], output->limited_current[i], input->speed_rpm[i]);
        if (!isfinite(output->limited_current[i]) ||
            !isfinite(output->predicted_limited_power_w[i]))
        {
            ResetOutput(output, POWER_MODEL_STATUS_NUMERIC_ERROR);
            return false;
        }
    }

    return true;
}

bool ChassisPowerTelemetryBuild(const ChassisPowerInput_s *input,
                                const ChassisPowerOutput_s *power,
                                const bool output_enabled[POWER_MODEL_MOTOR_COUNT],
                                ChassisPowerTelemetry_s *telemetry)
{
    if (telemetry == NULL)
        return false;

    memset(telemetry, 0, sizeof(*telemetry));
    if (input == NULL || power == NULL || output_enabled == NULL ||
        power->status != POWER_MODEL_STATUS_OK)
        return false;

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        const float target_speed_rpm = input->speed_rpm[i] + input->speed_error_rpm[i];
        if (!isfinite(power->limited_current[i]) || !isfinite(input->speed_rpm[i]) ||
            !isfinite(target_speed_rpm) || !isfinite(power->predicted_limited_power_w[i]))
        {
            memset(telemetry, 0, sizeof(*telemetry));
            return false;
        }

        telemetry->current_command[i] =
            output_enabled[i] ? power->limited_current[i] : 0.0f;
        telemetry->actual_speed_rpm[i] = input->speed_rpm[i];
        telemetry->target_speed_rpm[i] = target_speed_rpm;
        telemetry->predicted_power_w[i] =
            output_enabled[i] ? power->predicted_limited_power_w[i] : 0.0f;
        telemetry->predicted_total_power_w += telemetry->predicted_power_w[i];
    }

    if (!isfinite(telemetry->predicted_total_power_w))
    {
        memset(telemetry, 0, sizeof(*telemetry));
        return false;
    }

    return true;
}

static int32_t RoundTelemetryValue(float value)
{
    if (!isfinite(value))
        return 0;
    if (value >= (float)INT32_MAX)
        return INT32_MAX;
    if (value <= (float)INT32_MIN)
        return INT32_MIN;
    return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static int32_t PowerToCentiwatts(float power_w)
{
    if (!isfinite(power_w))
        return 0;
    if (power_w >= (float)INT32_MAX / 100.0f)
        return INT32_MAX;
    if (power_w <= (float)INT32_MIN / 100.0f)
        return INT32_MIN;
    return RoundTelemetryValue(power_w * 100.0f);
}

size_t ChassisPowerTelemetryFormatCsv(const ChassisPowerTelemetry_s *telemetry,
                                      uint32_t tick_ms,
                                      float measured_chassis_power_w,
                                      char *buffer,
                                      size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0U)
        return 0U;
    buffer[0] = '\0';
    if (telemetry == NULL)
        return 0U;

    const int written = snprintf(
        buffer,
        buffer_size,
        "%lu,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
        (unsigned long)tick_ms,
        (long)RoundTelemetryValue(telemetry->current_command[0]),
        (long)RoundTelemetryValue(telemetry->current_command[1]),
        (long)RoundTelemetryValue(telemetry->current_command[2]),
        (long)RoundTelemetryValue(telemetry->current_command[3]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[0]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[1]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[2]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[3]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[0]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[1]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[2]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[3]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[0]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[1]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[2]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[3]),
        (long)PowerToCentiwatts(telemetry->predicted_total_power_w),
        (long)PowerToCentiwatts(measured_chassis_power_w));

    if (written < 0 || (size_t)written >= buffer_size)
    {
        buffer[0] = '\0';
        return 0U;
    }
    return (size_t)written;
}
