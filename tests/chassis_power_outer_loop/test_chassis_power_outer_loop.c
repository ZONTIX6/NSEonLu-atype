#include "chassis_power_outer_loop.h"

#include <math.h>
#include <stdio.h>

static int failures;

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
        if (expected_ != actual_)                                                          \
        {                                                                                 \
            printf("FAIL %s:%d: expected %d, got %d\n",                                  \
                   __FILE__, __LINE__, expected_, actual_);                                \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

#define ASSERT_NEAR(expected, actual, tolerance)                                           \
    do                                                                                    \
    {                                                                                     \
        const float expected_ = (expected);                                                \
        const float actual_ = (actual);                                                    \
        if (fabsf(expected_ - actual_) > (tolerance))                                      \
        {                                                                                 \
            printf("FAIL %s:%d: expected %.6f, got %.6f\n",                              \
                   __FILE__, __LINE__, (double)expected_, (double)actual_);                \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

static const ChassisPowerOuterLoopConfig_s valid_config = {
    .kp = 0.5f,
    .ki = 1.0f,
    .measurement_lpf_tau_s = 0.030f,
    .activation_margin_w = 5.0f,
    .error_deadband_w = 1.0f,
    .max_positive_correction_w = 8.0f,
    .positive_slew_rate_w_per_s = 20.0f,
    .budget_drop_reset_w = 1.0f,
};

static ChassisPowerOuterLoopInput_s BaseInput(void)
{
    const ChassisPowerOuterLoopInput_s input = {
        .nominal_budget_w = 60.0f,
        .target_power_w = 57.0f,
        .measured_power_w = 20.0f,
        .predicted_unlimited_power_w = 20.0f,
        .meter_online = true,
        .allow_positive_correction = true,
        .control_enabled = true,
    };
    return input;
}

static void AssertStateIsReset(const ChassisPowerOuterLoopState_s *state)
{
    ASSERT_NEAR(0.0f, state->filtered_power_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, state->integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, state->correction_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, state->previous_nominal_budget_w, 1.0e-5f);
    ASSERT_FALSE(state->measurement_initialized);
}

static void AssertOutputFailsClosed(const ChassisPowerOuterLoopOutput_s *output)
{
    ASSERT_NEAR(0.0f, output->corrected_budget_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output->correction_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output->filtered_power_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output->error_w, 1.0e-5f);
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output->mode);
}

static void TestResetClearsEveryStateField(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 40.0f,
        .integral_w = 3.0f,
        .correction_w = -4.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };

    ChassisPowerOuterLoopReset(&state);

    AssertStateIsReset(&state);
    ChassisPowerOuterLoopReset(NULL);
}

static void TestOfflineMeterPassesNominalBudgetFromReset(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.meter_online = false;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_FALSE(state.measurement_initialized);
    ASSERT_NEAR(60.0f, state.previous_nominal_budget_w, 1.0e-5f);
}

static void TestFirstOnlineSampleSeedsFilterWithoutZeroRamp(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_STANDBY, output.mode);
    ASSERT_NEAR(20.0f, output.filtered_power_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_TRUE(state.measurement_initialized);
}

static void TestSubsequentOnlineSampleUsesLowPassFilter(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.010f,
                                            &state, &output));

    input.measured_power_w = 40.0f;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.010f,
                                            &state, &output));

    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_STANDBY, output.mode);
    ASSERT_NEAR(25.0f, output.filtered_power_w, 1.0e-5f);
}

static void TestLowDemandStaysStandbyAndDoesNotAccumulateCorrection(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 20.0f,
        .integral_w = 3.0f,
        .correction_w = -1.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_STANDBY, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(-0.9f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(59.1f, output.corrected_budget_w, 1.0e-5f);

    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                                &state, &output));
    }
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestDisabledControlResetsStateAndOutputsZeroBudget(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 40.0f,
        .integral_w = 3.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output = {
        .corrected_budget_w = 60.0f,
        .correction_w = 2.0f,
        .filtered_power_w = 40.0f,
        .error_w = 17.0f,
        .mode = CHASSIS_POWER_OUTER_ACTIVE,
    };
    ChassisPowerOuterLoopInput_s input = BaseInput();
    input.control_enabled = false;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_STANDBY, output.mode);
    ASSERT_NEAR(0.0f, output.corrected_budget_w, 1.0e-5f);
    ASSERT_FALSE(state.measurement_initialized);
    AssertStateIsReset(&state);
}

