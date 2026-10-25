#ifndef CHASSIS_FORCE_SPIN_POWER_H
#define CHASSIS_FORCE_SPIN_POWER_H
#include "chassis_force_config.h"
typedef struct {
    bool enabled,growth_allowed,meter_valid,last_submission_ok;
    float target_w,load_w,dt_s;
    uint32_t now_ms,meter_tick_ms;
} ChassisForceSpinPowerInput_s;
typedef struct {
    float torque_nm,filtered_w,target_w;
    uint32_t meter_tick_ms;
    unsigned fresh_samples;
    bool initialized,growth_permitted;
} ChassisForceSpinPowerState_s;
bool ChassisForceSpinPowerUpdate(const ChassisForceConfig_s *config,const ChassisForceSpinPowerInput_s *input,
    ChassisForceSpinPowerState_s *state,float *torque);
void ChassisForceSpinPowerProject(ChassisForceSpinPowerState_s *state,float sent_torque_nm,bool limited);
#endif
