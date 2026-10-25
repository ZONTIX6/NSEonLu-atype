#ifndef VEHICLE_SAFETY_H
#define VEHICLE_SAFETY_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool input_online;
    bool emergency;
    bool reset; /* Explicit neutral/right-up recovery position. */
    bool bench; /* Dedicated bench firmware and neutral controls. */
    bool aux_stopped; /* Fresh remote actuator stop proof in a dual-board vehicle. */
} VehicleSafetyInput_s;

void VehicleSafetyPublish(const VehicleSafetyInput_s *input, uint32_t now_ms);
bool VehicleSafetyAllowsDrive(uint32_t now_ms);
bool VehicleSafetyAllowsAutotune(uint32_t now_ms);
bool VehicleSafetyIsStopped(uint32_t now_ms);
bool VehicleSafetyAuxStopped(uint32_t now_ms);
uint32_t VehicleSafetyEpoch(void);
#endif
