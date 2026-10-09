#include "chassis_power_transient.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ChassisPowerTransientConfig_s config;
static ChassisPowerTransientState_s state;
static ChassisPowerTransientInput_s input;
static ChassisPowerTransientOutput_s output;
static void Prepare(void)
{
    config = ChassisPowerTransientDefaultConfig();
    ChassisPowerTransientReset(&state);
    input = (ChassisPowerTransientInput_s){.budget_w = 20, .target_w = 19, .slow_w = 3,
        .meter_enabled = true, .meter_online = true, .control_enabled = true,
        .online_mask = 15, .enabled_mask = 15, .now_ms = 100, .meter_tick_ms = 100};
    for (unsigned i = 0; i < 4; ++i) {
        input.raw_current[i] = 2000; input.actual_rpm[i] = 1000; input.target_rpm[i] = 1000;
        input.feedback_tick_ms[i] = 100; input.feedback_sequence[i] = 1;
    }
}
static void Tick(uint32_t tick, bool new_meter)
{
    input.now_ms = tick;
    if (new_meter) input.meter_tick_ms = tick;
    for (unsigned i = 0; i < 4; ++i) input.feedback_tick_ms[i] = tick;
    assert(ChassisPowerTransientUpdate(&config, &input, .001f, &state, &output));
}
static void TestFastClampIndependentOfSlowCompensation(void)
{
    Prepare(); input.meter_w = 36.1f; Tick(100, true);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_CLAMP);
    assert(fabsf(output.fast_w + 9) < .001f);
    assert(output.slow_applied_w == 0 && fabsf(output.model_budget_w - 11) < .001f);
    assert(!output.learning_allowed);
    input.slow_w = -2; Tick(101, false);
    assert(fabsf(output.model_budget_w - 9) < .001f);
}
static void TestReleaseNeedsDistinctSamplesAndHold(void)
{
    Prepare(); input.meter_w = 36.1f; Tick(100, true);
    input.meter_w = 18; Tick(101, true);
    Tick(111, false);
    assert(fabsf(output.fast_w + 9) < .001f);
    Tick(112, true);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_RECOVERY);
    assert(fabsf(output.fast_w + 8.94f) < .001f);
    for (uint32_t t = 113; t <= 270; ++t) Tick(t, true);
    assert(output.fast_w == 0 && output.mode == CHASSIS_POWER_TRANSIENT_RUN);
    input.meter_w = 30; Tick(271, true);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_CLAMP && output.fast_w < -5);
}
static void TestStaleMeterReleasesConservativelyAndReacquires(void)
{
    Prepare(); input.meter_w = 36.1f; Tick(100, true);
    Tick(121, false);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_OFFLINE);
    assert(fabsf(output.fast_w + 8.98f) < .001f);
    assert(output.slow_applied_w == 0);
    input.meter_w = 18; Tick(122, true); Tick(123, false);
    assert(!output.learning_allowed);
    Tick(134, true);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_RECOVERY);
}
static void TestDisabledMeterResetInvalidAndWrap(void)
{
    Prepare(); input.meter_enabled = false; input.meter_w = 100;
    Tick(100, true);
    assert(output.fast_w == 0 && output.slow_applied_w == 0 && output.model_budget_w == 20);
    Prepare(); input.now_ms = UINT32_MAX - 5; input.meter_tick_ms = input.now_ms;
    input.meter_w = 36.1f; Tick(input.now_ms, true);
    input.meter_w = 18; Tick(UINT32_MAX - 4, true); Tick(7, true);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_RECOVERY);
    input.control_enabled = false; Tick(8, true);
    assert(output.fast_w == 0 && output.model_budget_w == 0);
    Prepare(); input.raw_current[1] = NAN;
    assert(!ChassisPowerTransientUpdate(&config, &input, .001f, &state, &output));
    assert(output.fault && output.model_budget_w == 0);
}

