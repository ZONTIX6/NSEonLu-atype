#ifndef CHASSIS_POWER_POLICY_H
#define CHASSIS_POWER_POLICY_H

#include "referee_power.h"
#include "super_cap_protocol.h"

typedef struct
{
    RefereePowerSnapshot_s referee;
    bool bench_enabled;
    float bench_limit_w;
    bool chassis_zero_force;
} ChassisPowerCommandInput_s;

typedef struct
{
    SuperCapCommand_s command;
    float base_power_limit_w;
} ChassisPowerCommandOutput_s;

bool ChassisPowerBuildCommand(const ChassisPowerCommandInput_s *input,
                              ChassisPowerCommandOutput_s *output);
#endif
