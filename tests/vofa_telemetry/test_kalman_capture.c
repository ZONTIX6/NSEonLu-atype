#include "vofa_kalman_capture.h"
#include "vofa_observation.h"
#include "vofa_schema.h"
#include "usart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Each scenario runs in a fresh process: production has no test-only reset. */
enum { FRAME_BYTES = 388, LOG_CAPACITY = 512 };
UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static uint32_t tick, send_started;
static unsigned critical, sent, read_imu_calls, clock_reads;
static HAL_StatusTypeDef next_result = HAL_OK;
static const uint8_t *active;
static uint8_t frames[LOG_CAPACITY][FRAME_BYTES];
static ChassisFusedVelocitySnapshot_s velocity;
static ChassisImuCanSnapshot_s imu;
static ChassisVelocity_s target;
static bool capture_during_send;

uint32_t HAL_GetTick(void) { ++clock_reads; return tick; }
void TestEnterCritical(void) { ++critical; }
void TestExitCritical(void) { assert(critical); --critical; }
bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *out, uint32_t now)
{
    (void)now;
    ++read_imu_calls;
    *out = imu;
    return false; /* The queued source must not be replaced by this newer data. */
}

static void sample(uint32_t now, uint32_t attempt)
{
    memset(&velocity, 0, sizeof velocity);
    memset(&imu, 0, sizeof imu);
    velocity.status = CHASSIS_VELOCITY_FUSED;
    velocity.valid = true;
    velocity.tick_ms = now - 20U;
    velocity.wheel = (ChassisVelocity_s){.valid = true, .tick_ms = now - 2U,
        .vx_mps = .5f, .vy_mps = -.25f, .wz_rpm = 15.f};
    velocity.vx_mps = .45f;
    velocity.vy_mps = -.2f;
    velocity.diagnostics.available = true;
    velocity.diagnostics.tick_ms = now;
    velocity.diagnostics.wheel_online_mask = 15;
    for (unsigned i = 0; i < 4; ++i) {
        velocity.diagnostics.raw_rpm[i] = 100.f + (float)i;
        velocity.diagnostics.wheel_tick_ms[i] = now - 2U;
    }
    ChassisRemoteFusionAttempt_s *a = &velocity.remote.attempt;
    a->available = true;
    a->sequence = attempt;
    a->receiver_call_ms = now;
    a->middle_receiver_ms = now - 20U;
    a->input = (ChassisVelocityFusionInput_s){.now_us = now * 1000U,
        .wheel_us = (now - 20U) * 1000U, .imu_us = (now - 20U) * 1000U,
        .wheel_n_mps = {1.25f, -2.5f}, .accel_n_mps2 = {.125f, -.5f},
        .wheel_valid = true, .imu_valid = true};
    a->output = (ChassisVelocityFusionOutput_s){.valid = true, .fused = true,
        .velocity_n_mps = {1.f, -2.f}, .sample_us = a->input.wheel_us};
    a->config.sensor_to_body_quaternion[0] = 1;
    a->config.imu_offset_body_m[1] = .116f;
    a->config.filter = (ChassisVelocityFusionConfig_s){.wheel_variance = .01f,
        .accel_variance = .25f, .jerk_density = 1.f, .max_velocity_residual_mps = 1.5f,
        .max_age_us = 60000, .max_skew_us = 0, .max_gap_us = 60000};
    a->effective_accel_variance = 1.75f;
    a->config.spin_accel_uncertainty_fraction = .25f;
    a->config.max_accel_variance = 4.f;
    a->config.max_pair_span_ms = 15;
    a->config.max_wheel_skew_ms = 3;
    a->config.max_timing_jitter_ms = 10;
    a->pair_span_ms = 4;
    a->source_step_ms = a->receive_step_ms = 20;
    imu.available = imu.sample.valid = true;
    imu.first_rx_tick_ms = now - 4U;
    imu.last_rx_tick_ms = now - 2U;
    imu.sample.source_age_ms = 1;
    imu.sample.sequence = attempt + 100U;
    imu.sample.quaternion[0] = 1;
    imu.sample.accel_n_mps2[0] = 2.5f;
    target = (ChassisVelocity_s){.valid = true, .tick_ms = now - 3U,
        .vx_mps = 1.2345f, .vy_mps = -.75f, .wz_rpm = 15.f};
}

