#include "vofa_observation.h"
#include "vofa_schema.h"
#include "usart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "imu_capture_config.h"
enum { CHANNELS = VOFA_OBSERVATION_PROFILE == 1 ? VOFA_POWER_CHANNELS : VOFA_KALMAN_GIMBAL_CHANNELS,
       FRAME_BYTES = (CHANNELS + 1) * 4 };
static uint32_t tick;
static unsigned critical;
#if CHASSIS_IMU_CAN_RX_ENABLE
#include "chassis_imu_can.h"
static ChassisImuCanSnapshot_s source_imu;
static bool interrupt_on_tick, interrupt_pending;
static void receive_interrupt(void)
{
    interrupt_pending = false;
    tick += 2U;
    source_imu.first_rx_tick_ms = source_imu.last_rx_tick_ms = tick;
    ++source_imu.sample.sequence;
}
#endif
uint32_t HAL_GetTick(void)
{
    const uint32_t sampled = tick;
#if CHASSIS_IMU_CAN_RX_ENABLE
    if (interrupt_on_tick) {
        interrupt_on_tick = false;
        interrupt_pending = true;
        if (!critical) receive_interrupt();
    }
#endif
    return sampled;
}
void TestEnterCritical(void) { ++critical; }
void TestExitCritical(void)
{
    assert(critical);
    --critical;
#if CHASSIS_IMU_CAN_RX_ENABLE
    if (!critical && interrupt_pending) receive_interrupt();
#endif
}
#if CHASSIS_IMU_CAN_RX_ENABLE
bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *out, uint32_t now)
{
    TestEnterCritical();
    *out = source_imu;
    TestExitCritical();
    const uint32_t age = now - out->first_rx_tick_ms;
    return out->available && out->sample.valid && out->sample.source_age_ms <= 20U &&
        age <= 60U && out->sample.source_age_ms <= 60U - age;
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
    assert(critical == 0); /* Encoding/transmission must not mask CAN interrupts. */
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
static void send_observation(const USBTelemetrySample_s *sample,
    const ChassisPowerSnapshot_s *power, const ChassisFusedVelocitySnapshot_s *velocity,
    const SuperCapSnapshot_s *cap, const RefereePowerSnapshot_s *referee, uint32_t now)
{
    tick = now;
    VofaObservationSend(sample, power, velocity, cap, referee, now);
}
static void check_frame(void)
{
    const uint8_t tail[] = {0, 0, 128, 127};
    assert(length == FRAME_BYTES);
    assert(memcmp(active + CHANNELS * 4, tail, 4) == 0);
    for (unsigned i = 0; i < CHANNELS; ++i) assert(isfinite(field(i)));
    for (unsigned i = 0; i + 4 <= CHANNELS * 4; ++i) assert(memcmp(active + i, tail, 4) != 0);
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
    source_imu.sample.valid = true;
    source_imu.sample.source_age_ms = 2;
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
    send_observation(&sample, &power, &velocity, &cap, &referee, start);
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
    assert(field(0) == 6);
    assert(field(VOFA_KALMAN_GIMBAL_WHEEL_N_X_MPS) == 1.25f);
    assert(field(VOFA_KALMAN_GIMBAL_ACCEL_N_Y_MPS2) == -0.5f);
    assert(fabsf(field(VOFA_KALMAN_GIMBAL_TARGET_VX_MPS) - 1.234f) < 1e-6f);
    assert(fabsf(field(VOFA_KALMAN_GIMBAL_TARGET_WZ_RADPS) - 1.5707963f) < 1e-6f);
    assert(field(VOFA_KALMAN_GIMBAL_ATTEMPT_FLAGS) == 7);
#if CHASSIS_IMU_CAN_RX_ENABLE
    assert(field(VOFA_KALMAN_GIMBAL_IMU_SOURCE) == 1);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_AX_MPS2) == 2.5f);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_AGE_MS) == 7);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_RECEIVE_AGE_MS) == 3);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_SOURCE_AGE_MS) == 2);
#endif
#endif
    uint8_t saved[FRAME_BYTES]; memcpy(saved, active, sizeof saved);
