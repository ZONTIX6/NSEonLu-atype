#include "chassis_power.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned BSPLogWrite(const void *data, unsigned length) {(void)data; return length;}
unsigned BSPLogWriteString(const char *text) {return (unsigned)strlen(text);}

int main(int argc, char **argv)
{
    assert(argc == 2);
    ChassisPower_Init_Config_s config = {.stage = (uint8_t)strtoul(argv[1], NULL, 10),
        .meter_enabled = true, .buffer_enabled = true};
    for (unsigned w = 0; w < 4; ++w)
        config.models[w] = (MotorPowerModelConfig_s){.k1 = .01f, .current_conversion = 1};
    config.algorithm = (ChassisPowerAlgorithmConfig_s){.safety_factor = .98f,
        .max_current_command = 15000, .small_error_threshold_rpm = 500,
        .reserved_power_threshold_w = 54, .per_motor_reserved_power_w = 8};
    config.transient = ChassisPowerTransientDefaultConfig();
    config.budget = (ChassisPowerBudgetConfig_s){.empty_energy_ratio = .1f, .full_energy_ratio = .3f,
        .max_boost_w = 100, .rise_rate_w_per_s = 200, .demand_margin_w = 5,
        .buffer = {.max_energy_j = 60, .recovery_enter_j = 10, .recovery_exit_j = 20,
            .hard_floor_j = 5, .prediction_window_s = .25f, .guard_window_s = .3f,
            .settlement_delay_s = .1f, .energy_margin_j = 3, .power_margin_w = 5,
            .max_boost_w = 60, .recovery_power_w = 20, .recovery_target_time_s = 1,
            .observer_max_step_s = .02f, .recovery_hold_s = .1f,
            .rise_rate_w_per_s = 1200, .safety_factor = .98f, .correction_rise_j_per_s = 20}};
    config.outer = (ChassisPowerOuterLoopConfig_s){.kp = 1, .ki = 1.5f,
        .measurement_lpf_tau_s = .03f, .activation_margin_w = 5, .error_deadband_w = .3f,
        .max_positive_correction_w = 8, .positive_slew_rate_w_per_s = 60,
        .budget_drop_reset_w = 1, .antiwindup_gain_per_s = 10,
        .low_demand_delay_s = .5f, .low_demand_decay_w_per_s = 1};
    assert(ChassisPowerInit(&config));

    for (unsigned limited = 0; limited < 2; ++limited)
    {
        for (unsigned n = 0; n < 100; ++n)
        {
            const uint32_t tick = 1000 + limited * 200 + n * 2;
            ChassisPowerCycleInput_s input = {.now_ms = tick, .dt_s = .002f,
                .online_mask = 15, .enabled_mask = 15,
                .control = {.version = limited * 100 + n + 1, .tick_ms = tick,
                    .control_enabled = true, .nominal_budget_w = 50, .target_power_w = 49,
                    .legacy_budget_w = 50, .power_ceiling_w = 50, .power_deadband_w = .3f},
                .meter = {.online = true, .last_rx_tick_ms = tick, .power_w = 36}};
            const float demand[4] = {1000, 600, limited ? 4000 : 1500, 1100};
            const float expected[4] = {1000, 600, limited ? 2200 : 1500, 1100};
            for (unsigned w = 0; w < 4; ++w)
            {
                input.raw_current[w] = demand[w];
                input.actual_rpm[w] = 1000;
                input.target_rpm[w] = 1100;
                input.feedback_tick_ms[w] = tick;
                input.feedback_sequence[w] = limited * 100 + n + 1;
            }
            ChassisPowerCycleOutput_s output;
            assert(ChassisPowerLimit(&input, &output));
            ChassisPowerFinalInput_s final = {.now_ms = tick,
                .control_version = input.control.version, .enabled_mask = 15, .can_submitted = true};
            for (unsigned w = 0; w < 4; ++w)
            {
                assert(fabsf(output.current[w] - expected[w]) < 1.1f);
                final.current_command[w] = (int16_t)output.current[w];
            }
            assert(ChassisPowerRecordFinal(&final));
            ChassisPowerSnapshot_s snapshot;
            assert(ChassisPowerGetSnapshot(&snapshot));
            assert(snapshot.base_limited == (limited != 0));
            assert(snapshot.transient.drive_mask != 0 && snapshot.transient.brake_mask == 0);
            assert(!snapshot.transient.fault && !snapshot.numeric_error);
            assert(snapshot.telemetry.predicted_total_power_w <= 49.001f);
        }
    }
    printf("PASS: stage %u idle allocation and genuine total-power limiting\n", config.stage);
    return 0;
}
