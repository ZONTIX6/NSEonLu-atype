#include "vofa_telemetry.h"
#include "chassis_power_types.h"
#include "usart.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static const uint8_t *active_data;
static uint16_t active_length;
static unsigned frames_started;

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
                                     const uint8_t *data, uint16_t length)
{
    assert(uart == &huart7 && uart->gState == HAL_UART_STATE_READY);
    active_data = data;
    active_length = length;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    ++frames_started;
    return HAL_OK;
}

static void AssertRow(const char *expected)
{
    assert(active_data != NULL && active_length == strlen(expected));
    assert(memchr(active_data, '\n', active_length) == active_data + active_length - 1);
    assert(memchr(active_data, '\r', active_length) == NULL);
    assert(memcmp(active_data, expected, active_length) == 0);
}

int main(void)
{
    const USBTelemetrySample_s sample = {
        .motor_online_mask = 15, .motor_enabled_mask = 13,
        .rpm_lf = 101, .rpm_rf = -202, .rpm_lb = 303, .rpm_rb = -404,
        .target_lf_rpm = 505, .target_rf_rpm = -606, .target_lb_rpm = 707, .target_rb_rpm = -808,
        .i_lf_cmd = 999, .i_lf_fb = 101, .i_rf_fb = -102, .i_lb_fb = 103, .i_rb_fb = -104,
        .meter_online = 1, .p_meter_cw = 9999,
        .power_valid = 1, .p_unlimited_cw = 9100, .p_limited_cw = 3850, .effective_limit_cw = 5250,
        .cap_online = 1, .cap_chassis_cw = 6125, .cap_energy_pm = 875,
        .cap_input_online = 1, .cap_input_cw = 2225,
        .imu_valid = 1, .imu_yaw_mdeg = -75500, .imu_pitch_mdeg = 11250, .imu_roll_mdeg = -8750,
        .referee_online = 1, .buffer_j = 47
    };
    ChassisPowerSnapshot_s cycle = {
        .registered = true, .cycle_valid = true, .final_valid = true, .can_submitted = true,
        .tick_ms = UINT32_MAX - 5, .final_current = {121, -122, 123, -124},
        .feedback_tick_ms = {UINT32_MAX - 5, UINT32_MAX - 6, UINT32_MAX - 7, UINT32_MAX - 8},
        .measured_power_w = 73.5f, .meter_online = true, .meter_tick_ms = UINT32_MAX - 9
    };
    cycle.meter_sequence = 0xabcd1234U;

    /* The control-cycle reading and final CAN command win over unrelated sample values. */
    VofaTelemetrySendMeasurement(&sample, &cycle, 0);
    assert(frames_started == 1);
    const char *first_row = "101,-202,303,-404,505,-606,707,-808,7350,3850,5250,6125,2225,875,"
                            "-75500,11250,-8750,15,255,121,-122,123,-124,101,-102,103,-104,"
                            "9100,47,4,65535,65530,0,0,13,43981,4660,1\n";
    AssertRow(first_row);

    /* A busy UART must retain the active line, and the next sequence exposes the gap. */
    VofaTelemetrySendMeasurement(&sample, NULL, 60);
    assert(frames_started == 1);
    AssertRow(first_row);
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, &cycle, 119);
    assert(frames_started == 1);

    /* A stale cycle cannot make a fresh independent USB meter reading look aligned. */
    VofaTelemetrySendMeasurement(&sample, &cycle, 120);
    assert(frames_started == 2);
    AssertRow("0,0,0,0,0,0,0,0,0,0,0,6125,2225,875,-75500,11250,-8750,0,60,"
              "0,0,0,0,0,0,0,0,0,47,-1,0,0,0,2,0,0,0,0\n");

    /* Keep failed CAN attempts visible, but reject stale feedback and a non-finite meter reading. */
    huart7.gState = HAL_UART_STATE_READY;
    cycle.tick_ms = 179;
    cycle.meter_tick_ms = 178;
    cycle.feedback_tick_ms[0] = 159;
    cycle.feedback_tick_ms[1] = 160;
    cycle.feedback_tick_ms[2] = 177;
    cycle.feedback_tick_ms[3] = 178;
    cycle.measured_power_w = NAN;
    cycle.can_submitted = false;
    VofaTelemetrySendMeasurement(&sample, &cycle, 180);
    assert(frames_started == 3);
    AssertRow("0,-202,303,-404,0,-606,707,-808,0,3850,5250,6125,2225,875,"
              "-75500,11250,-8750,14,253,121,-122,123,-124,0,-102,103,-104,"
              "9100,47,1,0,179,0,3,13,43981,4660,0\n");
    puts("PASS: VOFA+ FireWater measurement rows, freshness and UART cadence");
    return 0;
}
