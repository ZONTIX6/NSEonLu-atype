#ifndef BOARD_CHASSIS_FORCE_H
#define BOARD_CHASSIS_FORCE_H
#include "robot_def.h"
#include "chassis_control_config.h"
#include "chassis_energy_config.h"
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
#include "chassis_force_config.h"
#include "chassis_power.h"

/* Board entry points; tune physical/energy defaults in config/22_chassis_force/chassis_force_vehicle_config.c. */
ChassisForceConfig_s BoardChassisForceConfig(void);
ChassisPower_Init_Config_s BoardChassisForcePowerConfig(void);
#endif
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
#if CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_OMNI_X
#error Direct force control requires OMNI_X
#endif
#if CHASSIS_POWER_CONTROL_STAGE != 3
#error Direct force control requires power stage 3
#endif
#if !CHASSIS_POWER_CONTROL_ENABLE
#error Direct force control requires CHASSIS_POWER_CONTROL_ENABLE=1
#endif
#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE && !POWER_METER_ENABLE
#error Adaptive force spin requires load power feedback
#endif
#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP && !CHASSIS_BUFFER_POWER_ENABLE
#error Force supercap control requires the verified input-power budget path
#endif
#endif
#endif
