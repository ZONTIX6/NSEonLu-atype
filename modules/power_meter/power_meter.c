#include "power_meter.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdlib.h>
#include <string.h>

#define POWER_METER_DEFAULT_OFFLINE_RELOAD_COUNT 10U

static void PowerMeterOffline(void *owner)
{
    PowerMeterInstance *instance = (PowerMeterInstance *)owner;
    if (instance == NULL)
        return;

    taskENTER_CRITICAL();
    if (DaemonIsOnline(instance->daemon) == 0U)
        instance->status.online = false;
    taskEXIT_CRITICAL();
}

static void PowerMeterRxCallback(CANInstance *can_instance)
{
    if (can_instance == NULL)
        return;

    PowerMeterInstance *instance = (PowerMeterInstance *)can_instance->id;
    if (instance == NULL)
        return;

    PowerMeterMeasurement_s decoded;
    if (!PowerMeterProtocolDecodeStatus(can_instance->rx_buff,
                                        can_instance->rx_len,
                                        &decoded))
    {
        instance->rejected_frame_count++;
        return;
    }

    instance->status.online = true;
    instance->status.shunt_voltage_v = decoded.shunt_voltage_v;
    instance->status.bus_voltage_v = decoded.bus_voltage_v;
    instance->status.current_a = decoded.current_a;
    instance->status.power_w = decoded.power_w;
    instance->received_frame_count++;
    instance->last_rx_tick_ms = HAL_GetTick();
    DaemonReload(instance->daemon);
}

PowerMeterInstance *PowerMeterInit(const PowerMeter_Init_Config_s *config)
{
    if (config == NULL)
        return NULL;

    PowerMeterInstance *instance =
        (PowerMeterInstance *)malloc(sizeof(PowerMeterInstance));
    if (instance == NULL)
        return NULL;
    memset(instance, 0, sizeof(*instance));

    const uint16_t reload_count =
        config->offline_reload_count == 0U
            ? POWER_METER_DEFAULT_OFFLINE_RELOAD_COUNT
            : config->offline_reload_count;
    Daemon_Init_Config_s daemon_config = {
        .reload_count = reload_count,
        .init_count = reload_count,
        .callback = PowerMeterOffline,
        .owner_id = instance,
    };
    instance->daemon = DaemonRegister(&daemon_config);
    if (instance->daemon == NULL)
    {
        free(instance);
        return NULL;
    }

    CAN_Init_Config_s can_config = config->can_config;
    can_config.can_module_callback = PowerMeterRxCallback;
    can_config.id = instance;
    instance->can_ins = CANRegister(&can_config);
    if (instance->can_ins == NULL)
        return NULL;

    CANSetDLC(instance->can_ins, POWER_METER_CAN_DLC);
    return instance;
}

bool PowerMeterGetStatus(PowerMeterInstance *instance,
                         PowerMeterStatus_s *status)
{
    if (instance == NULL || status == NULL || instance->daemon == NULL)
        return false;

    taskENTER_CRITICAL();
    *status = instance->status;
    status->online =
        status->online && DaemonIsOnline(instance->daemon) != 0U;
    taskEXIT_CRITICAL();
    return true;
}
