#ifndef POWER_METER_H
#define POWER_METER_H

#include "bsp_can.h"
#include "daemon.h"
#include "power_meter_protocol.h"
#include "power_meter_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    CANInstance *can_ins;
    DaemonInstance *daemon;
    PowerMeterStatus_s status;
    uint32_t offline_timeout_ms;
    volatile uint32_t received_frame_count;
    volatile uint32_t rejected_frame_count;
    volatile uint32_t last_rx_tick_ms;
} PowerMeterInstance;

typedef struct
{
    CAN_Init_Config_s can_config;
    uint16_t offline_reload_count;
} PowerMeter_Init_Config_s;

bool PowerMeterInit(const PowerMeter_Init_Config_s *config);
bool PowerMeterGetStatus(PowerMeterStatus_s *status);

#endif
