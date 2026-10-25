#ifndef CHASSIS_POWER_FORCE_H
#define CHASSIS_POWER_FORCE_H
#include "chassis_power.h"
#include "chassis_force_control.h"
/* Freeze once per motor cycle, without target speeds or a speed-based allocator. */
bool ChassisPowerForceBegin(const ChassisPowerControlSnapshot_s *control,const PowerMeterStatus_s *meter,
    uint32_t now_ms,float dt_s,ChassisForceCycleInput_s *cycle);
bool ChassisPowerForceRecord(const ChassisForceCycleInput_s *input,const ChassisForceCycleOutput_s *output,
    const uint32_t feedback_sequence[4]);
#endif
