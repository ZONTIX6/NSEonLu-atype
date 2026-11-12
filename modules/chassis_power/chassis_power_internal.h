#ifndef CHASSIS_POWER_INTERNAL_H
#define CHASSIS_POWER_INTERNAL_H

#include "chassis_power.h"
#include <string.h>
#include <math.h>
#ifdef STM32F427xx
#include "FreeRTOS.h"
#include "task.h"
#define POWER_LOCK() taskENTER_CRITICAL()
#define POWER_UNLOCK() taskEXIT_CRITICAL()
#else
#define POWER_LOCK() ((void)0)
#define POWER_UNLOCK() ((void)0)
#endif

typedef struct
{
    ChassisPower_Init_Config_s config, slow_config, pending_config;
    ChassisPowerConfigObservation_s config_observation, slow_observation, pending_observation;
    uint32_t config_version, slow_version, pending_version;
    bool config_pending;
    ChassisPowerInput_s input;
    ChassisPowerSnapshot_s state, working_state;
    ChassisPowerControlSnapshot_s control;
    ChassisPowerTransientState_s transient;
    ChassisPowerBudgetState_s budget_state;
    ChassisPowerOuterLoopState_s outer_state;
    ChassisPowerPolicyOutput_s policy;
    uint32_t policy_tick_ms, policy_version;
    uint8_t telemetry_count;
    SuperCapCommand_s cap_sent_command;
    uint32_t cap_sent_ms, cap_anchor_frames, cap_confirmed_tick_ms;
    uint32_t cap_send_failures, cap_confirmation_timeouts;
    bool cap_command_ok, cap_confirmed, cap_timeout_reported;
} ChassisPowerInstance_s;

ChassisPowerInstance_s *ChassisPowerPrivate(void);
#define chassis_power (ChassisPowerPrivate())
bool ChassisPowerConfigValid(const ChassisPower_Init_Config_s *config);
void ChassisPowerTelemetryFinal(ChassisPowerInstance_s *instance);

#endif
