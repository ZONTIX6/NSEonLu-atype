#ifndef VOFA_OBSERVATION_H
#define VOFA_OBSERVATION_H

#include "vofa_telemetry.h"
#include "chassis_power_types.h"
#include "chassis_velocity_observer.h"
#include "super_cap_types.h"
#include "referee_power.h"

/* Snapshot pointers may be NULL. Called only by the telemetry task. */
void VofaObservationSend(const USBTelemetrySample_s *sample,
    const ChassisPowerSnapshot_s *power, const ChassisFusedVelocitySnapshot_s *velocity,
    const SuperCapSnapshot_s *cap, const RefereePowerSnapshot_s *referee, uint32_t now_ms);

#endif
