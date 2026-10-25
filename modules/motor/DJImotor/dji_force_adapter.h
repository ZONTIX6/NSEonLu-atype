#ifndef DJI_FORCE_ADAPTER_H
#define DJI_FORCE_ADAPTER_H
#include "dji_motor.h"
#include "chassis_power_force.h"
typedef struct {
    ChassisForceIntent_s intent;
    ChassisPowerControlSnapshot_s power;
    uint32_t command_received_ms,config_version,safety_epoch,transport_margin_ms;
    uint32_t command_faults;
    bool growth_allowed,offer_allowed;
} ChassisForceActuationSnapshot_s;
bool DJIForceAdapterInit(DJIMotorInstance *const motors[4],const ChassisForceConfig_s *config);
bool DJIForceAdapterPublish(const ChassisForceActuationSnapshot_s *snapshot);
bool DJIForceAdapterGetSnapshot(ChassisForceCycleOutput_s *snapshot);
#endif
