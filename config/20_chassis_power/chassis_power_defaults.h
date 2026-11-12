#ifndef CHASSIS_POWER_DEFAULTS_H
#define CHASSIS_POWER_DEFAULTS_H
#include "../../modules/chassis_power/chassis_power_types.h"
#include "chassis_power_config.h"
#include "chassis_energy_config.h"
#include "chassis_power_model_config.h"
extern ChassisPowerBudgetConfig_s chassis_budget_config;
extern ChassisPowerOuterLoopConfig_s chassis_outer_loop_config;
/* Uses the owning task's budget/outer values, including any restored tuning. */
static inline ChassisPower_Init_Config_s ChassisPowerSpeedDefaultConfig(void)
{
    ChassisPower_Init_Config_s config={
        .budget=chassis_budget_config,.outer=chassis_outer_loop_config,
        .transient=ChassisPowerTransientDefaultConfig(),.stage=CHASSIS_POWER_CONTROL_STAGE,
        .use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP,
        .meter_enabled=POWER_METER_ENABLE!=0,.buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0,
        .algorithm={.safety_factor=CHASSIS_POWER_SAFETY_FACTOR,
            .small_error_threshold_rpm=CHASSIS_POWER_SMALL_ERROR_RPM,
            .reserved_power_threshold_w=CHASSIS_POWER_RESERVED_THRESHOLD_W,
            .per_motor_reserved_power_w=CHASSIS_POWER_PER_MOTOR_RESERVED_W,
            .max_current_command=CHASSIS_POWER_MAX_CURRENT_COMMAND}};
    const MotorPowerModelConfig_s model=CHASSIS_SPEED_POWER_MODEL_INITIALIZER;
    for(unsigned w=0;w<4;++w) config.models[w]=model;
    return config;
}
#endif