static void capture(uint32_t now, uint32_t attempt)
{
    sample(now, attempt);
    VofaKalmanCapture(&velocity, &imu, true, &target, now);
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
    const uint8_t *data, uint16_t length)
{
    assert(uart == &huart7 && length == FRAME_BYTES && critical == 0);
    if (next_result != HAL_OK) {
        HAL_StatusTypeDef result = next_result;
        next_result = HAL_OK;
        return result;
    }
    active = data;
    if (sent < LOG_CAPACITY) memcpy(frames[sent], data, FRAME_BYTES);
    ++sent;
    send_started = tick;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    /* Model the higher-priority observer preempting the consumer before pop. */
    if (capture_during_send) {
        capture_during_send = false;
        capture(tick + 20U, 2);
    }
    return HAL_OK;
}

static float field(const uint8_t *frame, unsigned index)
{
    float value;
    memcpy(&value, frame + index * sizeof(float), sizeof value);
    return value;
}

static uint32_t word(const uint8_t *frame, unsigned index)
{
    return (uint32_t)lroundf(field(frame, index)) * 65536U +
        (uint32_t)lroundf(field(frame, index + 1));
}

static void check_frame(const uint8_t *frame)
{
    const uint8_t tail[] = {0, 0, 128, 127};
    assert(field(frame, 0) == 5 && memcmp(frame + 384, tail, 4) == 0);
    for (unsigned i = 0; i < 96; ++i) assert(isfinite(field(frame, i)));
    for (unsigned i = 0; i + 4 <= 384; ++i) assert(memcmp(frame + i, tail, 4) != 0);
    assert((unsigned)lroundf(field(frame, VOFA_KALMAN_VALID)) & 64U);
}

static VofaKalmanCaptureStats_s stats(void)
{
    VofaKalmanCaptureStats_s result;
    VofaKalmanCaptureGetStats(&result);
    assert(result.captured == result.sent + result.pending);
    assert(result.high_water <= 16U && result.pending <= 16U);
    return result;
}

static void finish_uart(void)
{
    huart7.gState = HAL_UART_STATE_READY;
}

static void startup_preserves_frozen_records(void)
{
    capture(1000, 1);
    capture(1020, 2);
    assert(stats().pending == 2 && sent == 0);
    memset(&velocity, 0, sizeof velocity);
    memset(&imu, 0, sizeof imu);
    memset(&target, 0, sizeof target);
    tick = 5000; /* A slow consumer must not age a recorded sample at send time. */
    VofaObservationSend(NULL, NULL, NULL, NULL, NULL, tick);
    assert(sent == 1 && stats().pending == 1);
    check_frame(active);
    assert(word(active, VOFA_KALMAN_TICK_MS_HI) == 1000);
    assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == 0);
    assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 1);
    assert((unsigned)field(active, VOFA_KALMAN_VALID) == 95U);
    assert(field(active, VOFA_KALMAN_ATTEMPT_FLAGS) == 55U); /* Known initial state. */
    assert(field(active, VOFA_KALMAN_IMU_AGE_MS) == 5);
    assert(field(active, VOFA_KALMAN_IMU_RECEIVE_AGE_MS) == 2);
    assert(field(active, VOFA_KALMAN_IMU_SOURCE_AGE_MS) == 1);
    assert(field(active, VOFA_KALMAN_VELOCITY_AGE_MS) == 20);
    assert(field(active, VOFA_KALMAN_IMU_AX_MPS2) == 2.5f);
    assert(field(active, VOFA_KALMAN_FUSION_ACCEL_VARIANCE) == 1.75f);
    assert(fabsf(field(active, VOFA_KALMAN_TARGET_VX_MPS) - 1.2345f) < 1e-6f);
    assert(fabsf(field(active, VOFA_KALMAN_TARGET_WZ_RADPS) - 1.5707963f) < 1e-6f);
    finish_uart();
    VofaKalmanCaptureDrain();
    assert(sent == 2 && stats().pending == 0);
    assert(word(active, VOFA_KALMAN_TICK_MS_HI) == 1020);
    assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == 1);
    assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 2);
    finish_uart();
    VofaKalmanCaptureDrain();
    assert(sent == 2 && read_imu_calls == 0 && clock_reads == 0);
}

