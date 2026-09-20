#include "super_cap.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdlib.h>
#include <string.h>

#define SUPERCAP_DEFAULT_OFFLINE_RELOAD_COUNT 20U

static SuperCapInstance *cap;

static void SuperCapOffline(void *owner)
{
    SuperCapInstance *instance = (SuperCapInstance *)owner;
    if (instance == NULL)
        return;

    taskENTER_CRITICAL();
    instance->status.online = false;
    taskEXIT_CRITICAL();
}

static void SuperCapRxCallback(CANInstance *can_instance)
{
    if (can_instance == NULL)
        return;

    SuperCapInstance *instance = (SuperCapInstance *)can_instance->id;
    if (instance == NULL)
        return;

    SuperCapStatus_s decoded;
    if (!SuperCapProtocolDecodeStatus(can_instance->rx_buff,
                                      can_instance->rx_len,
                                      &decoded))
    { // 帧收到了但内容不合法,单独计数,以便和"完全没收到帧"区分开
        instance->rejected_frame_count++;
        return;
    }

    instance->status = decoded;
    instance->received_frame_count++;
    instance->last_rx_tick_ms = HAL_GetTick();
    DaemonReload(instance->daemon);
}

bool SuperCapInit(const SuperCap_Init_Config_s *config)
{
    if (config == NULL || cap != NULL)
        return false;

    SuperCapInstance *instance = (SuperCapInstance *)malloc(sizeof(SuperCapInstance));
    if (instance == NULL)
        return false;
    memset(instance, 0, sizeof(*instance));

    const uint16_t reload_count = config->offline_reload_count == 0U
                                      ? SUPERCAP_DEFAULT_OFFLINE_RELOAD_COUNT
                                      : config->offline_reload_count;
    Daemon_Init_Config_s daemon_config = {
        .reload_count = reload_count,
        .init_count = reload_count,
        .callback = SuperCapOffline,
        .owner_id = instance,
    };
    instance->daemon = DaemonRegister(&daemon_config);
    if (instance->daemon == NULL)
    {
        free(instance);
        return false;
    }

    CAN_Init_Config_s can_config = config->can_config;
    can_config.can_module_callback = SuperCapRxCallback;
    can_config.id = instance;
    instance->can_ins = CANRegister(&can_config);
    if (instance->can_ins == NULL)
        return false;

    CANSetDLC(instance->can_ins, SUPERCAP_CAN_DLC);
    cap = instance;
    return true;
}

bool SuperCapSendCommand(const SuperCapCommand_s *command)
{
    if (cap == NULL || cap->can_ins == NULL ||
        !SuperCapProtocolEncodeCommand(command, cap->can_ins->tx_buff))
        return false;

    return CANTransmit(cap->can_ins, 1.0f) != 0U;
}

bool SuperCapGetStatus(SuperCapStatus_s *status)
{
    if (cap == NULL || status == NULL || cap->daemon == NULL)
        return false;

    taskENTER_CRITICAL();
    *status = cap->status;
    status->online = status->online && DaemonIsOnline(cap->daemon) != 0U;
    taskEXIT_CRITICAL();
    return true;
}
