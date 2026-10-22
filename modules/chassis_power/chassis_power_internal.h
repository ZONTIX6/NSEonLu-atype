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
} ChassisPowerInstance_s;

ChassisPowerInstance_s *ChassisPowerPrivate(void);
#define chassis_power (ChassisPowerPrivate())
bool ChassisPowerConfigValid(const ChassisPower_Init_Config_s *config);
void ChassisPowerTelemetryFinal(ChassisPowerInstance_s *instance);

#endif
