#ifndef CHASSIS_FORCE_TELEMETRY_H
#define CHASSIS_FORCE_TELEMETRY_H
#include "chassis_force_control.h"
#include "chassis_power.h"
#include <stddef.h>
size_t ChassisForceTelemetryHeader(char *buffer,size_t capacity);
size_t ChassisForceTelemetryRow(const ChassisForceCycleOutput_s *force,const ChassisPowerSnapshot_s *power,
    const SuperCapSnapshot_s *cap,const RefereePowerSnapshot_s *referee,char *buffer,size_t capacity);
#endif