#if CHASSIS_IMU_CAN_RX_ENABLE
    source_imu.available = false;
#endif
    send_observation(NULL, NULL, NULL, NULL, NULL, start + 20U);
    assert(sent == 1 && memcmp(saved, active, sizeof saved) == 0);
    huart7.gState = HAL_UART_STATE_READY;
    send_observation(NULL, NULL, NULL, NULL, NULL, start + 39U);
    assert(sent == 1);
    send_observation(NULL, NULL, NULL, NULL, NULL, start + 40U);
    assert(sent == 2 && field(4) == 2 && field(5) == 0);
    check_frame();
    huart7.gState = HAL_UART_STATE_READY; fail_send = true;
#if VOFA_OBSERVATION_PROFILE == 2
    assert(field(VOFA_KALMAN_GIMBAL_YAW_AGE_MS) == -1);
    assert(field(VOFA_KALMAN_GIMBAL_CHASSIS_MODE) == -1);
    assert(field(VOFA_KALMAN_GIMBAL_GIMBAL_IMU_AGE_MS) == -1);
#endif
    send_observation(NULL, NULL, NULL, NULL, NULL, start + 60U);
    assert(sent == 2);
    fail_send = false;
    send_observation(NULL, NULL, NULL, NULL, NULL, start + 80U);
    assert(sent == 3 && field(4) == 4);
    huart7.gState = HAL_UART_STATE_READY;
    power.measured_power_w = NAN;
    velocity.remote.attempt.input.wheel_n_mps[0] = NAN;
    send_observation(&sample, &power, &velocity, &cap, &referee, start + 100U);
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
        send_observation(NULL, NULL, NULL, NULL, NULL, start + sequence * 20U);
        check_frame();
        assert((uint32_t)lroundf(field(3)) * 65536U + (uint32_t)lroundf(field(4)) == sequence);
    }
    const uint32_t jitter = start + 65537U * 20U;
    const uint32_t offsets[] = {0, 21, 40, 61, 80, 101};
    const unsigned before = sent;
    for (unsigned i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        huart7.gState = HAL_UART_STATE_READY;
        send_observation(NULL, NULL, NULL, NULL, NULL, jitter + offsets[i]);
        assert(sent == before + i + 1);
    }
    huart7.gState = HAL_UART_STATE_READY;
    send_observation(NULL, NULL, NULL, NULL, NULL, jitter + 181U);
    assert(sent == before + 7);
    huart7.gState = HAL_UART_STATE_READY;
    send_observation(NULL, NULL, NULL, NULL, NULL, jitter + 182U);
    assert(sent == before + 7);
