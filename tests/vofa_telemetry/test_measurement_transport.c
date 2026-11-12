#include "vofa_telemetry.h"
#include "usart.h"
#include "justfloat_decode.h"
#include <stdio.h>

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static const uint8_t *active;
static uint16_t active_length;
static unsigned frames;
static bool fail_send;

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t length)
{
    assert(uart == &huart7 && uart->gState == HAL_UART_STATE_READY);
    if (fail_send) return HAL_ERROR;
    active = data;
    active_length = length;
    uart->gState = HAL_UART_STATE_BUSY_TX;
    ++frames;
    return HAL_OK;
}

int main(void)
{
    USBTelemetrySample_s sample = {0};
    ChassisFusedVelocitySnapshot_s velocity = {.diagnostics = {
        .available = true, .wheel_online_mask = 3, .raw_rpm = {0, 1022},
    }};
    const uint32_t start = UINT32_MAX - 19U;
    velocity.diagnostics.tick_ms = start;
    velocity.diagnostics.wheel_tick_ms[0] = velocity.diagnostics.wheel_tick_ms[1] = start;
    int64_t fields[64];
    VofaTelemetrySendMeasurement(&sample, NULL, &velocity, start);
    assert(frames == 1);
    ReadJustFloatFields(active, active_length, fields, 64);
    assert(fields[0] == 2 && fields[13] == 0 && fields[14] == 1022);
    assert(fields[1] * 65536 + fields[2] == start);
    uint8_t saved[260];
    memcpy(saved, active, sizeof saved);

    /* A busy attempt consumes its sequence but may not overwrite its buffer. */
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + 20U);
    assert(frames == 1 && memcmp(saved, active, sizeof saved) == 0);
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + 39U);
    assert(frames == 1);
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + 40U);
    assert(frames == 2);
    ReadJustFloatFields(active, active_length, fields, 64);
    assert(fields[4] == 2 && fields[1] * 65536 + fields[2] == start + 40U);

    huart7.gState = HAL_UART_STATE_READY;
    fail_send = true;
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + 60U);
    assert(frames == 2);
    fail_send = false;
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + 79U);
    assert(frames == 2);
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + 80U);
    assert(frames == 3);

    /* Check every frame through the low-half rollover, including 0,1022 in
     * sequence metadata: no false delimiters and metadata rounds correctly. */
    for (uint32_t sequence = 4; sequence <= 65536U; ++sequence) {
        if (sequence > 4) {
            huart7.gState = HAL_UART_STATE_READY;
            VofaTelemetrySendMeasurement(&sample, NULL, NULL, start + sequence * 20U);
        }
        ReadJustFloatFields(active, active_length, fields, 64);
        assert(fields[3] * 65536 + fields[4] == sequence);
        assert(fields[1] * 65536 + fields[2] == (uint32_t)(start + sequence * 20U));
        assert(fields[5] == 0 && fields[6] == -1);
    }
    /* A late task wake must not shift the next slot and halve the output rate.
     * Each adjacent frame still has enough wire time at 230400 baud. */
    const uint32_t jitter_start = start + 65537U * 20U;
    const uint32_t offsets[] = {0, 21, 40, 61, 80, 101};
    const unsigned before_jitter = frames;
    for (unsigned i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        huart7.gState = HAL_UART_STATE_READY;
        VofaTelemetrySendMeasurement(&sample, NULL, NULL, jitter_start + offsets[i]);
        assert(frames == before_jitter + i + 1);
        ReadJustFloatFields(active, active_length, fields, 64);
        assert(fields[3] * 65536 + fields[4] == 65537U + i);
        assert(fields[1] * 65536 + fields[2] == (uint32_t)(jitter_start + offsets[i]));
    }
    /* After a long pause, send only the latest snapshot, never a catch-up burst. */
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, jitter_start + 181U);
    assert(frames == before_jitter + 7);
    huart7.gState = HAL_UART_STATE_READY;
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, jitter_start + 182U);
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, jitter_start + 199U);
    assert(frames == before_jitter + 7);
    VofaTelemetrySendMeasurement(&sample, NULL, NULL, jitter_start + 200U);
    assert(frames == before_jitter + 8);
    puts("PASS: full JustFloat 20 ms slots, jitter, async ownership, rollover and delimiter safety");
    return 0;
}
