#include "chassis_power.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

unsigned BSPLogWrite(const void *data, unsigned length) {(void)data; return length;}
unsigned BSPLogWriteString(const char *text) {return (unsigned)strlen(text);}

static ChassisPower_Init_Config_s Config(void)
{
    ChassisPower_Init_Config_s config = {.stage = 3, .meter_enabled = true};
    for (unsigned w = 0; w < 4; ++w)
        config.models[w] = (MotorPowerModelConfig_s){.k4 = .000001f, .current_conversion = 1};
    config.algorithm = (ChassisPowerAlgorithmConfig_s){.safety_factor = .95f,
        .max_current_command = 15000, .small_error_threshold_rpm = 500,
        .reserved_power_threshold_w = 54, .per_motor_reserved_power_w = 8};
    config.transient = ChassisPowerTransientDefaultConfig();
    config.budget = (ChassisPowerBudgetConfig_s){.empty_energy_ratio = .1f, .full_energy_ratio = .3f,
        .max_boost_w = 100, .rise_rate_w_per_s = 200, .demand_margin_w = 5};
    config.outer = (ChassisPowerOuterLoopConfig_s){.kp = 1, .ki = 1.5f,
        .measurement_lpf_tau_s = .03f, .activation_margin_w = 5, .error_deadband_w = .3f,
        .max_positive_correction_w = 8, .positive_slew_rate_w_per_s = 60,
        .budget_drop_reset_w = 1, .antiwindup_gain_per_s = 10,
        .low_demand_delay_s = .5f, .low_demand_decay_w_per_s = 1};
    return config;
}

static ChassisPowerCycleInput_s Input(uint32_t tick)
{
    ChassisPowerCycleInput_s input = {.now_ms = tick, .dt_s = .002f,
        .online_mask = 15, .enabled_mask = 15,
        .control = {.version = 1, .tick_ms = tick, .control_enabled = true,
            .nominal_budget_w = 1000, .target_power_w = 950, .legacy_budget_w = 1000},
        .meter = {.online = true, .last_rx_tick_ms = tick, .power_w = 0}};
    for (unsigned w = 0; w < 4; ++w) {
        input.feedback_tick_ms[w] = tick;
        input.feedback_sequence[w] = 1;
    }
    return input;
}

static ChassisPowerCycleOutput_s Calculate(ChassisPowerCycleInput_s *input, float demand)
{
    ++input->control.version;
    input->control.tick_ms = input->now_ms;
    input->meter.last_rx_tick_ms = input->now_ms;
    for (unsigned w = 0; w < 4; ++w) {
        input->raw_current[w] = demand;
        input->target_rpm[w] = demand == 0 ? 0 : copysignf(1000, demand);
    }
    ChassisPowerCycleOutput_s output;
    assert(ChassisPowerLimit(input, &output));
    assert(output.valid);
    return output;
}

static void ExpectCurrent(const ChassisPowerCycleOutput_s *output, float expected)
{
    for (unsigned w = 0; w < 4; ++w) {
        if (fabsf(output->current[w] - expected) > 1.0f) {
            fprintf(stderr, "Expected wheel %u current %.0f, got %.0f\n",
                    w, (double)expected, (double)output->current[w]);
            assert(false);
        }
    }
}

static void Record(ChassisPowerCycleInput_s *input, const ChassisPowerCycleOutput_s *output,
                   bool submitted, bool late_stop)
{
    ChassisPowerFinalInput_s final = {.now_ms = input->now_ms,
        .control_version = input->control.version, .can_submitted = submitted,
        .enabled_mask = late_stop ? 0 : input->enabled_mask};
    for (unsigned w = 0; w < 4; ++w)
        final.current_command[w] = late_stop ? 0 : (int16_t)output->current[w];
    assert(ChassisPowerRecordFinal(&final));
    assert(!ChassisPowerRecordFinal(&final));
    ChassisPowerSnapshot_s snapshot;
    assert(ChassisPowerGetSnapshot(&snapshot));
    assert(snapshot.final_valid && snapshot.can_submitted == submitted);
    assert(snapshot.final_tick_ms == input->now_ms);
    for (unsigned w = 0; w < 4; ++w) {
        /* Failed bytes remain diagnostic attempts, never rewritten as applied zeros. */
        assert(snapshot.final_current[w] == final.current_command[w]);
        assert(snapshot.telemetry.current_command[w] == final.current_command[w]);
    }
    input->now_ms += 2;
}