#if CHASSIS_IMU_CAN_RX_ENABLE && VOFA_OBSERVATION_PROFILE == 2
    /* A complete group arrives after the caller sampled its scheduling clock. */
    const uint32_t scheduled = jitter + 220U;
    source_imu.available = source_imu.sample.valid = true;
    source_imu.sample.source_age_ms = 0;
    source_imu.first_rx_tick_ms = scheduled + 1U;
    source_imu.last_rx_tick_ms = scheduled + 2U;
    sample.chassis_tick_ms = sample.velocity_target_tick_ms = scheduled - 19U;
    velocity.diagnostics.available = true;
    velocity.diagnostics.wheel_online_mask = 1;
    velocity.diagnostics.wheel_tick_ms[0] = scheduled - 3U;
    tick = scheduled + 2U;
    huart7.gState = HAL_UART_STATE_READY;
    VofaObservationSend(&sample, NULL, &velocity, NULL, NULL, scheduled);
    assert((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 8U);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_AGE_MS) == 1);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_RECEIVE_AGE_MS) == 0);
    assert(field(VOFA_KALMAN_GIMBAL_WHEEL_AGE_MS_LF) == 5);
    assert(((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 2U) == 0); /* Target is now 21 ms old. */
    assert((uint32_t)field(1) * 65536U + (uint32_t)field(2) == tick);

    /* An IRQ between the fresh clock read and snapshot copy must be deferred.
     * Once unmasked it publishes a newer sequence, but this frame stays coherent. */
    tick = scheduled + 40U;
    source_imu.first_rx_tick_ms = tick - 1U;
    source_imu.last_rx_tick_ms = tick;
    source_imu.sample.sequence = 100;
    interrupt_on_tick = true;
    huart7.gState = HAL_UART_STATE_READY;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, tick);
    assert((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 8U);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_SEQUENCE_LO) == 100);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_AGE_MS) == 1);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_RECEIVE_AGE_MS) == 0);
    assert(source_imu.sample.sequence == 101 && !interrupt_pending);
    assert((uint32_t)field(1) * 65536U + (uint32_t)field(2) == scheduled + 40U);

    /* Refreshing the clock must preserve uint32 wrap and genuine invalidity. */
    source_imu.first_rx_tick_ms = UINT32_MAX;
    source_imu.last_rx_tick_ms = 0;
    tick = 1;
    huart7.gState = HAL_UART_STATE_READY;
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, UINT32_MAX - 1U);
    assert((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 8U);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_AGE_MS) == 2);
    assert(field(VOFA_KALMAN_GIMBAL_IMU_RECEIVE_AGE_MS) == 1);
    assert(field(1) == 0 && field(2) == 1);
    const uint16_t source_ages[] = {UINT16_MAX, 21, 0, 0};
    for (unsigned i = 0; i < 4; ++i) {
        source_imu.sample.source_age_ms = source_ages[i];
        source_imu.sample.valid = i != 2;
        source_imu.first_rx_tick_ms = source_imu.last_rx_tick_ms = i == 3 ? 0 : 100U + i * 20U;
        huart7.gState = HAL_UART_STATE_READY;
        send_observation(NULL, NULL, NULL, NULL, NULL, 100U + i * 20U);
        assert(((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 8U) == 0);
    }
#endif
#if VOFA_OBSERVATION_PROFILE == 2
    /* Raw encoder counts, circular offsets and independent local-head IMU.
     * Pitch validity must not gate yaw-only diagnostics. */
    const uint16_t encoders[] = {350, 2398, 6494, 8191, 350, 8192, 350, 350, 350};
    const float offsets_deg[] = {0, 90, -90, -15.4248046875f, 0, 0, 0, 0, 0};
    for (unsigned i = 0; i < sizeof encoders / sizeof encoders[0]; ++i) {
        const uint32_t now = 2000U + i * 20U;
        velocity.diagnostics = (ChassisVelocityDiagnostics_s){.available = true, .tick_ms = now,
            .yaw_encoder_valid = i != 6, .joint_ecd = {encoders[i], 8192},
            .joint_tick_ms = {now - (i == 4 ? 21U : 2U), 0}, .chassis_mode = i ? 1 : 3,
            .gimbal_imu_known = true, .gimbal_imu_valid = true,
            .gimbal_imu_tick_ms = now - (i == 8 ? 21U : 1U),
            .gimbal_yaw_total_deg = i == 7 ? NAN : 721.25f};
        huart7.gState = HAL_UART_STATE_READY;
        send_observation(NULL, NULL, &velocity, NULL, NULL, now);
        check_frame();
        assert(((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 256U) == (i < 4 || i > 6 ? 256U : 0U));
        assert(field(VOFA_KALMAN_GIMBAL_YAW_ECD) == (i < 4 || i > 6 ? encoders[i] : 0));
        assert(fabsf(field(VOFA_KALMAN_GIMBAL_YAW_OFFSET_DEG) - offsets_deg[i]) < 1e-5f);
        assert(field(VOFA_KALMAN_GIMBAL_YAW_AGE_MS) == (i == 6 ? -1 : i == 4 ? 21 : 2));
        assert(field(VOFA_KALMAN_GIMBAL_CHASSIS_MODE) == (i ? 1 : 3));
        assert(((unsigned)field(VOFA_KALMAN_GIMBAL_VALID) & 512U) == (i < 7 ? 512U : 0U));
        assert(field(VOFA_KALMAN_GIMBAL_GIMBAL_YAW_TOTAL_DEG) == (i < 7 ? 721.25f : 0));
    }
#endif
    puts("PASS: dedicated profile units, evidence, finite data and asynchronous transport");
    return 0;
}
