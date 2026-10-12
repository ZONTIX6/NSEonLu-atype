#include "chassis_power_budget.h"
#include <math.h>
#include <stdio.h>

static int failures;
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
#define NEAR(want, got) CHECK(isfinite(got) && fabsf((want) - (got)) < 0.01f)

static const ChassisPowerBudgetConfig_s config = {
    .empty_energy_ratio = 0.1f, .full_energy_ratio = 0.3f,
    .max_boost_w = 100, .rise_rate_w_per_s = 200,
    .buffer = {.max_energy_j = 60, .recovery_enter_j = 15,
        .recovery_exit_j = 30, .hard_floor_j = 5,
        .prediction_window_s = 0.5f, .guard_window_s = 0.3f,
        .settlement_delay_s = 0.1f, .energy_margin_j = 3,
        .power_margin_w = 5, .max_boost_w = 60,
        .recovery_power_w = 20, .recovery_hold_s = 0.1f,
        .rise_rate_w_per_s = 600, .safety_factor = 0.98f},
};

static ChassisPowerBudgetInput_s Input(float energy)
{
    const ChassisPowerBudgetInput_s input = {
        .referee_limit_w = 80, .buffer_control_enabled = true,
        .buffer_data_valid = true, .buffer_energy_j = energy,
        .input_power_valid = true, .measured_input_power_w = 80,
    };
    return input;
}

static ChassisPowerBudgetOutput_s Update(ChassisPowerBudgetInput_s input)
{
    ChassisPowerBudgetState_s state;
    ChassisPowerBudgetOutput_s output = {0};
    ChassisPowerBudgetReset(&state, 80);
    CHECK(ChassisPowerBudgetUpdate(&config, &input, 1, &state, &output));
    return output;
}

static void HighBufferWorksWithoutCap(void)
{
    ChassisPowerBudgetOutput_s out = Update(Input(60));
    NEAR(140, out.target_budget_w);
    NEAR(137.2f, out.input_target_w);
    CHECK(!out.recovering);
}

static void LowBufferActuallyReducesBudget(void)
{
    ChassisPowerBudgetOutput_s out = Update(Input(20));
    NEAR(60, out.applied_budget_w);
    NEAR(58.8f, out.input_target_w);
    CHECK(out.recovering);
}

static void MissingFeedbackRemovesBoost(void)
{
    ChassisPowerBudgetInput_s input = Input(60);
    input.input_power_valid = false;
    NEAR(80, Update(input).applied_budget_w);
    input.input_power_valid = true;
    input.buffer_data_valid = false;
    NEAR(80, Update(input).applied_budget_w);
}

static void CapBudgetCountsBoardTotalOnlyOnce(void)
{
    ChassisPowerBudgetInput_s input = Input(60);
    input.use_supercap = true;
    input.cap_online = input.cap_output_enabled = true;
    input.cap_input_control_ready = true;
    input.cap_energy_ratio = 0.5f;
    input.cap_reported_limit_w = 180;
    ChassisPowerBudgetOutput_s out = Update(input);
    CHECK(out.applied_budget_w * config.buffer.safety_factor <= 180.01f);
    CHECK(out.applied_budget_w > 140);
    NEAR(137.2f, out.input_target_w);
    input.cap_input_control_ready = false;
    NEAR(80, Update(input).applied_budget_w);
}

static void PredictionInterruptsHighConsumption(void)
{
    ChassisPowerBudgetInput_s input = Input(30);
    input.measured_input_power_w = 140;
    ChassisPowerBudgetOutput_s out = Update(input);
    CHECK(out.recovering);
    CHECK(out.applied_budget_w < 80);
}

