#include "vofa_telemetry.h"
#include "usart.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { CHANNEL_COUNT = 19, FRAME_SIZE = 80 };

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static const uint8_t *active_data;
static unsigned frames_started;

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
                                       const uint8_t *data,
                                       uint16_t length)
{
    assert(uart == &huart7);
    assert(data != NULL && length == FRAME_SIZE);
    if (uart->gState == HAL_UART_STATE_BUSY_TX)
        return HAL_BUSY;
    active_data = data;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    ++frames_started;
    return HAL_OK;
}

static const USBTelemetrySample_s full_sample = {
    .motor_online_mask = 15,
    .rpm_lf = 101, .rpm_rf = -202, .rpm_lb = 303, .rpm_rb = -404,
    .target_lf_rpm = 505, .target_rf_rpm = -606, .target_lb_rpm = 707, .target_rb_rpm = -808,
    .meter_online = 1, .p_meter_cw = 7350,
    .power_valid = 1, .p_limited_cw = 3850, .effective_limit_cw = 5250,
    .cap_online = 1, .cap_chassis_cw = 6125, .cap_energy_pm = 875,
    .cap_input_online = 1, .cap_input_cw = 2225,
    .imu_valid = 1, .imu_yaw_mdeg = -75500, .imu_pitch_mdeg = 11250, .imu_roll_mdeg = -8750
};

static void AssertFrame(const float expected[CHANNEL_COUNT])
{
    static const uint8_t frame_tail[] = {0x00, 0x00, 0x80, 0x7f};
    assert(active_data != NULL);
    assert(memcmp(active_data + 76, frame_tail, sizeof frame_tail) == 0);
    for (unsigned channel = 0; channel < CHANNEL_COUNT; ++channel) {
        const uint8_t *bytes = active_data + channel * 4U;
        const uint32_t bits = (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
                              (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
        float value;
        memcpy(&value, &bits, sizeof value);
        const float difference = value - expected[channel];
        if (!(difference > -0.0001f && difference < 0.0001f)) {
            fprintf(stderr, "channel %u: expected %g, received %g\n", channel,
                    (double)expected[channel], (double)value);
            assert(0);
        }
    }
}

static void TestFrameLayoutAndScales(void)
{
    const float expected[CHANNEL_COUNT] = {
        101, -202, 303, -404, 505, -606, 707, -808,
        73.5f, 38.5f, 52.5f, 61.25f, 22.25f, 87.5f, -75.5f, 11.25f, -8.75f, 15, 31
    };

    const unsigned previous = frames_started;
    VofaTelemetrySend(&full_sample);
    assert(frames_started == previous + 1);
    AssertFrame(expected);
    huart7.gState = HAL_UART_STATE_READY;
}

static void TestValidityGatesEachSource(void)
{
    static const float expected[][CHANNEL_COUNT] = {
        {101, 0, 303, 0, 505, 0, 707, 0, [9] = 38.5f, [10] = 52.5f, [17] = 5, [18] = 1},
        {101, 0, 303, 0, 505, 0, 707, 0, [8] = 73.5f, [17] = 5, [18] = 2},
        {101, 0, 303, 0, 505, 0, 707, 0, [11] = 61.25f, [13] = 87.5f, [17] = 5, [18] = 4},
        {101, 0, 303, 0, 505, 0, 707, 0, [12] = 22.25f, [17] = 5, [18] = 8},
        {101, 0, 303, 0, 505, 0, 707, 0, [14] = -75.5f, [15] = 11.25f, [16] = -8.75f, [17] = 5, [18] = 16}
    };

    for (unsigned source = 0; source < sizeof expected / sizeof expected[0]; ++source) {
        USBTelemetrySample_s sample = full_sample;
        sample.motor_online_mask = 5;
        sample.power_valid = source == 0;
        sample.meter_online = source == 1;
        sample.cap_online = source == 2;
        sample.cap_input_online = source == 3;
        sample.imu_valid = source == 4;

        const unsigned previous = frames_started;
        VofaTelemetrySend(&sample);
        assert(frames_started == previous + 1);
        AssertFrame(expected[source]);
        huart7.gState = HAL_UART_STATE_READY;
    }
}

static void TestBusyPreservesBytesUntilTransmitCompletes(void)
{
    USBTelemetrySample_s latest = full_sample;
    uint8_t original[FRAME_SIZE];
    const unsigned previous = frames_started;

    VofaTelemetrySend(&full_sample);
    assert(frames_started == previous + 1);
    memcpy(original, active_data, sizeof original);

    latest.rpm_lf = 0;
    VofaTelemetrySend(&latest);
    assert(frames_started == previous + 1);
    assert(memcmp(active_data, original, sizeof original) == 0);

    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySend(&latest);
    assert(frames_started == previous + 2);
    static const uint8_t zero_float[] = {0, 0, 0, 0};
    assert(memcmp(active_data, zero_float, sizeof zero_float) == 0);
    assert(memcmp(active_data + 4, original + 4, FRAME_SIZE - 4) == 0);
}

int main(void)
{
    TestFrameLayoutAndScales();
    TestValidityGatesEachSource();
    TestBusyPreservesBytesUntilTransmitCompletes();
    puts("PASS: VOFA+ JustFloat frame, validity and asynchronous buffer ownership");
    return 0;
}
