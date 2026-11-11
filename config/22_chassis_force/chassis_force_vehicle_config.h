#ifndef CHASSIS_FORCE_VEHICLE_CONFIG_H
#define CHASSIS_FORCE_VEHICLE_CONFIG_H

#include "../02_chassis/chassis_config.h"
#include "../20_chassis_power/chassis_energy_config.h"
#include "chassis_force_config.h"
#include "chassis_power.h"

/* 车辆直接力控的物理标定与操作限幅；具体数值、单位和验收方法在同名 .c 文件。
 * 修改配置后重新构建；未完成物理标定时仍保持校验失败和输出锁定。 */
ChassisForceConfig_s ChassisForceVehicleConfig(void);
/* 生成直接力控使用的功率、能量、瞬态保护和静态模型初值。 */
ChassisPower_Init_Config_s ChassisForceVehiclePowerConfig(void);

#endif
