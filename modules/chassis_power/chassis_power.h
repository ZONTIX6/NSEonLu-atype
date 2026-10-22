#ifndef CHASSIS_POWER_H
#define CHASSIS_POWER_H

#include "chassis_power_types.h"

bool ChassisPowerInit(const ChassisPower_Init_Config_s *config);
bool ChassisPowerUpdatePolicy(const ChassisPowerPolicyInput_s *input,
                              ChassisPowerPolicyOutput_s *output);
bool ChassisPowerLimit(const ChassisPowerCycleInput_s *input,
                       ChassisPowerCycleOutput_s *output);
bool ChassisPowerRecordFinal(const ChassisPowerFinalInput_s *input);
bool ChassisPowerGetSnapshot(ChassisPowerSnapshot_s *output);
bool ChassisPowerRequestConfig(const ChassisPower_Init_Config_s *config, uint32_t version);
void ChassisPowerCaptureDrain(void);

#define CHASSIS_POWER_RTT_DIVIDER 20U /* 40 ms at a 2 ms motor period. */

#endif
