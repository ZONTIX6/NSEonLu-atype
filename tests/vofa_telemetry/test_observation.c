#include "vofa_observation.h"
#include "vofa_schema.h"
#include "usart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "imu_capture_config.h"
#if CHASSIS_IMU_CAN_RX_ENABLE
#include "chassis_imu_can.h"
static ChassisImuCanSnapshot_s source_imu;
bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *out, uint32_t now)
{
    *out = source_imu;
    return out->available && (uint32_t)(now - out->first_rx_tick_ms) <= 60U;
}
#endif

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static const uint8_t *active;
static uint16_t length;
static unsigned sent;
static bool fail_send;
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t size)
{
    assert(uart == &huart7);
    if (fail_send) return HAL_ERROR;
    active = data; length = size; ++sent;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    return HAL_OK;
}
static float field(unsigned index)
{
    float value;
    memcpy(&value, active + index * 4, 4);
    return value;
}
static void check_frame(void)
{
    const uint8_t tail[] = {0, 0, 128, 127};
    assert(length == 388);
    assert(memcmp(active + 384, tail, 4) == 0);
    for (unsigned i = 0; i < 96; ++i) assert(isfinite(field(i)));
    for (unsigned i = 0; i + 4 <= 384; ++i) assert(memcmp(active + i, tail, 4) != 0);
}
int main(void)
{
    USBTelemetrySample_s sample = {.velocity_target_valid = true, .chassis_valid = true,
        .velocity_target_vx_mmps = 1234, .velocity_target_wz_mrps = 250};
    ChassisPowerSnapshot_s power = {0};
    SuperCapSnapshot_s cap = {0};
    RefereePowerSnapshot_s referee = {0};
    ChassisFusedVelocitySnapshot_s velocity = {0};
    const uint32_t start = UINT32_MAX - 19U;
    sample.chassis_tick_ms = sample.velocity_target_tick_ms = start;
#if CHASSIS_IMU_CAN_RX_ENABLE
    source_imu.available = true;
    source_imu.first_rx_tick_ms = start - 5U;
    source_imu.last_rx_tick_ms = start - 3U;
    source_imu.sample.sequence = 27;
    source_imu.sample.quaternion[0] = 1.0f;
    source_imu.sample.accel_n_mps2[0] = 2.5f;
#endif
    velocity.remote.attempt.available = true;
    velocity.remote.attempt.sequence = 1022;
    velocity.remote.attempt.input = (ChassisVelocityFusionInput_s){
        .now_us = 200000, .wheel_us = 180000, .imu_us = 180000,
        .wheel_n_mps = {1.25f, -2.5f}, .accel_n_mps2 = {0.125f, -0.5f},
        .wheel_valid = true, .imu_valid = true};
    velocity.remote.attempt.config.filter.wheel_variance = 0.04f;
    velocity.remote.attempt.receiver_call_ms = start;
    power.registered = power.final_valid = power.cycle_valid = power.can_submitted = true;
    power.tick_ms = power.final_tick_ms = start;
    power.online_mask = 15;
    power.cycle_control.tick_ms = start;
    power.cycle_config.available = true;
    power.cycle_control.nominal_budget_w = 68.25f;
    power.cycle_control.target_power_w = 70.0f;
    power.final_current[0] = -1234;
    power.raw_current[0] = -2345.5f;
    power.telemetry.actual_speed_rpm[0] = 456.5f;
    power.meter_online = true;
    power.meter_tick_ms = start;
    power.meter_sequence = 65537;
    power.measured_power_w = 12.75f;
    for (unsigned i = 0; i < 4; ++i) power.feedback_tick_ms[i] = start;
    power.policy.control.nominal_budget_w = 99.0f;
    VofaObservationSend(&sample, &power, &velocity, &cap, &referee, start);
    assert(sent == 1);
    check_frame();
#if VOFA_OBSERVATION_PROFILE == 1
    assert(field(0) == 4);
    assert(field(VOFA_POWER_FINAL_CURRENT_LF) == -1234);
    assert(field(VOFA_POWER_RAW_CURRENT_LF) == -2345.5f);
    assert(field(VOFA_POWER_NOMINAL_BUDGET_W) == 68.25f);
    assert(field(VOFA_POWER_METER_POWER_W) == 12.75f);
    assert(field(VOFA_POWER_METER_SEQUENCE_HI) == 1);
#else
    assert(field(0) == 5);
    assert(field(VOFA_KALMAN_WHEEL_N_X_MPS) == 1.25f);
    assert(field(VOFA_KALMAN_ACCEL_N_Y_MPS2) == -0.5f);
    assert(fabsf(field(VOFA_KALMAN_TARGET_VX_MPS) - 1.234f) < 1e-6f);
    assert(fabsf(field(VOFA_KALMAN_TARGET_WZ_RADPS) - 1.5707963f) < 1e-6f);
    assert(field(VOFA_KALMAN_ATTEMPT_FLAGS) == 7);
#if CHASSIS_IMU_CAN_RX_ENABLE
    assert(field(VOFA_KALMAN_IMU_SOURCE) == 2);
    assert(field(VOFA_KALMAN_IMU_AX_MPS2) == 2.5f);
    assert(field(VOFA_KALMAN_IMU_AGE_MS) == 5);
    assert(field(VOFA_KALMAN_IMU_RECEIVE_AGE_MS) == 3);
#endif
#endif
    uint8_t saved[388]; memcpy(saved, active, sizeof saved);
#if CHASSIS_IMU_CAN_RX_ENABLE
    source_imu.available = false;
#endif
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, start + 20U);
    assert(sent == 1 && memcmp(saved, active, sizeof saved) == 0);
    huart7.gState = HAL_UART_STATE_READY;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, start + 39U);
    assert(sent == 1);
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, start + 40U);
    assert(sent == 2 && field(4) == 2 && field(5) == 0);
    check_frame();
    huart7.gState = HAL_UART_STATE_READY; fail_send = true;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, start + 60U);
    assert(sent == 2);
    fail_send = false;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, start + 80U);
    assert(sent == 3 && field(4) == 4);
    huart7.gState = HAL_UART_STATE_READY;
    power.measured_power_w = NAN;
    velocity.remote.attempt.input.wheel_n_mps[0] = NAN;
    VofaObservationSend(&sample, &power, &velocity, &cap, &referee, start + 100U);
    check_frame();
