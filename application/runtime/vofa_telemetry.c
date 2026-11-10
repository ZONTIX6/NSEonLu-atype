#include "vofa_telemetry.h"

#if VOFA_TELEMETRY_ENABLE
#include "usart.h"
#if VOFA_TELEMETRY_MODE == 0
#include <string.h>

/* STM32F427 stores IEEE 754 floats in JustFloat's little-endian wire order. */
static struct {
    float channels[19];
    uint8_t tail[4];
} frame = {.tail = {0x00, 0x00, 0x80, 0x7f}};
_Static_assert(sizeof(float) == 4 && sizeof(frame) == 80, "JustFloat frame layout");

void VofaTelemetrySend(const USBTelemetrySample_s *sample)
{
    UART_HandleTypeDef *uart = BOARD_VOFA_UART;
    /* The IRQ reads this buffer until TX completion. Only this task starts TX. */
    if (uart->gState != HAL_UART_STATE_READY) return;

    const uint32_t motor_mask = sample->motor_online_mask & 0x0fU;
    const int32_t actual_rpm[] = {sample->rpm_lf, sample->rpm_rf, sample->rpm_lb, sample->rpm_rb};
    const int32_t target_rpm[] = {sample->target_lf_rpm, sample->target_rf_rpm,
                                  sample->target_lb_rpm, sample->target_rb_rpm};
    float *channels = frame.channels;
    for (unsigned wheel = 0; wheel < 4; ++wheel) {
        const bool online = (motor_mask & (1U << wheel)) != 0;
        channels[wheel] = online ? (float)actual_rpm[wheel] : 0.0f;
        channels[4 + wheel] = online ? (float)target_rpm[wheel] : 0.0f;
    }
    channels[8] = sample->meter_online ? sample->p_meter_cw * 0.01f : 0.0f;
    channels[9] = sample->power_valid ? sample->p_limited_cw * 0.01f : 0.0f;
    channels[10] = sample->power_valid ? sample->effective_limit_cw * 0.01f : 0.0f;
    channels[11] = sample->cap_online ? sample->cap_chassis_cw * 0.01f : 0.0f;
    channels[12] = sample->cap_input_online ? sample->cap_input_cw * 0.01f : 0.0f;
    channels[13] = sample->cap_online ? sample->cap_energy_pm * 0.1f : 0.0f;
    channels[14] = sample->imu_valid ? sample->imu_yaw_mdeg * 0.001f : 0.0f;
    channels[15] = sample->imu_valid ? sample->imu_pitch_mdeg * 0.001f : 0.0f;
    channels[16] = sample->imu_valid ? sample->imu_roll_mdeg * 0.001f : 0.0f;
    channels[17] = (float)motor_mask;
    channels[18] = (float)((sample->power_valid ? 1U : 0U) |
                           (sample->meter_online ? 2U : 0U) |
                           (sample->cap_online ? 4U : 0U) |
                           (sample->cap_input_online ? 8U : 0U) |
                           (sample->imu_valid ? 16U : 0U));

    /* JustFloat scans for the delimiter even across adjacent float boundaries.
     * In a collision, flip the lowest mantissa bit of the float ending the match. */
    uint8_t *bytes = (uint8_t *)&frame;
    for (size_t i = 0; i + sizeof(frame.tail) <= sizeof(frame.channels); ++i) {
        if (memcmp(bytes + i, frame.tail, sizeof(frame.tail)) == 0) {
            const size_t last = i + sizeof(frame.tail) - 1;
            bytes[last - last % sizeof(float)] ^= 1U;
        }
    }

    (void)HAL_UART_Transmit_IT(uart, bytes, sizeof(frame));
}
#else
#include <math.h>
#include <stdio.h>

enum {
    MEASUREMENT_PERIOD_MS = 60,
    MEASUREMENT_MAX_AGE_MS = 20,
    MEASUREMENT_CHANNELS = 38,
    // An int32 takes at most 11 characters plus one delimiter.
    MEASUREMENT_ROW_CAPACITY = MEASUREMENT_CHANNELS * 12 + 1
};

/* Only the telemetry task writes this buffer, while TX is idle. */
static char row[MEASUREMENT_ROW_CAPACITY];
_Static_assert((sizeof row - 1) * 10U * 1000U < 115200U * MEASUREMENT_PERIOD_MS,
               "FireWater row must fit in one send interval");