static void retry_and_preemption_preserve_the_queue(void)
{
    capture(1000, 1);
    tick = 1000;
    next_result = HAL_ERROR;
    VofaKalmanCaptureDrain();
    assert(sent == 0 && stats().pending == 1 && stats().send_errors == 1);
    next_result = HAL_BUSY;
    VofaKalmanCaptureDrain();
    assert(sent == 0 && stats().pending == 1);
    capture_during_send = true;
    VofaKalmanCaptureDrain();
    assert(sent == 1 && stats().pending == 1);
    assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == 0);
    uint8_t saved[FRAME_BYTES];
    memcpy(saved, active, sizeof saved);
    for (unsigned i = 0; i < 25; ++i) VofaKalmanCaptureDrain();
    assert(sent == 1 && memcmp(saved, active, sizeof saved) == 0);
    finish_uart();
    VofaKalmanCaptureDrain();
    assert(sent == 2 && stats().pending == 0);
    assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == 1);
    assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 2);
    assert(read_imu_calls == 0);
}

static void overflow_is_counted_without_overwriting_pending_records(void)
{
    for (uint32_t i = 0; i < 18; ++i) capture(1000U + i * 20U, i + 1U);
    VofaKalmanCaptureStats_s before = stats();
    assert(before.pending == 16 && before.captured == 16 && before.dropped == 2);
    assert(before.high_water == 16);
    for (uint32_t i = 0; i < 16; ++i) {
        finish_uart();
        VofaKalmanCaptureDrain();
        assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == i);
        assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == i + 1U);
        check_frame(active);
    }
    assert(stats().pending == 0);
    capture(1361, 19);
    finish_uart();
    VofaKalmanCaptureDrain();
    assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == 18); /* Dropped 16 and 17. */
    assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 19);
    assert((unsigned)lroundf(field(active, VOFA_KALMAN_VALID)) & 128U);
    assert(stats().sent == 17 && stats().dropped == 2);
}

