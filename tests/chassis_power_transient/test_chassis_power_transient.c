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
int main(void)
{
    TestFastClampIndependentOfSlowCompensation();
    TestReleaseNeedsDistinctSamplesAndHold();
    TestStaleMeterReleasesConservativelyAndReacquires();
    TestDisabledMeterResetInvalidAndWrap();
    puts("PASS: chassis power transient tests");
    return 0;
}
