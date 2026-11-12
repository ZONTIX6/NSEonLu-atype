#include "vofa_telemetry.h"
#include "usart.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if VOFA_TELEMETRY_MODE == 2
#include "justfloat_decode.h"
#endif

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static const uint8_t *active_data;
static uint16_t active_length;
static unsigned frames_started;
static bool fail_send;

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t length)
{
    assert(uart == &huart7 && uart->gState == HAL_UART_STATE_READY);
    if (fail_send) return HAL_ERROR;
    active_data = data;
    active_length = length;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    ++frames_started;
    return HAL_OK;
}

static void ReadRow(int64_t fields[64])
{
#if VOFA_TELEMETRY_MODE == 2
    ReadJustFloatFields(active_data, active_length, fields, 64);
#else
    assert(active_data && active_length > 0 && active_length <= 768);
    assert(memchr(active_data, '\n', active_length) == active_data + active_length - 1);
    assert(memchr(active_data, '\r', active_length) == NULL);
    const char *next = (const char *)active_data;
    for (unsigned i = 0; i < 64; ++i) {
        char *end;
        const long value = strtol(next, &end, 10);
        assert(end != next && value >= INT32_MIN && value <= INT32_MAX);
        assert(*end == (i == 63 ? '\n' : ','));
        fields[i] = (int32_t)value;
        next = end + 1;
    }
    assert(next == (const char *)active_data + active_length);
#endif
}

static void Freshen(USBTelemetrySample_s *s, ChassisPowerSnapshot_s *p,
                    ChassisFusedVelocitySnapshot_s *v, uint32_t now)
{
    s->chassis_tick_ms = s->velocity_target_tick_ms = now;
    p->tick_ms = p->meter_tick_ms = now;
    v->tick_ms = now - 25U;
    v->wheel.tick_ms = v->diagnostics.tick_ms = v->diagnostics.imu_tick_ms = now;
    for (unsigned i = 0; i < 4; ++i) v->diagnostics.wheel_tick_ms[i] = now;
    for (unsigned i = 0; i < 2; ++i) v->diagnostics.joint_tick_ms[i] = now;
}