static void RecoveryHasHysteresisAndImmediateDownwardLimit(void)
{
    ChassisPowerBudgetState_s state;
    ChassisPowerBudgetOutput_s out = {0};
    ChassisPowerBudgetInput_s input = Input(60);
    ChassisPowerBudgetReset(&state, 80);
    CHECK(ChassisPowerBudgetUpdate(&config, &input, 0.005f, &state, &out));
    NEAR(83, out.applied_budget_w);
    input = Input(20);
    CHECK(ChassisPowerBudgetUpdate(&config, &input, 0.005f, &state, &out));
    NEAR(60, out.applied_budget_w);
    input = Input(35);
    CHECK(ChassisPowerBudgetUpdate(&config, &input, 1, &state, &out));
    CHECK(out.recovering);
    NEAR(60, out.applied_budget_w);
    input = Input(50);
    CHECK(ChassisPowerBudgetUpdate(&config, &input, 0.05f, &state, &out));
    CHECK(out.recovering);
    CHECK(ChassisPowerBudgetUpdate(&config, &input, 0.05f, &state, &out));
    CHECK(!out.recovering);
    NEAR(90, out.applied_budget_w);
}

static void HealthyCapCanAssistWhileBufferRecovers(void)
{
    ChassisPowerBudgetInput_s input = Input(20);
    input.use_supercap = true;
    input.cap_online = input.cap_output_enabled = true;
    input.cap_input_control_ready = true;
    input.cap_energy_ratio = 0.5f;
    input.cap_reported_limit_w = 180;
    ChassisPowerBudgetOutput_s out = Update(input);
    CHECK(out.recovering);
    NEAR(58.8f, out.input_target_w);
    NEAR(160, out.applied_budget_w);
    input.cap_energy_ratio = 0.05f;
    NEAR(60, Update(input).applied_budget_w);
    input.cap_error_code = 1;
    NEAR(60, Update(input).applied_budget_w);
}

static void OldFramesAndInvalidNumbersCannotProvideFullBoost(void)
{
    ChassisPowerBudgetInput_s input = Input(60);
    input.buffer_age_s = 0.2f;
    NEAR(125, Update(input).target_budget_w);
    input.buffer_energy_j = NAN;
    NEAR(80, Update(input).applied_budget_w);
    input = Input(60);
    input.measured_input_power_w = INFINITY;
    NEAR(80, Update(input).applied_budget_w);
}

static void BufferSpendingAndRecoveryWithDelayedPowerResponse(void)
{
    ChassisPowerBudgetState_s state;
    ChassisPowerBudgetOutput_s out;
    ChassisPowerBudgetInput_s input = Input(60);
    ChassisPowerBudgetReset(&state, 80);
    float energy = 60, actual_power = 80, minimum = 60, peak = 0;
    float sample_energy = 60;
    for (int tick = 0; tick < 1200; ++tick)
    {
        if (tick % 20 == 0)
            sample_energy = floorf(energy);
        input.buffer_energy_j = sample_energy;
        input.buffer_age_s = (float)(tick % 20) * 0.005f;
        input.measured_input_power_w = actual_power;
        CHECK(ChassisPowerBudgetUpdate(&config, &input, 0.005f, &state, &out));
        const float demand = tick < 400 ? 180 : 30;
        const float requested = fminf(demand, out.applied_budget_w * 0.98f);
        /* Independent plant: 20 ms power response, 10 Hz integer energy frames. */
        actual_power += 0.25f * (requested - actual_power);
        energy = fminf(60, energy + (80 - actual_power) * 0.005f);
        minimum = fminf(minimum, energy);
        peak = fmaxf(peak, actual_power);
    }
    CHECK(peak > 120);
    CHECK(minimum > 5 && minimum < 45);
    CHECK(energy > 59);
}

int main(void)
{
    HighBufferWorksWithoutCap();
    LowBufferActuallyReducesBudget();
    MissingFeedbackRemovesBoost();
    CapBudgetCountsBoardTotalOnlyOnce();
    PredictionInterruptsHighConsumption();
    RecoveryHasHysteresisAndImmediateDownwardLimit();
    HealthyCapCanAssistWhileBufferRecovers();
    OldFramesAndInvalidNumbersCannotProvideFullBoost();
    BufferSpendingAndRecoveryWithDelayedPowerResponse();
    if (failures) return 1;
    puts("PASS: dynamic buffer budget tests");
    return 0;
}
