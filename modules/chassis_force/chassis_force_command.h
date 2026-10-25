#ifndef CHASSIS_FORCE_COMMAND_H
#define CHASSIS_FORCE_COMMAND_H
#include "chassis_force_config.h"
#include "chassis_force_types.h"
typedef struct {
    float force_g[2], torque_nm;
    uint8_t previous_mode;
    int8_t brake_sign;
    uint32_t brake_start_ms;
    bool braking,brake_timed_out;
} ChassisForceCommandState_s;
typedef struct {float high[3],low[3],force_g[2];bool braking,brake_timed_out;} ChassisForceCommandOutput_s;
bool ChassisForceCommand(const ChassisForceConfig_s *config,const ChassisForceIntent_s *intent,
    float omega_rad_s,uint32_t now_ms,float dt_s,ChassisForceCommandState_s *state,ChassisForceCommandOutput_s *output);
#endif
