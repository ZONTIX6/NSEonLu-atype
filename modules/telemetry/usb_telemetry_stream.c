#include "usb_telemetry.h"
#include "bsp_usb.h"
#include <stdbool.h>
#include "runtime_tuning.h"
#include "autotune_protocol.h"
#include "force_calibration_protocol.h"
#include "chassis_control_config.h"
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
#include "chassis_force_telemetry.h"
#include "dji_force_adapter.h"
#include "super_cap.h"
#include "rm_referee.h"
static ChassisForceCycleOutput_s force_sample;
static ChassisPowerSnapshot_s force_power;
static SuperCapSnapshot_s force_cap;
static RefereePowerSnapshot_s force_referee;
#endif
#include <string.h>

static char header[USB_TELEMETRY_BUFFER_SIZE];
static char row[USB_TELEMETRY_BUFFER_SIZE];
static size_t header_length;
static uint32_t sequence, drop_count, session, header_tick;
static bool header_needed;
static uint8_t *receive_buffer;
static uint8_t rx_ring[1024];
static volatile unsigned rx_head, rx_tail;
static volatile bool rx_overflow;
static volatile uint32_t rx_session;
static char command[TUNING_LINE_SIZE], reply[TUNING_LINE_SIZE];
static size_t command_length;
static bool command_dropping;

/* USB interrupt: copy only; parsing and control changes run in tasks. */
static void Receive(uint16_t length)
{
    if (!receive_buffer || !CDC_IsPortOpen_FS()) return;
    uint32_t current = CDC_GetSession_FS();
    if (rx_session != current) {
        rx_head = rx_tail = 0;
        rx_overflow = false;
        rx_session = current;
    }
    for (uint32_t i = 0; i < length; ++i) {
        unsigned next = (rx_head + 1U) % sizeof rx_ring;
        if (next == rx_tail) {rx_overflow = true; break;}
        rx_ring[rx_head] = receive_buffer[i];
        rx_head = next;
    }
}
static void ReadCommands(void)
{
    for (unsigned budget = 0; budget < sizeof rx_ring; ++budget) {
        const uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (rx_overflow) {
            ForceCalibrationProtocolCancel(CDC_GetSession_FS(),FC_REASON_PROTOCOL);
            rx_tail = rx_head; rx_overflow = false;
            command_length = 0; command_dropping = true;
        }
        if (rx_head == rx_tail) {__set_PRIMASK(primask); break;}
        uint8_t byte = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1U) % sizeof rx_ring;
        __set_PRIMASK(primask);
        if (byte == '\n') {
            if (!command_dropping) {
                if (command_length && command[command_length - 1] == '\r') --command_length;
                command[command_length] = 0;
                if (!strncmp(command,"@FC1,",5)) ForceCalibrationProtocolSubmit(command,CDC_GetSession_FS());
                else if (!strncmp(command,"@A1,",4) || !strncmp(command,"@A2,",4)) AutotuneSubmit(command);
                else RuntimeTuningSubmit(command);
            }
            command_length = 0; command_dropping = false;
        } else if (!command_dropping) {
            if (byte == 0 || command_length >= sizeof command - 1) {
                ForceCalibrationProtocolCancel(CDC_GetSession_FS(),FC_REASON_PROTOCOL);
                command_length = 0; command_dropping = true;
            } else command[command_length++] = (char)byte;
        }
    }
}

void USBTelemetryStreamInit(void)
{
    USB_Init_Config_s config = {.rx_cbk = Receive};
    receive_buffer = USBInit(config);
    rx_head = rx_tail = 0;
    rx_overflow = false;
    command_length = 0; command_dropping = false;
    RuntimeTuningCancel();
    AutotuneCancel();
    ForceCalibrationProtocolInit();
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
    header_length=ChassisForceTelemetryHeader(header,sizeof header);
#else
    header_length = USBTelemetryFormatHeader(header, sizeof header);
#endif
    sequence = drop_count = header_tick = 0;
    session = CDC_GetSession_FS();
    rx_session = session;
    header_needed = true;
}

void USBTelemetryStreamSend(USBTelemetrySample_s *sample, uint32_t now_ms)
{
    if (sample == NULL) return;
    sample->version = 5;
    sample->seq = sequence++;
    sample->tick = now_ms;
    sample->tx_drop_count = drop_count;
    const uint32_t current_session = CDC_GetSession_FS();
    if (session != current_session) {
        session = current_session;
        header_needed = true;
        RuntimeTuningCancel();
        AutotuneCancel();
        ForceCalibrationProtocolCancel(current_session,FC_REASON_USB);
        command_length = 0; command_dropping = false;
        const uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (rx_session != session) {
            rx_head = rx_tail = 0; rx_overflow = false; rx_session = session;
        }
        __set_PRIMASK(primask);
    }
    if (!CDC_IsPortOpen_FS()) {AutotuneCancel();ForceCalibrationProtocolCancel(current_session,FC_REASON_USB);return;}
    ReadCommands();
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
    size_t reply_length=ForceCalibrationProtocolReply(reply,sizeof reply);
    bool calibration_reply=reply_length>0;
    if(!reply_length) reply_length = RuntimeTuningReply(reply, sizeof reply);
    bool autotune_reply=false;
    if (!reply_length) {reply_length=AutotuneReply(reply,sizeof reply);autotune_reply=reply_length>0;}
    if (reply_length) {
        if (USBTransmit((uint8_t *)reply, (uint16_t)reply_length) == USBD_OK) {
            if(calibration_reply) ForceCalibrationProtocolReplySent();
            else if (autotune_reply) AutotuneReplySent();else RuntimeTuningReplySent();
        }
        else ++drop_count;
        return;
    }
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
    (void)DJIForceAdapterGetSnapshot(&force_sample);
    (void)ChassisPowerGetSnapshot(&force_power);
    (void)SuperCapGetSnapshot(now_ms,&force_cap);
    (void)RefereeGetPowerSnapshot(&force_referee);
    const size_t length=ChassisForceTelemetryRow(&force_sample,&force_power,&force_cap,&force_referee,row,sizeof row);
#else
    const size_t length = USBTelemetryFormatRow(sample, row, sizeof row);
#endif
    /* A reset/reopen during formatting must get its header before a data row. */
    if (!length || CDC_GetSession_FS() != session || !CDC_IsPortOpen_FS() ||
        USBTransmit((uint8_t *)row, (uint16_t)length) != USBD_OK)
        ++drop_count;
}
