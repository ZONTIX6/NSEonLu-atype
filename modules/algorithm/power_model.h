#ifndef POWER_MODEL_H
#define POWER_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define POWER_MODEL_MOTOR_COUNT 4U
#define CHASSIS_POWER_TELEMETRY_CSV_MAX_LENGTH 256U

typedef enum
{
    POWER_MODEL_STATUS_OK = 0,
    POWER_MODEL_STATUS_INVALID_ARGUMENT,
    POWER_MODEL_STATUS_INVALID_CONFIG,
    POWER_MODEL_STATUS_INVALID_BUDGET,
    POWER_MODEL_STATUS_NUMERIC_ERROR,
} PowerModelStatus_e;

typedef struct
{
    float k0;
    float k1;
    float k2;
    float k3;
    float k4;
    float k5;
    float current_conversion;
} MotorPowerModelConfig_s;

typedef struct
{
    float safety_factor;
    float small_error_threshold_rpm;
    float reserved_power_threshold_w;
    float per_motor_reserved_power_w;
    float max_current_command;
} ChassisPowerAlgorithmConfig_s;

typedef struct
{
    float desired_current[POWER_MODEL_MOTOR_COUNT];
    float speed_rpm[POWER_MODEL_MOTOR_COUNT];
    float speed_error_rpm[POWER_MODEL_MOTOR_COUNT];
    float referee_power_limit_w;
    float attenuation;
} ChassisPowerInput_s;

typedef struct
{
    float effective_limit_w;
    float predicted_unlimited_power_w[POWER_MODEL_MOTOR_COUNT];
    float allocated_power_w[POWER_MODEL_MOTOR_COUNT];
    float current_scale[POWER_MODEL_MOTOR_COUNT];
    float limited_current[POWER_MODEL_MOTOR_COUNT];
    float predicted_limited_power_w[POWER_MODEL_MOTOR_COUNT];
    PowerModelStatus_e status;
} ChassisPowerOutput_s;

typedef struct
{
    float current_command[POWER_MODEL_MOTOR_COUNT];
    float actual_speed_rpm[POWER_MODEL_MOTOR_COUNT];
    float target_speed_rpm[POWER_MODEL_MOTOR_COUNT];
    float predicted_power_w[POWER_MODEL_MOTOR_COUNT];
    float predicted_total_power_w;
} ChassisPowerTelemetry_s;

bool PowerModelConfigIsValid(const MotorPowerModelConfig_s *config);
bool ChassisPowerAlgorithmConfigIsValid(const ChassisPowerAlgorithmConfig_s *config);
float PowerModelSelectLimit(float referee_limit_w, float fallback_limit_w);
float PowerModelPredict(const MotorPowerModelConfig_s *config,
                        float current_command,
                        float speed_rpm);
bool PowerModelApply(const MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT],
                      const ChassisPowerAlgorithmConfig_s *config,
                      const ChassisPowerInput_s *input,
                      ChassisPowerOutput_s *output);
bool ChassisPowerTelemetryBuild(const ChassisPowerInput_s *input,
                                const ChassisPowerOutput_s *power,
                                const bool output_enabled[POWER_MODEL_MOTOR_COUNT],
                                ChassisPowerTelemetry_s *telemetry);
size_t ChassisPowerTelemetryFormatCsv(const ChassisPowerTelemetry_s *telemetry,
                                      uint32_t tick_ms,
                                      float measured_chassis_power_w,
                                      char *buffer,
                                      size_t buffer_size);

#endif // POWER_MODEL_H