int main(void)
{
    USBTelemetrySample_s sample = {
        .motor_online_mask = 15, .motor_enabled_mask = 13, .rpm_lf = 9999,
        .chassis_valid = 1, .chassis_mode = 3, .chassis_tick_ms = UINT32_MAX - 2,
        .velocity_target_valid = 1, .velocity_target_tick_ms = UINT32_MAX - 3,
        .velocity_target_vx_mmps = 3000, .velocity_target_vy_mmps = -4000,
        .velocity_target_wz_mrps = -500, .meter_online = 1, .p_meter_cw = 9999,
        .power_valid = 1, .p_unlimited_cw = 9100, .p_limited_cw = 3850,
        .effective_limit_cw = 5250, .power_source = 1,
    };
    ChassisPowerSnapshot_s cycle = {
        .registered = true, .cycle_valid = true, .final_valid = true, .can_submitted = true,
        .tick_ms = UINT32_MAX - 5, .final_current = {121, -122, 123, -124},
        .measured_power_w = 73.5f, .meter_online = true, .meter_tick_ms = UINT32_MAX - 9,
    };
    ChassisFusedVelocitySnapshot_s velocity = {
        .wheel = {.valid = true, .tick_ms = UINT32_MAX - 5,
                  .vx_mps = 1.25f, .vy_mps = -2.5f, .wz_rpm = 30},
        .valid = true, .status = CHASSIS_VELOCITY_FUSED, .tick_ms = UINT32_MAX - 24,
        .vx_mps = 1, .vy_mps = -2, .paired_wheel_mps = {1.125f, -2.125f},
        .center_accel_n_mps2 = {.125f, -.25f},
        .diagnostics = {
            .available = true, .tick_ms = UINT32_MAX - 1, .wheel_online_mask = 15,
            .raw_rpm = {101, -202, 303, -404},
            .wheel_tick_ms = {UINT32_MAX - 5, UINT32_MAX - 6, UINT32_MAX - 7, UINT32_MAX - 8},
            .imu_valid = true, .imu_tick_ms = UINT32_MAX - 2, .imu_sequence = 0xabcd1234U,
            .imu_quaternion = {.5f, -.5f, .5f, -.5f},
            .imu_accel_n_mps2 = {.125f, -1.25f, 2.5f},
            .imu_gyro_sensor_radps = {.5f, -1.5f, 2.25f},
            .joints_valid = true, .joint_ecd = {1000, 2000}, .joint_rpm = {-6, 7},
            .joint_tick_ms = {UINT32_MAX - 3, UINT32_MAX - 4},
        },
    };
    int64_t fields[64];
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, 0);
    assert(frames_started == 1);
    ReadRow(fields);
    const int32_t expected[64] = {
        2, 0, 0, 0, 0, 16383, 9, 3, 15, 13, 2, 25, 6,
        101, -202, 303, -404, 6, 7, 8, 9, 1250, -2500, 500,
        3000, -4000, -500, 1000, -2000, 1125, -2125, 125, -250,
        3, 43981, 4660, 500000, -500000, 500000, -500000,
        125, -1250, 2500, 500, -1500, 2250, 1000, 2000, -6, 7, 4, 5,
        121, -122, 123, -124, 9100, 3850, 5250, 7350, 6, 4, 10, 0,
    };
    for (unsigned i = 0; i < 64; ++i) {
        if (fields[i] != expected[i]) fprintf(stderr, "I%u: %ld != %ld\n", i, (long)fields[i], (long)expected[i]);
        assert(fields[i] == expected[i]);
    }
    char saved[769];
    memcpy(saved, active_data, active_length);
    const uint16_t saved_length = active_length;
    /* Busy transport preserves in-flight bytes and exposes the skipped sequence. */
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, 80);
    assert(frames_started == 1 && active_length == saved_length);
    assert(memcmp(saved, active_data, saved_length) == 0);
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity,
                                  VOFA_TELEMETRY_MODE == 2 ? 99 : 159);
    assert(frames_started == 1);
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, 160);
    ReadRow(fields);
    assert(fields[4] == 2 && fields[5] == 8192 && fields[6] == 9);
    assert(fields[8] == 0 && fields[10] == 162 && fields[33] == 163);
    assert(fields[13] == 0 && fields[27] == 0 && fields[36] == 0 && fields[59] == 0);
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, 240);
    ReadRow(fields);
    assert(fields[6] == -1 && fields[10] == -1 && fields[11] == -1 && fields[33] == -1);
    assert(fields[13] == 0 && fields[27] == 0 && fields[34] == 0 && fields[50] == -1);
    /* Uncalibrated / rejected estimates are wheel fallbacks, never paired fusion samples. */
    for (unsigned i = 0; i < 2; ++i) {
        const uint32_t now = 320 + i * 80;
        Freshen(&sample, &cycle, &velocity, now);
        velocity.status = i ? CHASSIS_VELOCITY_REJECTED : CHASSIS_VELOCITY_UNCALIBRATED;
        velocity.tick_ms = now;
        huart7.gState = HAL_UART_STATE_READY;
        VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, now);
        ReadRow(fields);
        assert((fields[5] & 4) && !(fields[5] & 8));
        assert(fields[6] == (i ? 8 : 2) && fields[27] == 1000);
        assert(fields[29] == 0 && fields[30] == 0 && fields[31] == 0 && fields[32] == 0);
    }
    /* Stale wheels, invalid IMU numbers and stale joints invalidate only their groups. */
    Freshen(&sample, &cycle, &velocity, 480);
    velocity.status = CHASSIS_VELOCITY_FUSED;
    velocity.diagnostics.wheel_tick_ms[0] = 459;
    velocity.diagnostics.imu_accel_n_mps2[1] = NAN;
    velocity.diagnostics.joint_tick_ms[1] = 459;
    cycle.measured_power_w = NAN;
    cycle.can_submitted = false;
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, 480);
    ReadRow(fields);
    assert(fields[8] == 14 && fields[13] == 0 && fields[14] == -202);
    assert((fields[5] & (2 | 4 | 8 | 16 | 32 | 1024 | 4096)) == 0);
    assert(fields[27] == 0 && fields[40] == 0 && fields[46] == 0 && fields[59] == 0);
    assert(fields[52] == 121 && fields[56] == 9100 && fields[51] == 21);
    /* Extreme finite fixed-point values saturate without overflowing the row. */
    Freshen(&sample, &cycle, &velocity, 560);
    velocity.diagnostics.imu_accel_n_mps2[1] = -1e30f;
    velocity.diagnostics.imu_accel_n_mps2[0] = 1e30f;
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, 560);
    ReadRow(fields);
    assert(fields[40] == (VOFA_TELEMETRY_MODE == 2 ? (int64_t)(float)INT32_MAX : INT32_MAX));
    assert(fields[41] == INT32_MIN);
    /* Failed starts also consume a sequence, exposing a lost line. */
    huart7.gState = HAL_UART_STATE_READY;
    fail_send = true;
    const unsigned before_failure = frames_started;
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, 640);
    assert(frames_started == before_failure);
    fail_send = false;
    VofaTelemetrySendMeasurement(&sample, &cycle, &velocity, 720);
    ReadRow(fields);
    assert(fields[4] == 9);
    puts("PASS: full observation fields, validity, rollover and asynchronous transport");
    return 0;
}
