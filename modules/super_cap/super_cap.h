#ifndef SUPER_CAP_H
#define SUPER_CAP_H

#include "bsp_can.h"
#include "daemon.h"
#include "super_cap_types.h"

typedef struct
{
    CAN_Init_Config_s can_config;
    uint16_t offline_reload_count;
    bool input_power_feedback_enabled;
} SuperCap_Init_Config_s;

bool SuperCapInit(const SuperCap_Init_Config_s *config);
bool SuperCapSendCommand(const SuperCapCommand_s *command);
bool SuperCapGetSnapshot(uint32_t now_ms, SuperCapSnapshot_s *output);

#endif