static void jitter_and_consumer_stall_do_not_skip_calls(void)
{
    enum { EVENTS = 400 };
    uint32_t event_times[EVENTS];
    event_times[0] = 1000;
    for (unsigned i = 1; i < EVENTS; ++i)
        event_times[i] = event_times[i-1] + (i & 1U ? 19U : 21U);
    unsigned next = 0;
    for (tick = 1000; tick <= event_times[EVENTS-1] + 200U; ++tick) {
        if (huart7.gState == HAL_UART_STATE_BUSY_TX && tick - send_started >= 17U)
            finish_uart();
        if (next < EVENTS && tick == event_times[next]) {
            capture(tick, 100U + next);
            ++next;
        } else if (next && next < EVENTS) {
            VofaKalmanCapture(&velocity, &imu, true, &target, tick);
        }
        /* USB work or a scheduler delay temporarily prevents UART draining. */
        if (tick < 3000U || tick >= 3070U) VofaKalmanCaptureDrain();
    }
    assert(next == EVENTS && sent == EVENTS);
    assert(stats().dropped == 0 && stats().pending == 0);
    assert(stats().high_water >= 3);
    for (unsigned i = 0; i < EVENTS; ++i) {
        check_frame(frames[i]);
        assert(word(frames[i], VOFA_KALMAN_TX_SEQUENCE_HI) == i);
        assert(word(frames[i], VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 100U + i);
        assert(word(frames[i], VOFA_KALMAN_TICK_MS_HI) == event_times[i]);
    }
    assert(read_imu_calls == 0);
}

static void heartbeat_and_rejected_calls_remain_visible(void)
{
    sample(1000, 1);
    velocity.status = CHASSIS_VELOCITY_NO_SAMPLE;
    velocity.remote.attempt.available = false;
    VofaKalmanCapture(&velocity, &imu, true, &target, 1000);
    VofaKalmanCapture(&velocity, &imu, true, &target, 1001);
    velocity.status = CHASSIS_VELOCITY_WARMUP;
    VofaKalmanCapture(&velocity, &imu, true, &target, 1002);
    VofaKalmanCapture(&velocity, &imu, true, &target, 1101);
    assert(stats().captured == 2);
    VofaKalmanCapture(&velocity, &imu, true, &target, 1102);
    capture(1120, 1);
    VofaKalmanCapture(&velocity, &imu, true, &target, 1121);
    sample(1140, 2);
    velocity.status = CHASSIS_VELOCITY_REJECTED;
    velocity.remote.attempt.output.fused = false;
    VofaKalmanCapture(&velocity, &imu, true, &target, 1140);
    velocity.remote.attempt.sequence = 3;
    velocity.remote.attempt.receiver_call_ms = 1160;
    VofaKalmanCapture(&velocity, &imu, true, &target, 1160);
    assert(stats().captured == 6);
    for (unsigned i = 0; i < 6; ++i) {
        finish_uart();
        VofaKalmanCaptureDrain();
        assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == i);
    }
    assert(field(frames[0], VOFA_KALMAN_OBSERVER_STATUS) == CHASSIS_VELOCITY_NO_SAMPLE);
    assert(field(frames[1], VOFA_KALMAN_OBSERVER_STATUS) == CHASSIS_VELOCITY_WARMUP);
    assert(word(frames[2], VOFA_KALMAN_TICK_MS_HI) == 1102);
    assert(word(frames[4], VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 2);
    assert(word(frames[5], VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 3);
    assert(((unsigned)field(frames[4], VOFA_KALMAN_ATTEMPT_FLAGS) & 32U) == 0);
    assert(((unsigned)field(frames[5], VOFA_KALMAN_ATTEMPT_FLAGS) & 32U) == 0);
    assert(stats().pending == 0);
}

static void clock_attempt_and_wire_sequence_rollovers(void)
{
    capture(UINT32_MAX - 10U, UINT32_MAX);
    VofaKalmanCaptureDrain();
    assert(word(active, VOFA_KALMAN_TICK_MS_HI) == UINT32_MAX - 10U);
    assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == UINT32_MAX);
    capture(9, 0);
    finish_uart();
    VofaKalmanCaptureDrain();
    assert(word(active, VOFA_KALMAN_TICK_MS_HI) == 9);
    assert(word(active, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI) == 0);
    for (uint32_t i = 2; i <= 65536U; ++i) {
        capture(9U + (i - 1U) * 20U, i - 1U);
        finish_uart();
        VofaKalmanCaptureDrain();
        check_frame(active);
        assert(word(active, VOFA_KALMAN_TX_SEQUENCE_HI) == i);
    }
    assert(stats().sent == 65537U && stats().dropped == 0);
    assert(field(active, VOFA_KALMAN_TX_SEQUENCE_HI) == 1);
    assert(field(active, VOFA_KALMAN_TX_SEQUENCE_LO) == 0);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "startup")) startup_preserves_frozen_records();
    else if (!strcmp(argv[1], "retry")) retry_and_preemption_preserve_the_queue();
    else if (!strcmp(argv[1], "overflow")) overflow_is_counted_without_overwriting_pending_records();
    else if (!strcmp(argv[1], "jitter")) jitter_and_consumer_stall_do_not_skip_calls();
    else if (!strcmp(argv[1], "heartbeat")) heartbeat_and_rejected_calls_remain_visible();
    else if (!strcmp(argv[1], "wrap")) clock_attempt_and_wire_sequence_rollovers();
    else assert(!"unknown scenario");
    assert(critical == 0);
    printf("PASS: Kalman capture %s\n", argv[1]);
    return 0;
}
