#ifndef CHASSIS_FORCE_ALLOCATOR_H
#define CHASSIS_FORCE_ALLOCATOR_H
#include "power_model.h"
typedef struct {
    float high[4], low[4], min_current[4], max_current[4];
    MotorPowerModelConfig_s models[4];
    float rpm_bound[4], budget_w;
    /* dot(linear[k],current)<=bound[k]; body kinetic-energy constraints. */
    float linear[2][4], bound[2];
    unsigned linear_count;
} ChassisForceAllocationInput_s;
typedef struct {
    float alpha, beta, current[4], predicted_w;
    bool valid, infeasible;
} ChassisForceAllocationOutput_s;
float ChassisForcePredict(const ChassisForceAllocationInput_s *input,const float current[4]);
float ChassisForcePredictWheel(const MotorPowerModelConfig_s *model,float current,float rpm_bound);
bool ChassisForceCandidateValid(const ChassisForceAllocationInput_s *input,const float current[4]);
bool ChassisForceAllocate(const ChassisForceAllocationInput_s *input,ChassisForceAllocationOutput_s *output);
#endif