#if VOFA_OBSERVATION_PROFILE == 1
    assert(((unsigned)field(5) & (1U << 3)) == 0);
#else
    assert(((unsigned)field(5) & (1U << 4)) == 0);
#endif
    /* Exercise low-half sequence rollover, which includes false-tail patterns
     * across float boundaries (not just positive-infinity scalar values). */
    for (uint32_t sequence = 6; sequence <= 65536U; ++sequence) {
        huart7.gState = HAL_UART_STATE_READY;
        VofaObservationSend(NULL, NULL, NULL, NULL, NULL, start + sequence * 20U);
        check_frame();
        assert((uint32_t)lroundf(field(3)) * 65536U + (uint32_t)lroundf(field(4)) == sequence);
    }
    const uint32_t jitter = start + 65537U * 20U;
    const uint32_t offsets[] = {0, 21, 40, 61, 80, 101};
    const unsigned before = sent;
    for (unsigned i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        huart7.gState = HAL_UART_STATE_READY;
        VofaObservationSend(NULL, NULL, NULL, NULL, NULL, jitter + offsets[i]);
        assert(sent == before + i + 1);
    }
    huart7.gState = HAL_UART_STATE_READY;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, jitter + 181U);
    assert(sent == before + 7);
    huart7.gState = HAL_UART_STATE_READY;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, jitter + 182U);
    assert(sent == before + 7);
    puts("PASS: dedicated profile units, evidence, finite data and asynchronous transport");
    return 0;
}