static void StartLowSpeed(void)
{
    Prepare(); input.startup_enabled = true; input.slow_w = 3;
    for (unsigned w = 0; w < 4; ++w) {
        input.actual_rpm[w] = input.target_rpm[w] = input.raw_current[w] = 0;
    }
    for (uint32_t t = 0; t <= 100; ++t) {
        for (unsigned w = 0; w < 4; ++w) input.feedback_sequence[w] = t + 1;
        Tick(t, true);
    }
    for (unsigned w = 0; w < 4; ++w) { input.target_rpm[w] = 1000; input.raw_current[w] = 5000; }
    Tick(101, true);
    assert(output.mode == CHASSIS_POWER_TRANSIENT_STARTUP && output.startup_triggered);
    assert(output.slow_applied_w == 0 && output.model_budget_w == 20);
    assert(fabsf(output.allowance_j - .18f) < 1e-6f);
}
static void TestStartupWindowEnergyAndNoRecharge(void)
{
    StartLowSpeed(); input.meter_w = 25; Tick(102, true);
    assert(output.fast_w == 0 && fabsf(output.excess_j - .005f) < 1e-6f);
    for (uint32_t t = 103; t < 131; ++t) Tick(t, true);
    assert(output.startup_active);
    Tick(131, true);
    assert(!output.startup_active && output.fast_w < 0);
    input.target_rpm[0] += 300; Tick(132, true);
    assert(!output.startup_triggered);
    StartLowSpeed(); state.excess_j = .179f; input.meter_w = 25; Tick(102, true);
    assert(!output.startup_active && output.fast_w < 0);
    StartLowSpeed(); input.budget_w = 19; Tick(102, true);
    assert(!output.startup_active);
    StartLowSpeed(); input.raw_current[0] = -5000; Tick(102, true);
    assert(!output.startup_active && (output.brake_mask & 1));
}
static void TestAccelerationUsesNewFeedbackAndStaleFallback(void)
{
    Prepare(); input.startup_enabled = false; input.actual_rpm[0] = 100;
    Tick(100, true);
    input.actual_rpm[0] = 110;
    input.feedback_sequence[0] = 2; Tick(101, true);
    assert(fabsf(output.guard_rpm[0] - 190) < .01f);
    input.actual_rpm[0] = 500; Tick(102, false);
    /* Repeated sequence does not differentiate an inconsistent speed snapshot. */
    assert(fabsf(state.acceleration_rpm_s[0] - 10000) < .1f);
    input.feedback_tick_ms[0] = 101;
    input.now_ms = 107; input.meter_tick_ms = 107;
    assert(ChassisPowerTransientUpdate(&config, &input, .001f, &state, &output));
    assert(output.guard_rpm[0] == 500 && !output.feedback_fresh);
}
static void TestGuardPreservesBrakingAndUsesFeasibleIntervals(void)
{
    Prepare(); input.meter_enabled = false;
    config.max_current_command = 100;
    for (unsigned w = 0; w < 4; ++w) { input.raw_current[w] = 1000; input.actual_rpm[w] = 1000; }
    input.raw_current[1] = -1000;
    Tick(100, true);
    MotorPowerModelConfig_s models[4] = {{.current_conversion = 1, .k4 = 1},
        {.current_conversion = 1, .k4 = 1}, {.current_conversion = 1}, {.current_conversion = 1}};
    const float candidate[4] = {10, -2, 0, 0};
    output.allowed_guard_w = 20; state.feedback_seen[0] = 2;
    assert(ChassisPowerTransientGuard(&config, models, &input, candidate, &state, &output));
    assert(output.guarded_current[0] == 4 && output.guarded_current[1] == -2);
    assert(fabsf(output.guard_power_w - 20) < .01f);
    models[0].k0 = 10; models[0].k1 = -5; models[0].k4 = 1;
    models[1].k4 = 0; output.allowed_guard_w = 4;
    assert(ChassisPowerTransientGuard(&config, models, &input, candidate, &state, &output));
    assert(output.guarded_current[0] == 3 && output.guarded_current[1] == -2);
    output.allowed_guard_w = 1;
    assert(!ChassisPowerTransientGuard(&config, models, &input, candidate, &state, &output));
    assert(output.guard_infeasible && output.guarded_current[0] == 0 && output.guarded_current[1] == -2);
}
static void TestFallbackUsesSentCommandsAndDoesNotRampReductions(void)
{
    Prepare(); input.meter_enabled = false; config.max_current_command = 15000;
    MotorPowerModelConfig_s models[4] = {{.current_conversion = 1}, {.current_conversion = 1},
        {.current_conversion = 1}, {.current_conversion = 1}};
    Tick(100, true);
    const float candidate[4] = {15000, 15000, 15000, 15000};
    assert(ChassisPowerTransientGuard(&config, models, &input, candidate, &state, &output));
    assert(output.guarded_current[0] == 3000);
    float sent[4] = {1000, 1000, 1000, 1000};
    ChassisPowerTransientRecordSent(&state, sent);
    assert(ChassisPowerTransientGuard(&config, models, &input, candidate, &state, &output));
    assert(output.guarded_current[0] == 4000);
    float reduction[4] = {500, 500, 500, 500};
    assert(ChassisPowerTransientGuard(&config, models, &input, reduction, &state, &output));
    assert(output.guarded_current[0] == 500);
}

static void TestInvalidTimingConfigurationAndStartupCooldown(void)
{
    Prepare(); config.growth_time_s = 0;
    assert(!ChassisPowerTransientUpdate(&config, &input, .001f, &state, &output));
    assert(output.fault);
    Prepare(); input.slow_w = 5;
    assert(ChassisPowerTransientUpdate(&config, &input, NAN, &state, &output));
    assert(!output.learning_allowed && output.slow_applied_w == 0);
    StartLowSpeed(); input.control_enabled = false; Tick(102, true);
    input.control_enabled = true;
    for (unsigned w = 0; w < 4; ++w) input.target_rpm[w] = input.raw_current[w] = 0;
    for (uint32_t t = 103; t <= 205; ++t) Tick(t, true);
    for (unsigned w = 0; w < 4; ++w) { input.target_rpm[w] = 1000; input.raw_current[w] = 5000; }
    Tick(206, true);
    assert(!output.startup_active && !output.startup_triggered);
}
int main(void)
{
    TestInvalidTimingConfigurationAndStartupCooldown();
    TestStartupWindowEnergyAndNoRecharge();
    TestAccelerationUsesNewFeedbackAndStaleFallback();
    TestGuardPreservesBrakingAndUsesFeasibleIntervals();
    TestFallbackUsesSentCommandsAndDoesNotRampReductions();
    TestFastClampIndependentOfSlowCompensation();
    TestReleaseNeedsDistinctSamplesAndHold();
    TestStaleMeterReleasesConservativelyAndReacquires();
    TestDisabledMeterResetInvalidAndWrap();
    puts("PASS: chassis power transient tests");
    return 0;
}
