#include "usb_telemetry.h"
#include "bsp_usb.h"
#include <stdbool.h>

static char header[USB_TELEMETRY_BUFFER_SIZE];
static char row[USB_TELEMETRY_BUFFER_SIZE];
static size_t header_length;
static uint32_t sequence, drop_count, session, header_tick;
static bool header_needed;

void USBTelemetryStreamInit(void)
{
    USB_Init_Config_s config = {0};
    (void)USBInit(config);
    header_length = USBTelemetryFormatHeader(header, sizeof header);
    sequence = drop_count = header_tick = 0;
    session = CDC_GetSession_FS();
    header_needed = true;
}

void USBTelemetryStreamSend(USBTelemetrySample_s *sample, uint32_t now_ms)
{
    if (sample == NULL) return;
    sample->version = 1;
    sample->seq = sequence++;
    sample->tick = now_ms;
    sample->tx_drop_count = drop_count;
    const uint32_t current_session = CDC_GetSession_FS();
    if (session != current_session) {
        session = current_session;
        header_needed = true;
    }
    if (!CDC_IsPortOpen_FS()) return;
    /* Never format into a buffer while the endpoint still owns its bytes. */
    if (!CDC_IsTransmitIdle_FS()) {
        ++drop_count;
        return;
    }
    if (header_needed || (uint32_t)(now_ms - header_tick) >= USB_TELEMETRY_HEADER_PERIOD_MS) {
        if (header_length && USBTransmit((uint8_t *)header, (uint16_t)header_length) == USBD_OK) {
            header_needed = false;
            header_tick = now_ms;
        } else {
            ++drop_count;
        }
        return;
    }
    const size_t length = USBTelemetryFormatRow(sample, row, sizeof row);
    /* A reset/reopen during formatting must get its header before a data row. */
    if (!length || CDC_GetSession_FS() != session || !CDC_IsPortOpen_FS() ||
        USBTransmit((uint8_t *)row, (uint16_t)length) != USBD_OK)
        ++drop_count;
}