void VofaTelemetrySendMeasurement(const USBTelemetrySample_s *sample,
                                  const ChassisPowerSnapshot_s *cycle, uint32_t now_ms)
{
    static bool started;
    static uint32_t last_attempt_ms, next_sequence;
    if (started && (uint32_t)(now_ms - last_attempt_ms) < MEASUREMENT_PERIOD_MS) return;
    started = true;
    last_attempt_ms = now_ms;
    const uint32_t sequence = next_sequence++;

    UART_HandleTypeDef *uart = BOARD_VOFA_UART;
    if (uart->gState != HAL_UART_STATE_READY) return;

    const bool final_valid = cycle && cycle->registered && cycle->final_valid &&
                             (uint32_t)(now_ms - cycle->tick_ms) <= MEASUREMENT_MAX_AGE_MS;
    const bool power_valid = final_valid && sample->power_valid;
    const bool meter_known = final_valid &&
        (cycle->meter_online || cycle->meter_sequence || cycle->meter_tick_ms);
    const bool meter_valid = meter_known && cycle->meter_online &&
        (uint32_t)(now_ms - cycle->meter_tick_ms) <= MEASUREMENT_MAX_AGE_MS &&
        isfinite(cycle->measured_power_w);
    const uint32_t meter_age = meter_known ? (uint32_t)(cycle->tick_ms - cycle->meter_tick_ms) : UINT32_MAX;
    const uint32_t meter_sequence = meter_known ? cycle->meter_sequence : 0U;
    const uint32_t cycle_tick = final_valid ? cycle->tick_ms : 0U;
    uint32_t online = 0;
    if (final_valid) {
        online = sample->motor_online_mask & 15U;
        for (unsigned wheel = 0; wheel < 4; ++wheel) {
            if ((uint32_t)(now_ms - cycle->feedback_tick_ms[wheel]) > MEASUREMENT_MAX_AGE_MS)
                online &= ~(1U << wheel);
        }
    }
    const uint32_t valid = (power_valid ? 1U : 0U) | (meter_valid ? 2U : 0U) |
        (sample->cap_online ? 4U : 0U) | (sample->cap_input_online ? 8U : 0U) |
        (sample->imu_valid ? 16U : 0U) | (sample->referee_online ? 32U : 0U) |
        (final_valid ? 64U : 0U) | (final_valid && cycle->cycle_valid ? 128U : 0U);

    /* Fixed-point units and order are documented in docs/vofa-telemetry.md. */
    const int32_t fields[] = {
        online & 1U ? sample->rpm_lf : 0, online & 2U ? sample->rpm_rf : 0,
        online & 4U ? sample->rpm_lb : 0, online & 8U ? sample->rpm_rb : 0,
        online & 1U ? sample->target_lf_rpm : 0, online & 2U ? sample->target_rf_rpm : 0,
        online & 4U ? sample->target_lb_rpm : 0, online & 8U ? sample->target_rb_rpm : 0,
        meter_valid ? USBTelemetryFixed(cycle->measured_power_w, 100) : 0,
        power_valid ? sample->p_limited_cw : 0, power_valid ? sample->effective_limit_cw : 0,
        sample->cap_online ? sample->cap_chassis_cw : 0,
        sample->cap_input_online ? sample->cap_input_cw : 0,
        sample->cap_online ? sample->cap_energy_pm : 0,
        sample->imu_valid ? sample->imu_yaw_mdeg : 0, sample->imu_valid ? sample->imu_pitch_mdeg : 0,
        sample->imu_valid ? sample->imu_roll_mdeg : 0, online, valid,
        final_valid ? cycle->final_current[0] : 0, final_valid ? cycle->final_current[1] : 0,
        final_valid ? cycle->final_current[2] : 0, final_valid ? cycle->final_current[3] : 0,
        online & 1U ? sample->i_lf_fb : 0, online & 2U ? sample->i_rf_fb : 0,
        online & 4U ? sample->i_lb_fb : 0, online & 8U ? sample->i_rb_fb : 0,
        power_valid ? sample->p_unlimited_cw : 0, sample->referee_online ? sample->buffer_j : 0,
        meter_age <= INT32_MAX ? (int32_t)meter_age : -1,
        cycle_tick >> 16, cycle_tick & UINT16_MAX, sequence >> 16, sequence & UINT16_MAX,
        final_valid ? sample->motor_enabled_mask & 15U : 0U,
        meter_sequence >> 16, meter_sequence & UINT16_MAX, final_valid && cycle->can_submitted
    };
    _Static_assert(sizeof fields / sizeof fields[0] == MEASUREMENT_CHANNELS,
                   "FireWater channel count must match the row layout");

    size_t used = 0;
    for (size_t i = 0; i < MEASUREMENT_CHANNELS; ++i) {
        const int written = snprintf(row + used, sizeof row - used, "%ld,", (long)fields[i]);
        if (written <= 0 || (size_t)written >= sizeof row - used) return;
        used += (size_t)written;
    }
    row[used - 1] = '\n';
    (void)HAL_UART_Transmit_IT(uart, (uint8_t *)row, (uint16_t)used);
}
#endif
#endif