static void AssertInvalidInputFailsClosed(ChassisPowerOuterLoopInput_s input)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 40.0f,
        .integral_w = 3.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output = {
        .corrected_budget_w = 60.0f,
        .correction_w = 2.0f,
        .filtered_power_w = 40.0f,
        .error_w = 17.0f,
        .mode = CHASSIS_POWER_OUTER_ACTIVE,
    };

    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                             &state, &output));
    AssertStateIsReset(&state);
    AssertOutputFailsClosed(&output);
}

static void TestInvalidBudgetFailsClosed(void)
{
    ChassisPowerOuterLoopInput_s input = BaseInput();
    input.nominal_budget_w = 0.0f;
    AssertInvalidInputFailsClosed(input);
    input.nominal_budget_w = -1.0f;
    AssertInvalidInputFailsClosed(input);
    input.nominal_budget_w = NAN;
    AssertInvalidInputFailsClosed(input);

    input = BaseInput();
    input.target_power_w = 0.0f;
    AssertInvalidInputFailsClosed(input);
    input.target_power_w = -1.0f;
    AssertInvalidInputFailsClosed(input);
    input.target_power_w = INFINITY;
    AssertInvalidInputFailsClosed(input);

    input = BaseInput();
    input.predicted_unlimited_power_w = -1.0f;
    AssertInvalidInputFailsClosed(input);
    input.predicted_unlimited_power_w = NAN;
    AssertInvalidInputFailsClosed(input);
}

static void AssertInvalidConfigFailsClosed(ChassisPowerOuterLoopConfig_s config)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 40.0f,
        .integral_w = 3.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output = {
        .corrected_budget_w = 60.0f,
        .correction_w = 2.0f,
        .filtered_power_w = 40.0f,
        .error_w = 17.0f,
        .mode = CHASSIS_POWER_OUTER_ACTIVE,
    };
    ChassisPowerOuterLoopInput_s input = BaseInput();

    ASSERT_FALSE(ChassisPowerOuterLoopConfigIsValid(&config));
    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&config, &input, 0.005f,
                                             &state, &output));
    AssertStateIsReset(&state);
    AssertOutputFailsClosed(&output);
}

static void TestInvalidConfigFailsClosed(void)
{
    ChassisPowerOuterLoopConfig_s config = valid_config;
    ASSERT_TRUE(ChassisPowerOuterLoopConfigIsValid(&valid_config));
    ASSERT_FALSE(ChassisPowerOuterLoopConfigIsValid(NULL));

    config.kp = -1.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.ki = -1.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.measurement_lpf_tau_s = 0.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.activation_margin_w = -1.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.error_deadband_w = -1.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.max_positive_correction_w = 0.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.positive_slew_rate_w_per_s = 0.0f;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.budget_drop_reset_w = -1.0f;
    AssertInvalidConfigFailsClosed(config);

    config = valid_config;
    config.kp = NAN;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.ki = INFINITY;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.measurement_lpf_tau_s = NAN;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.activation_margin_w = NAN;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.error_deadband_w = NAN;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.max_positive_correction_w = NAN;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.positive_slew_rate_w_per_s = NAN;
    AssertInvalidConfigFailsClosed(config);
    config = valid_config;
    config.budget_drop_reset_w = NAN;
    AssertInvalidConfigFailsClosed(config);
}

static void TestInvalidPointersFailSafely(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 40.0f,
        .integral_w = 3.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output = {
        .corrected_budget_w = 60.0f,
        .correction_w = 2.0f,
        .filtered_power_w = 40.0f,
        .error_w = 17.0f,
        .mode = CHASSIS_POWER_OUTER_ACTIVE,
    };
    ChassisPowerOuterLoopInput_s input = BaseInput();

    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(NULL, &input, 0.005f, &state, &output));
    AssertStateIsReset(&state);
    AssertOutputFailsClosed(&output);

    state.measurement_initialized = true;
    output.corrected_budget_w = 60.0f;
    output.mode = CHASSIS_POWER_OUTER_ACTIVE;
    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, NULL, 0.005f,
                                             &state, &output));
    AssertStateIsReset(&state);
    AssertOutputFailsClosed(&output);

    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                             NULL, &output));
    AssertOutputFailsClosed(&output);
    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                             &state, NULL));
}

