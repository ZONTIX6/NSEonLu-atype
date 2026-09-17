#include "power_model.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define ASSERT_NEAR(expected, actual, tolerance)                                          \
    do                                                                                    \
    {                                                                                     \
        const float expected_ = (expected);                                               \
        const float actual_ = (actual);                                                   \
        if (fabsf(expected_ - actual_) > (tolerance))                                     \
        {                                                                                 \
            printf("FAIL %s:%d: expected %.6f, got %.6f\n",                              \
                   __FILE__, __LINE__, (double)expected_, (double)actual_);                \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

#define ASSERT_TRUE(condition)                                                            \
    do                                                                                    \
    {                                                                                     \
        if (!(condition))                                                                 \
        {                                                                                 \
            printf("FAIL %s:%d: expected true: %s\n", __FILE__, __LINE__, #condition);   \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ_INT(expected, actual)                                                    \
    do                                                                                    \
    {                                                                                     \
        const int expected_ = (expected);                                                  \
        const int actual_ = (actual);                                                      \
        if (expected_ != actual_)                                                         \
        {                                                                                 \
            printf("FAIL %s:%d: expected %d, got %d\n",                                  \
                   __FILE__, __LINE__, expected_, actual_);                               \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

#define ASSERT_STREQ(expected, actual)                                                     \
    do                                                                                    \
    {                                                                                     \
        if (strcmp((expected), (actual)) != 0)                                            \
        {                                                                                 \
            printf("FAIL %s:%d: expected '%s', got '%s'\n",                             \
                   __FILE__, __LINE__, (expected), (actual));                             \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

static const MotorPowerModelConfig_s m3508_model = {
    .k0 = 1.10671243f,
    .k1 = -0.275458501f,
    .k2 = 0.000465227105f,
    .k3 = 0.00166421676f,
    .k4 = 0.228857816f,
    .k5 = 1.11735923e-7f,
    .current_conversion = 1000.0f,
};

static const ChassisPowerAlgorithmConfig_s algorithm_config = {
    .safety_factor = 0.98f,
    .small_error_threshold_rpm = 500.0f,
    .reserved_power_threshold_w = 54.0f,
    .per_motor_reserved_power_w = 8.0f,
    .max_current_command = 15000.0f,
};

static void FillModels(MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT],
                       MotorPowerModelConfig_s model)
{
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
        models[i] = model;
}

static ChassisPowerInput_s ValidInput(void)
{
    ChassisPowerInput_s input = {
        .desired_current = {1000.0f, 1000.0f, 1000.0f, 1000.0f},
        .speed_rpm = {1000.0f, 1000.0f, 1000.0f, 1000.0f},
        .speed_error_rpm = {100.0f, 100.0f, 100.0f, 100.0f},
        .referee_power_limit_w = 100.0f,
        .attenuation = 1.0f,
    };
    return input;
}

static float Sum4(const float values[POWER_MODEL_MOTOR_COUNT])
{
    float sum = 0.0f;
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
        sum += values[i];
    return sum;
}

static void TestDisabledBenchFallbackPreservesInvalidLimit(void)
{
    ASSERT_NEAR(0.0f, PowerModelSelectLimit(0.0f, 0.0f), 1.0e-6f);
}

static void TestBenchFallbackReplacesInvalidRefereeLimit(void)
{
    ASSERT_NEAR(40.0f, PowerModelSelectLimit(NAN, 40.0f), 1.0e-6f);
}

static void TestRefereeLimitHasPriorityOverBenchFallback(void)
{
    ASSERT_NEAR(80.0f, PowerModelSelectLimit(80.0f, 40.0f), 1.0e-6f);
}

static void TestInvalidBenchFallbackIsIgnored(void)
{
    ASSERT_NEAR(0.0f, PowerModelSelectLimit(0.0f, -40.0f), 1.0e-6f);
}

static void TestPredictionMatchesRecalibratedM3508Sample(void)
{
    const float actual = PowerModelPredict(&m3508_model, 5000.0f, 3000.0f);
    ASSERT_NEAR(32.81542f, actual, 1.0e-4f);
}

static void TestSmallErrorEvenlySplitsBudget(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);

    ASSERT_TRUE(PowerModelApply(models, &algorithm_config, &input, &output));
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
        ASSERT_NEAR(24.5f, output.allocated_power_w[i], 1.0e-4f);
    ASSERT_NEAR(98.0f, Sum4(output.allocated_power_w), 1.0e-4f);
}

static void TestLargeErrorUsesReservedPower(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);
    input.speed_error_rpm[0] = 700.0f;

    ASSERT_TRUE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_NEAR(54.2f, output.allocated_power_w[0], 1.0e-3f);
    ASSERT_NEAR(14.6f, output.allocated_power_w[1], 1.0e-3f);
    ASSERT_NEAR(14.6f, output.allocated_power_w[2], 1.0e-3f);
    ASSERT_NEAR(14.6f, output.allocated_power_w[3], 1.0e-3f);
    ASSERT_NEAR(98.0f, Sum4(output.allocated_power_w), 1.0e-4f);
}

static void TestLowBudgetUsesPureErrorRatio(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);
    input.referee_power_limit_w = 50.0f;
    input.speed_error_rpm[0] = 700.0f;

    ASSERT_TRUE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_NEAR(34.3f, output.allocated_power_w[0], 1.0e-3f);
    ASSERT_NEAR(4.9f, output.allocated_power_w[1], 1.0e-3f);
    ASSERT_NEAR(49.0f, Sum4(output.allocated_power_w), 1.0e-4f);
}

static void TestNoLimitKeepsCurrent(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    const MotorPowerModelConfig_s zero_power_model = {.current_conversion = 1.0f};
    FillModels(models, zero_power_model);

    ASSERT_TRUE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_NEAR(1000.0f, output.limited_current[0], 1.0e-4f);
    ASSERT_NEAR(1.0f, output.current_scale[0], 1.0e-6f);
}

static void TestQuadraticLimitScalesCurrent(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    const MotorPowerModelConfig_s square_model = {
        .k4 = 1.0f,
        .current_conversion = 1.0f,
    };
    const ChassisPowerAlgorithmConfig_s config = {
        .safety_factor = 1.0f,
        .small_error_threshold_rpm = 500.0f,
        .reserved_power_threshold_w = 1000.0f,
        .per_motor_reserved_power_w = 0.0f,
        .max_current_command = 100.0f,
    };
    FillModels(models, square_model);
    input.desired_current[0] = 10.0f;
    input.desired_current[1] = 0.0f;
    input.desired_current[2] = 0.0f;
    input.desired_current[3] = 0.0f;
    input.speed_rpm[0] = 0.0f;
    input.referee_power_limit_w = 100.0f;

    ASSERT_TRUE(PowerModelApply(models, &config, &input, &output));
    ASSERT_NEAR(5.0f, output.limited_current[0], 1.0e-4f);
    ASSERT_NEAR(0.5f, output.current_scale[0], 1.0e-5f);
    ASSERT_NEAR(25.0f, output.predicted_limited_power_w[0], 1.0e-3f);
}

static void TestLinearBranchActuallyScalesCurrent(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    const MotorPowerModelConfig_s linear_model = {
        .k1 = 2.0f,
        .current_conversion = 1.0f,
    };
    const ChassisPowerAlgorithmConfig_s config = {
        .safety_factor = 1.0f,
        .small_error_threshold_rpm = 500.0f,
        .reserved_power_threshold_w = 1000.0f,
        .per_motor_reserved_power_w = 0.0f,
        .max_current_command = 100.0f,
    };
    FillModels(models, linear_model);
    input.desired_current[0] = 10.0f;
    input.desired_current[1] = 0.0f;
    input.desired_current[2] = 0.0f;
    input.desired_current[3] = 0.0f;
    input.speed_rpm[0] = 0.0f;
    input.referee_power_limit_w = 40.0f;

    ASSERT_TRUE(PowerModelApply(models, &config, &input, &output));
    ASSERT_NEAR(5.0f, output.limited_current[0], 1.0e-4f);
    ASSERT_NEAR(0.5f, output.current_scale[0], 1.0e-5f);
}

static void TestNegativeDiscriminantZerosCurrent(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    const MotorPowerModelConfig_s impossible_model = {
        .k0 = 10.0f,
        .k4 = 1.0f,
        .current_conversion = 1.0f,
    };
    const ChassisPowerAlgorithmConfig_s config = {
        .safety_factor = 1.0f,
        .small_error_threshold_rpm = 500.0f,
        .reserved_power_threshold_w = 1000.0f,
        .per_motor_reserved_power_w = 0.0f,
        .max_current_command = 100.0f,
    };
    FillModels(models, impossible_model);
    input.desired_current[0] = 10.0f;
    input.referee_power_limit_w = 20.0f;

    ASSERT_TRUE(PowerModelApply(models, &config, &input, &output));
    ASSERT_NEAR(0.0f, output.limited_current[0], 1.0e-6f);
    ASSERT_NEAR(0.0f, output.current_scale[0], 1.0e-6f);
}

static void TestCurrentCommandIsHardClamped(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    const MotorPowerModelConfig_s zero_power_model = {.current_conversion = 1.0f};
    FillModels(models, zero_power_model);
    input.desired_current[0] = 20000.0f;

    ASSERT_TRUE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_NEAR(15000.0f, output.limited_current[0], 1.0e-4f);
}

static void TestAttenuationAboveOneIsClamped(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);
    input.attenuation = 2.0f;

    ASSERT_TRUE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_NEAR(98.0f, output.effective_limit_w, 1.0e-4f);
}

static void TestInvalidBudgetFailsSafe(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);
    input.referee_power_limit_w = 0.0f;

    ASSERT_FALSE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_EQ_INT(POWER_MODEL_STATUS_INVALID_BUDGET, output.status);
    ASSERT_NEAR(0.0f, Sum4(output.limited_current), 1.0e-6f);
}

static void TestNanInputFailsSafe(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);
    input.desired_current[2] = NAN;

    ASSERT_FALSE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_EQ_INT(POWER_MODEL_STATUS_NUMERIC_ERROR, output.status);
    ASSERT_NEAR(0.0f, Sum4(output.limited_current), 1.0e-6f);
}

static void TestInvalidConfigurationIsRejected(void)
{
    MotorPowerModelConfig_s models[POWER_MODEL_MOTOR_COUNT];
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s output;
    FillModels(models, m3508_model);
    models[1].current_conversion = 0.0f;

    ASSERT_FALSE(PowerModelApply(models, &algorithm_config, &input, &output));
    ASSERT_EQ_INT(POWER_MODEL_STATUS_INVALID_CONFIG, output.status);
}

static void TestTelemetryBuildsCoherentSnapshot(void)
{
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s power = {.status = POWER_MODEL_STATUS_OK};
    ChassisPowerTelemetry_s telemetry;
    const bool output_enabled[POWER_MODEL_MOTOR_COUNT] = {true, true, true, true};

    input.speed_rpm[0] = 1000.0f;
    input.speed_rpm[1] = 1100.0f;
    input.speed_rpm[2] = 1200.0f;
    input.speed_rpm[3] = 1300.0f;
    input.speed_error_rpm[0] = 10.0f;
    input.speed_error_rpm[1] = 11.0f;
    input.speed_error_rpm[2] = 12.0f;
    input.speed_error_rpm[3] = 13.0f;
    power.limited_current[0] = 2000.0f;
    power.limited_current[1] = 2100.0f;
    power.limited_current[2] = 2200.0f;
    power.limited_current[3] = 2300.0f;
    power.predicted_limited_power_w[0] = 10.0f;
    power.predicted_limited_power_w[1] = 11.0f;
    power.predicted_limited_power_w[2] = 12.0f;
    power.predicted_limited_power_w[3] = 13.0f;

    ASSERT_TRUE(ChassisPowerTelemetryBuild(&input, &power, output_enabled, &telemetry));
    ASSERT_NEAR(46.0f, telemetry.predicted_total_power_w, 1.0e-6f);
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        ASSERT_NEAR(power.limited_current[i], telemetry.current_command[i], 1.0e-6f);
        ASSERT_NEAR(input.speed_rpm[i], telemetry.actual_speed_rpm[i], 1.0e-6f);
        ASSERT_NEAR(input.speed_rpm[i] + input.speed_error_rpm[i],
                    telemetry.target_speed_rpm[i],
                    1.0e-6f);
        ASSERT_NEAR(power.predicted_limited_power_w[i],
                    telemetry.predicted_power_w[i],
                    1.0e-6f);
    }
}

static void TestTelemetryRejectsInvalidPowerAndClearsOutput(void)
{
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s power = {.status = POWER_MODEL_STATUS_INVALID_BUDGET};
    ChassisPowerTelemetry_s telemetry;
    const bool output_enabled[POWER_MODEL_MOTOR_COUNT] = {true, true, true, true};
    memset(&telemetry, 0xA5, sizeof(telemetry));

    ASSERT_FALSE(ChassisPowerTelemetryBuild(&input, &power, output_enabled, &telemetry));
    const unsigned char *bytes = (const unsigned char *)&telemetry;
    for (size_t i = 0; i < sizeof(telemetry); ++i)
        ASSERT_TRUE(bytes[i] == 0U);
}

static void TestTelemetryClearsCommandsDisabledByFinalOutputGate(void)
{
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s power = {.status = POWER_MODEL_STATUS_OK};
    ChassisPowerTelemetry_s telemetry;
    const bool output_enabled[POWER_MODEL_MOTOR_COUNT] = {true, false, true, true};

    power.limited_current[0] = 2000.0f;
    power.limited_current[1] = 2100.0f;
    power.limited_current[2] = 2200.0f;
    power.limited_current[3] = 2300.0f;
    power.predicted_limited_power_w[0] = 10.0f;
    power.predicted_limited_power_w[1] = 11.0f;
    power.predicted_limited_power_w[2] = 12.0f;
    power.predicted_limited_power_w[3] = 13.0f;

    ASSERT_TRUE(ChassisPowerTelemetryBuild(&input, &power, output_enabled, &telemetry));
    ASSERT_NEAR(0.0f, telemetry.current_command[1], 1.0e-6f);
    ASSERT_NEAR(0.0f, telemetry.predicted_power_w[1], 1.0e-6f);
    ASSERT_NEAR(35.0f, telemetry.predicted_total_power_w, 1.0e-6f);
}

static void TestTelemetryFormatsOneCompleteCsvRow(void)
{
    ChassisPowerInput_s input = ValidInput();
    ChassisPowerOutput_s power = {.status = POWER_MODEL_STATUS_OK};
    ChassisPowerTelemetry_s telemetry;
    char row[CHASSIS_POWER_TELEMETRY_CSV_MAX_LENGTH];
    const bool output_enabled[POWER_MODEL_MOTOR_COUNT] = {true, true, true, true};

    input.speed_rpm[0] = 1000.0f;
    input.speed_rpm[1] = 1100.0f;
    input.speed_rpm[2] = 1200.0f;
    input.speed_rpm[3] = 1300.0f;
    input.speed_error_rpm[0] = 10.0f;
    input.speed_error_rpm[1] = 11.0f;
    input.speed_error_rpm[2] = 12.0f;
    input.speed_error_rpm[3] = 13.0f;
    power.limited_current[0] = 2000.0f;
    power.limited_current[1] = 2100.0f;
    power.limited_current[2] = 2200.0f;
    power.limited_current[3] = 2300.0f;
    power.predicted_limited_power_w[0] = 10.0f;
    power.predicted_limited_power_w[1] = 11.0f;
    power.predicted_limited_power_w[2] = 12.0f;
    power.predicted_limited_power_w[3] = 13.0f;
    ASSERT_TRUE(ChassisPowerTelemetryBuild(&input, &power, output_enabled, &telemetry));

    ASSERT_TRUE(ChassisPowerTelemetryFormatCsv(&telemetry,
                                               123U,
                                               12.34f,
                                               row,
                                               sizeof(row)) > 0U);
    ASSERT_STREQ("123,2000,2100,2200,2300,1000,1100,1200,1300,"
                 "1010,1111,1212,1313,1000,1100,1200,1300,4600,1234\r\n",
                 row);
}

static void TestTelemetryCsvPreservesNegativeMeasuredPower(void)
{
    ChassisPowerTelemetry_s telemetry = {0};
    char row[CHASSIS_POWER_TELEMETRY_CSV_MAX_LENGTH];

    ASSERT_TRUE(ChassisPowerTelemetryFormatCsv(&telemetry,
                                               1U,
                                               -1.25f,
                                               row,
                                               sizeof(row)) > 0U);
    ASSERT_STREQ("1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-125\r\n", row);
}

static void TestTelemetryCsvNormalizesNonfiniteMeasuredPower(void)
{
    ChassisPowerTelemetry_s telemetry = {0};
    char row[CHASSIS_POWER_TELEMETRY_CSV_MAX_LENGTH];

    ASSERT_TRUE(ChassisPowerTelemetryFormatCsv(&telemetry,
                                               1U,
                                               NAN,
                                               row,
                                               sizeof(row)) > 0U);
    ASSERT_STREQ("1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\r\n", row);
}

static void TestTelemetryCsvSaturatesMeasuredPower(void)
{
    ChassisPowerTelemetry_s telemetry = {0};
    char row[CHASSIS_POWER_TELEMETRY_CSV_MAX_LENGTH];

    ASSERT_TRUE(ChassisPowerTelemetryFormatCsv(&telemetry,
                                               1U,
                                               FLT_MAX,
                                               row,
                                               sizeof(row)) > 0U);
    ASSERT_STREQ("1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2147483647\r\n", row);
}

static void TestTelemetryCsvRejectsTruncatedRow(void)
{
    ChassisPowerTelemetry_s telemetry = {0};
    char row[8] = "dirty";

    ASSERT_TRUE(ChassisPowerTelemetryFormatCsv(&telemetry,
                                               123U,
                                               0.0f,
                                               row,
                                               sizeof(row)) == 0U);
    ASSERT_TRUE(row[0] == '\0');
}

static void TestTelemetryCsvSaturatesLargePower(void)
{
    ChassisPowerTelemetry_s telemetry = {0};
    char row[CHASSIS_POWER_TELEMETRY_CSV_MAX_LENGTH];
    telemetry.predicted_power_w[0] = FLT_MAX;

    ASSERT_TRUE(ChassisPowerTelemetryFormatCsv(&telemetry,
                                               1U,
                                               0.0f,
                                               row,
                                               sizeof(row)) > 0U);
    ASSERT_STREQ("1,0,0,0,0,0,0,0,0,0,0,0,0,2147483647,0,0,0,0,0\r\n", row);
}

int main(void)
{
    TestDisabledBenchFallbackPreservesInvalidLimit();
    TestBenchFallbackReplacesInvalidRefereeLimit();
    TestRefereeLimitHasPriorityOverBenchFallback();
    TestInvalidBenchFallbackIsIgnored();
    TestPredictionMatchesRecalibratedM3508Sample();
    TestSmallErrorEvenlySplitsBudget();
    TestLargeErrorUsesReservedPower();
    TestLowBudgetUsesPureErrorRatio();
    TestNoLimitKeepsCurrent();
    TestQuadraticLimitScalesCurrent();
    TestLinearBranchActuallyScalesCurrent();
    TestNegativeDiscriminantZerosCurrent();
    TestCurrentCommandIsHardClamped();
    TestAttenuationAboveOneIsClamped();
    TestInvalidBudgetFailsSafe();
    TestNanInputFailsSafe();
    TestInvalidConfigurationIsRejected();
    TestTelemetryBuildsCoherentSnapshot();
    TestTelemetryRejectsInvalidPowerAndClearsOutput();
    TestTelemetryClearsCommandsDisabledByFinalOutputGate();
    TestTelemetryFormatsOneCompleteCsvRow();
    TestTelemetryCsvPreservesNegativeMeasuredPower();
    TestTelemetryCsvNormalizesNonfiniteMeasuredPower();
    TestTelemetryCsvSaturatesMeasuredPower();
    TestTelemetryCsvRejectsTruncatedRow();
    TestTelemetryCsvSaturatesLargePower();
    if (failures != 0)
        return 1;

    puts("PASS: power model tests");
    return 0;
}
