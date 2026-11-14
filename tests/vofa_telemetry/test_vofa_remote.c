#include "vofa_telemetry.h"
#include "chassis_imu_can.h"
#include "usart.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if VOFA_TELEMETRY_MODE == 2
#include "justfloat_decode.h"
#endif

#ifndef CHASSIS_REMOTE_IMU_FUSION_ENABLE
#define CHASSIS_REMOTE_IMU_FUSION_ENABLE 0
#endif
#ifndef CHASSIS_REMOTE_IMU_CONFIG_VERIFIED
#define CHASSIS_REMOTE_IMU_CONFIG_VERIFIED 0
#endif

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static int64_t fields[72];
static ChassisImuCanSnapshot_s remote;
static bool remote_usable;

bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *out, uint32_t now_ms)
{
    (void)now_ms;
    *out = remote;
    return remote_usable;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t length)
{
    assert(uart == &huart7 && length < 865);
#if VOFA_TELEMETRY_MODE == 2
    ReadJustFloatFields(data, length, fields, 72);
#else
    assert(data[0] == '3' && data[1] == ',');
    char row[865];
    memcpy(row, data, length);
    row[length] = 0;
    char *next = row;
    for (unsigned i = 0; i < 72; ++i) {
        char *end;
        fields[i] = (int32_t)strtol(next, &end, 10);
        assert(end != next && *end == (i == 71 ? '\n' : ','));
        next = end + 1;
    }
    assert(next == row + length);
#endif
    /* A-board provenance survives unavailable/stale samples and either transport. */
    assert(fields[5] & 32768);
    assert(!(fields[5] & 524288)); /* Reserved legacy DM source bit. */
    /* Configuration/estimated-time provenance is independent of sample validity. */
#if CHASSIS_REMOTE_IMU_FUSION_ENABLE
    assert((fields[5] & 196608) == 196608);
#if CHASSIS_REMOTE_IMU_CONFIG_VERIFIED
    assert(fields[5] & 262144);
#else
    assert(!(fields[5] & 262144));
#endif
#else
    assert(!(fields[5] & 458752));
#endif
    return HAL_OK;
}

int main(void)
{
    const USBTelemetrySample_s sample = {0};
    ChassisFusedVelocitySnapshot_s local = {.diagnostics = {
        .available = true, .imu_valid = true, .imu_sequence = 42,
        .imu_quaternion = {1, 0, 0, 0}, .imu_accel_n_mps2 = {9999, 9999, 9999},
    }};
    VofaTelemetrySendMeasurement(&sample, NULL, &local, 0);
    assert(fields[0] == 3 && fields[64] == 2);
    assert(fields[5] & 32768);
    assert(!(fields[5] & 16) && fields[33] == -1 && fields[65] == -1);
    assert(fields[34] == 0 && fields[35] == 0 && fields[36] == 0 && fields[40] == 0);
    assert(fields[66] == -1 && fields[69] == -1);
    assert(!(fields[5] & 8));

    remote = (ChassisImuCanSnapshot_s){
        .available = true, .first_rx_tick_ms = 77, .last_rx_tick_ms = 79,
        .rejected_groups = 0x10002U,
        .sample = {.quaternion = {.5f, -.5f, .5f, -.5f},
            .accel_n_mps2 = {.125f, -.25f, .5f}, .gyro_sensor_radps = {1, -2, 3},
            .sequence = 0xabcd1234U, .tick_ms = 0x12345678U, .source_age_ms = 2, .valid = true},
    };
    remote_usable = true;
    local.diagnostics.tick_ms = local.diagnostics.imu_tick_ms = 80;
    local.diagnostics.wheel_online_mask = 15;
    for (unsigned i = 0; i < 4; ++i) local.diagnostics.wheel_tick_ms[i] = 80;
    local.wheel = (ChassisVelocity_s){.valid = true, .tick_ms = 80,
        .vx_mps = .25f, .vy_mps = -.5f};
    local.valid = true;
    local.tick_ms = 60;
    local.status = CHASSIS_VELOCITY_FUSED;
    local.vx_mps = .5f;
    local.vy_mps = -.75f;
    local.paired_wheel_mps[0] = .125f;
    local.paired_wheel_mps[1] = -.25f;
    local.center_accel_n_mps2[0] = .5f;
    local.center_accel_n_mps2[1] = -.75f;
    VofaTelemetrySendMeasurement(&sample, NULL, &local, 80);
    assert(fields[5] & 16);
    assert(fields[33] == 5 && fields[34] == 43981 && fields[35] == 4660);
    assert(fields[36] == 500000 && fields[37] == -500000);
    assert(fields[40] == 125 && fields[41] == -250 && fields[42] == 500);
    assert(fields[43] == 1000 && fields[44] == -2000 && fields[45] == 3000);
    assert(fields[63] == 0 && fields[64] == 2 && fields[65] == 1 && fields[66] == 2);
    assert(fields[67] == 4660 && fields[68] == 22136 && fields[69] == 2);
    assert(fields[70] == 1 && fields[71] == 2);
    assert((fields[5] & 12) == 12 && fields[6] == 9 && fields[11] == 20);
    assert(fields[27] == 500 && fields[28] == -750);
    assert(fields[29] == 125 && fields[30] == -250);
    assert(fields[31] == 500 && fields[32] == -750);

    /* A fresh local sensor must not hide a stale or invalid remote source. */
    remote_usable = false;
    local.diagnostics.tick_ms = local.diagnostics.imu_tick_ms = 160;
    for (unsigned i = 0; i < 4; ++i) local.diagnostics.wheel_tick_ms[i] = 160;
    local.wheel.tick_ms = 160;
    local.tick_ms = 99; /* Fresh wheels cannot make a 61 ms fused estimate fresh. */
    VofaTelemetrySendMeasurement(&sample, NULL, &local, 160);
    assert(!(fields[5] & 16) && fields[33] == 85 && fields[65] == 81);
    assert(!(fields[5] & 12) && fields[11] == 61);
    for (unsigned i = 27; i <= 32; ++i) assert(fields[i] == 0);
    for (unsigned i = 36; i <= 45; ++i) assert(fields[i] == 0);
    remote.sample.valid = false;
    remote.first_rx_tick_ms = remote.last_rx_tick_ms = 240;
    remote.sample.source_age_ms = UINT16_MAX;
    local.diagnostics.tick_ms = local.diagnostics.imu_tick_ms = 240;
    VofaTelemetrySendMeasurement(&sample, NULL, &local, 240);
    assert(!(fields[5] & 16) && fields[33] == -1 && fields[65] == 0 && fields[66] == -1);
    assert(fields[40] == 0 && fields[67] == 4660 && fields[68] == 22136);
    puts("PASS: observation identifies remote CAN IMU without silently using the local IMU");
    return 0;
}
