#include "super_cap.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdlib.h>
#include <string.h>

#define SUPERCAP_DEFAULT_OFFLINE_RELOAD_COUNT 20U

typedef struct
{
    CANInstance *can_ins, *input_can_ins;
    DaemonInstance *daemon;
    SuperCapSnapshot_s snapshot;
    bool active;
} SuperCapInstance;

static SuperCapInstance *cap;
static bool cap_init_started;

static void SuperCapOffline(void *owner)
{
    SuperCapInstance *instance = (SuperCapInstance *)owner;
    if (instance == NULL || !instance->active)
        return;

    taskENTER_CRITICAL();
    instance->snapshot.status.online = false;
    taskEXIT_CRITICAL();
}

static void SuperCapRxCallback(CANInstance *can_instance)
{
    if (can_instance == NULL)
        return;

    SuperCapInstance *instance = (SuperCapInstance *)can_instance->id;
    if (instance == NULL || !instance->active)
        return;

    SuperCapStatus_s decoded;
    if (!SuperCapProtocolDecodeStatus(can_instance->rx_buff,
                                      can_instance->rx_len,
                                      &decoded))
    { // 帧收到了但内容不合法,单独计数,以便和"完全没收到帧"区分开
        instance->snapshot.rejected_frame_count++;
        return;
    }

    decoded.last_rx_tick_ms = HAL_GetTick();
    instance->snapshot.status = decoded;
    instance->snapshot.received_frame_count++;
    DaemonReload(instance->daemon);
}

static void SuperCapInputRxCallback(CANInstance *can_instance)
{
    if (can_instance == NULL || can_instance->id == NULL)
        return;
    SuperCapInstance *instance = (SuperCapInstance *)can_instance->id;
    if (!instance->active) return;
    SuperCapInputPower_s decoded;
    if (!SuperCapProtocolDecodeInputPower(can_instance->rx_buff,
                                          can_instance->rx_len, &decoded))
    {
        instance->snapshot.input_rejected_frame_count++;
        return;
    }
    decoded.last_rx_tick_ms = HAL_GetTick();
    instance->snapshot.input_power = decoded;
    instance->snapshot.input_received_frame_count++;
    /* This frame must not keep an absent 0x051 load/status frame online. */
}

bool SuperCapInit(const SuperCap_Init_Config_s *config)
{
    if (config == NULL || cap_init_started)
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
    cap_init_started = true;

    CAN_Init_Config_s can_config = config->can_config;
    can_config.can_module_callback = SuperCapRxCallback;
    can_config.id = instance;
    instance->can_ins = CANRegister(&can_config);
    if (instance->can_ins == NULL)
        return false;

    CANSetDLC(instance->can_ins, SUPERCAP_CAN_DLC);
    if (config->input_power_feedback_enabled)
    {
        can_config.rx_id = SUPERCAP_INPUT_POWER_CAN_ID;
        can_config.can_module_callback = SuperCapInputRxCallback;
        instance->input_can_ins = CANRegister(&can_config);
        if (instance->input_can_ins == NULL)
            return false;
    }
    instance->snapshot.input_feedback_enabled = config->input_power_feedback_enabled;
    instance->active = true;
    cap = instance;
    return true;
}

bool SuperCapSendCommand(const SuperCapCommand_s *command)
{
    if (cap == NULL || cap->can_ins == NULL)
        return false;

    const bool success = SuperCapProtocolEncodeCommand(command, cap->can_ins->tx_buff) &&
                         CANTransmit(cap->can_ins, 1.0f) != 0U;
    if (!success) {
        taskENTER_CRITICAL();
        cap->snapshot.command_fail_count++;
        taskEXIT_CRITICAL();
    }
    return success;
}

bool SuperCapGetSnapshot(uint32_t now_ms, SuperCapSnapshot_s *output)
{
    if (output == NULL) return false;
    memset(output, 0, sizeof(*output));
    if (cap == NULL)
        return false;

    taskENTER_CRITICAL();
    *output = cap->snapshot;
    output->status.online = output->status.online && DaemonIsOnline(cap->daemon) != 0U;
    const uint32_t input_age = now_ms - output->input_power.last_rx_tick_ms;
    /* A CAN IRQ may publish after the caller sampled now_ms but before this lock.
     * Verify such a newer frame against the clock inside the lock. The live age
     * check also prevents an ancient (> half-range) timestamp appearing fresh. */
    const bool received_after_now = input_age > INT32_MAX &&
        (uint32_t)(HAL_GetTick() - output->input_power.last_rx_tick_ms) <= 20U;
    output->input_power.online = output->input_feedback_enabled && output->input_power.online &&
        (input_age <= 20U || received_after_now);
    output->snapshot_tick_ms = now_ms;
    taskEXIT_CRITICAL();
    return true;
}
