#include "vofa_telemetry.h"
#include "board_config.h"

#if VOFA_TELEMETRY_ENABLE
#include "usart.h"
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
#endif
