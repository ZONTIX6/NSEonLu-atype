#ifndef CHASSIS_POWER_H
#define CHASSIS_POWER_H

#include "chassis_power_types.h"

bool ChassisPowerInit(const ChassisPower_Init_Config_s *config);
/* FNV-1a32 over canonical little-endian fields; excludes provenance and padding. */
uint32_t ChassisPowerConfigFingerprint(const ChassisPower_Init_Config_s *config);
bool ChassisPowerUpdatePolicy(const ChassisPowerPolicyInput_s *input,
                              ChassisPowerPolicyOutput_s *output);
bool ChassisPowerLimit(const ChassisPowerCycleInput_s *input,
                       ChassisPowerCycleOutput_s *output);
bool ChassisPowerRecordFinal(const ChassisPowerFinalInput_s *input);
bool ChassisPowerGetSnapshot(ChassisPowerSnapshot_s *output);
bool ChassisPowerRequestConfig(const ChassisPower_Init_Config_s *config, uint32_t version);
void ChassisPowerCaptureDrain(void);
/* Slow-task-only: record actual submission, never treat it as capacitor execution. */
void ChassisPowerCapCommandResult(const SuperCapCommand_s *command,bool submitted,
    uint32_t now_ms,uint32_t input_frame_count,ChassisPowerPolicyOutput_s *policy);

#define CHASSIS_POWER_RTT_DIVIDER 20U /* 40 ms at a 2 ms motor period. */

#endif