static void TestOverPowerImmediatelyReducesBudget(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 62.0f;
    input.predicted_unlimited_power_w = 70.0f;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_ACTIVE, output.mode);
    ASSERT_NEAR(-2.525f, output.correction_w, 1.0e-4f);
    ASSERT_NEAR(57.475f, output.corrected_budget_w, 1.0e-4f);
}

static void TestPositiveCorrectionIsSlewLimited(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_NEAR(0.1f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.1f, output.corrected_budget_w, 1.0e-5f);
}

static void TestDeadbandProducesNoCorrection(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 57.5f;
    input.predicted_unlimited_power_w = 70.0f;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_ACTIVE, output.mode);
    ASSERT_NEAR(0.0f, output.error_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
}

static void TestPositiveCorrectionStopsAtEightWatts(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;

    for (int i = 0; i < 400; ++i)
    {
        ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                                &state, &output));
    }

    ASSERT_NEAR(8.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(68.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestLargeOverPowerCanReduceBudgetToZero(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 200.0f;
    input.predicted_unlimited_power_w = 200.0f;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_NEAR(-60.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestRawHardLimitBypassesFilteredMeasurement(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 40.0f;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    input.measured_power_w = 65.0f;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));

    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_ACTIVE, output.mode);
    ASSERT_NEAR(43.57143f, output.filtered_power_w, 1.0e-4f);
    ASSERT_NEAR(-8.0f, output.error_w, 1.0e-5f);
    ASSERT_NEAR(-4.04f, output.correction_w, 1.0e-4f);
}

static void TestBrakingCannotCreatePositiveCorrection(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;
    input.allow_positive_correction = false;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_ACTIVE, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestOfflineDropsPositiveCorrectionImmediately(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_NEAR(0.1f, output.correction_w, 1.0e-5f);

    input.meter_online = false;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestOfflineReleasesNegativeCorrectionAtPointOneWattPerTick(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 62.0f;
    input.predicted_unlimited_power_w = 70.0f;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_NEAR(-2.525f, output.correction_w, 1.0e-4f);

    input.meter_online = false;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(-2.425f, output.correction_w, 1.0e-4f);
    ASSERT_NEAR(57.575f, output.corrected_budget_w, 1.0e-4f);
}

static void TestNonfiniteMeasurementUsesOfflinePolicy(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 52.0f,
        .integral_w = 1.0f,
        .correction_w = 1.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    input.measured_power_w = NAN;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_FALSE(state.measurement_initialized);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestBudgetDropClearsPositiveState(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 47.5f,
        .integral_w = 2.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    input.nominal_budget_w = 50.0f;
    input.target_power_w = 47.5f;
    input.measured_power_w = 47.5f;
    input.predicted_unlimited_power_w = 70.0f;

    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_ACTIVE, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(50.0f, output.corrected_budget_w, 1.0e-5f);
}

static void TestSaturationDoesNotWindIntegralFurther(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;

    for (int i = 0; i < 400 && state.correction_w < 8.0f; ++i)
    {
        ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                                &state, &output));
    }
    ASSERT_NEAR(8.0f, state.correction_w, 1.0e-5f);
    const float integral_at_saturation_w = state.integral_w;

    for (int i = 0; i < 100; ++i)
    {
        ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                                &state, &output));
    }

    ASSERT_NEAR(integral_at_saturation_w, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(8.0f, output.correction_w, 1.0e-5f);
}

static void TestErrorCanDriveControllerOutOfSaturation(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;
    for (int i = 0; i < 400 && state.correction_w < 8.0f; ++i)
    {
        ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                                &state, &output));
    }
    const float saturated_integral_w = state.integral_w;

    input.measured_power_w = 65.0f;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));

    ASSERT_TRUE(output.correction_w < 8.0f);
    ASSERT_NEAR(saturated_integral_w - 0.04f, state.integral_w, 1.0e-4f);
}