static ChassisPowerCycleInput_s Restart(uint32_t tick)
{
    ChassisPowerCycleInput_s input = Input(tick);
    input.control.control_enabled = false;
    ChassisPowerCycleOutput_s output;
    (void)ChassisPowerLimit(&input, &output);
    ExpectCurrent(&output, 0);
    Record(&input, &output, true, true);
    return Input(tick);
}

static void TestFailuresCannotAccumulateGrowth(void)
{
    ChassisPowerCycleInput_s input = Restart(100);
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        ChassisPowerCycleOutput_s output = Calculate(&input, attempt < 2 ? 12000 : 15000);
        assert(output.transient.growth_fallback);
        /* 15000 / 5 ms * 2 ms = 6000 from the last successful zero command.
         * Aggregate false also represents one failed frame in a multi-frame group. */
        ExpectCurrent(&output, 6000);
        Record(&input, &output, attempt == 3, false);
    }
    ChassisPowerCycleOutput_s output = Calculate(&input, 15000);
    ExpectCurrent(&output, 12000);
    Record(&input, &output, true, false);
    output = Calculate(&input, 15000);
    ExpectCurrent(&output, 15000);
    Record(&input, &output, true, false);
}

static void TestFailedReversalGetsNoHistoryCredit(void)
{
    ChassisPowerCycleInput_s input = Restart(200);
    ChassisPowerCycleOutput_s output = Calculate(&input, 3000);
    ExpectCurrent(&output, 3000);
    Record(&input, &output, true, false);
    for (unsigned attempt = 0; attempt < 2; ++attempt) {
        output = Calculate(&input, -12000);
        ExpectCurrent(&output, -6000);
        Record(&input, &output, attempt == 1, false);
    }
    output = Calculate(&input, -12000);
    ExpectCurrent(&output, -12000);
    Record(&input, &output, true, false);
}

static void TestFailedLateStopPreservesSuccessfulHistory(void)
{
    ChassisPowerCycleInput_s input = Restart(300);
    ChassisPowerCycleOutput_s output = Calculate(&input, 3000);
    ExpectCurrent(&output, 3000);
    Record(&input, &output, true, false);
    output = Calculate(&input, 12000);
    ExpectCurrent(&output, 9000);
    Record(&input, &output, false, true);
    /* Failed zero bytes do not prove the motor stopped. Keep the prior 3000 baseline. */
    output = Calculate(&input, 12000);
    ExpectCurrent(&output, 9000);
    Record(&input, &output, true, false);
    output = Calculate(&input, 0);
    ExpectCurrent(&output, 0);
    Record(&input, &output, true, false);
    output = Calculate(&input, 12000);
    ExpectCurrent(&output, 6000);
    Record(&input, &output, true, false);
}

static void TestFailedHistoryAcrossTimestampWrap(void)
{
    ChassisPowerCycleInput_s input = Restart(UINT32_MAX - 3);
    for (unsigned attempt = 0; attempt < 3; ++attempt) {
        ChassisPowerCycleOutput_s output = Calculate(&input, 12000);
        ExpectCurrent(&output, 6000);
        Record(&input, &output, attempt == 2, false);
    }
    assert(input.now_ms == 2);
    ChassisPowerCycleOutput_s output = Calculate(&input, 12000);
    ExpectCurrent(&output, 12000);
    Record(&input, &output, true, false);
}

int main(void)
{
    const ChassisPower_Init_Config_s config = Config();
    assert(ChassisPowerInit(&config));
    TestFailuresCannotAccumulateGrowth();
    TestFailedReversalGetsNoHistoryCredit();
    TestFailedLateStopPreservesSuccessfulHistory();
    TestFailedHistoryAcrossTimestampWrap();
    puts("PASS: failed submissions cannot advance growth history; diagnostics and recovery preserved");
    return 0;
}
