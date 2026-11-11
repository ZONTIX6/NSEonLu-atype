#include "board_chassis_force.h"
#include "../../config/22_chassis_force/chassis_force_vehicle_config.h"

ChassisForceConfig_s BoardChassisForceConfig(void)
{
    return ChassisForceVehicleConfig();
}

ChassisPower_Init_Config_s BoardChassisForcePowerConfig(void)
{
    return ChassisForceVehiclePowerConfig();
}