static void TestCandidateSaturationDoesNotWindBeforeSlewCatchesUp(void)
{
    ChassisPowerOuterLoopState_s state;
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    ChassisPowerOuterLoopReset(&state);
    input.measured_power_w = 0.0f;
    input.predicted_unlimited_power_w = 70.0f;

    for (int i = 0; i < 20; ++i)
    {
        ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                                &state, &output));
    }
    ASSERT_NEAR(2.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);

    input.measured_power_w = 65.0f;
    ASSERT_TRUE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.005f,
                                            &state, &output));
    ASSERT_NEAR(-4.04f, output.correction_w, 1.0e-4f);
}

static void TestInvalidDeltaTimeFailsWithoutPositiveOutput(void)
{
    ChassisPowerOuterLoopState_s state = {
        .filtered_power_w = 52.0f,
        .integral_w = 2.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    ChassisPowerOuterLoopOutput_s output;
    ChassisPowerOuterLoopInput_s input = BaseInput();
    input.measured_power_w = 52.0f;
    input.predicted_unlimited_power_w = 70.0f;

    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, &input, 0.0f,
                                             &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
    ASSERT_TRUE(state.measurement_initialized);

    state.integral_w = -1.0f;
    state.correction_w = -2.5f;
    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, &input, NAN,
                                             &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_NEAR(-1.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(-2.5f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(57.5f, output.corrected_budget_w, 1.0e-5f);

    state = (ChassisPowerOuterLoopState_s){
        .filtered_power_w = 52.0f,
        .integral_w = 2.0f,
        .correction_w = 2.0f,
        .previous_nominal_budget_w = 60.0f,
        .measurement_initialized = true,
    };
    input.control_enabled = false;
    ASSERT_FALSE(ChassisPowerOuterLoopUpdate(&valid_config, &input, -0.005f,
                                             &state, &output));
    ASSERT_EQ_INT(CHASSIS_POWER_OUTER_OFFLINE, output.mode);
    ASSERT_NEAR(0.0f, state.integral_w, 1.0e-5f);
    ASSERT_NEAR(0.0f, output.correction_w, 1.0e-5f);
    ASSERT_NEAR(60.0f, output.corrected_budget_w, 1.0e-5f);
    ASSERT_TRUE(state.measurement_initialized);
}

int main(void)
{
    TestResetClearsEveryStateField();
    TestOfflineMeterPassesNominalBudgetFromReset();
    TestFirstOnlineSampleSeedsFilterWithoutZeroRamp();
    TestSubsequentOnlineSampleUsesLowPassFilter();
    TestLowDemandStaysStandbyAndDoesNotAccumulateCorrection();
    TestDisabledControlResetsStateAndOutputsZeroBudget();
    TestInvalidBudgetFailsClosed();
    TestInvalidConfigFailsClosed();
    TestInvalidPointersFailSafely();
    TestOverPowerImmediatelyReducesBudget();
    TestPositiveCorrectionIsSlewLimited();
    TestDeadbandProducesNoCorrection();
    TestPositiveCorrectionStopsAtEightWatts();
    TestLargeOverPowerCanReduceBudgetToZero();
    TestRawHardLimitBypassesFilteredMeasurement();
    TestBrakingCannotCreatePositiveCorrection();
    TestOfflineDropsPositiveCorrectionImmediately();
    TestOfflineReleasesNegativeCorrectionAtPointOneWattPerTick();
    TestNonfiniteMeasurementUsesOfflinePolicy();
    TestBudgetDropClearsPositiveState();
    TestSaturationDoesNotWindIntegralFurther();
    TestErrorCanDriveControllerOutOfSaturation();
    TestCandidateSaturationDoesNotWindBeforeSlewCatchesUp();
    TestInvalidDeltaTimeFailsWithoutPositiveOutput();

    if (failures != 0)
        return 1;

    puts("PASS: chassis power outer-loop tests");
    return 0;
}
