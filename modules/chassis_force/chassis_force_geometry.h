#ifndef CHASSIS_FORCE_GEOMETRY_H
#define CHASSIS_FORCE_GEOMETRY_H
#include "chassis_force_config.h"
typedef struct {
    float h[4][3], plus[3][4], current_map[4][3], wrench_map[3][4];
} ChassisForceGeometry_s;
bool ChassisForceGeometryInit(const ChassisForceConfig_s *config,ChassisForceGeometry_s *geometry);
void ChassisForceToCurrent(const ChassisForceGeometry_s *g,const float wrench[3],float current[4]);
void ChassisForceFromCurrent(const ChassisForceGeometry_s *g,const float current[4],float wrench[3]);
void ChassisForceEstimate(const ChassisForceGeometry_s *g,const ChassisForceConfig_s *c,const float rpm[4],float velocity[3]);
#endif
