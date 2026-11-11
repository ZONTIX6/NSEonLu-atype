#ifndef PID_CONFIG_H
#define PID_CONFIG_H

#include "controller.h"

/* PID 默认参数统一在 pid_config.c 调整；运行时调参只修改实例副本。 */
extern const PID_Init_Config_s pid_chassis_speed_config; /* 底盘四轮共用的轮速 PID；默认值见 pid_config.c，运行时可能被保存参数覆盖。 */
extern const PID_Init_Config_s pid_gimbal_yaw_angle_config; /* 云台 Yaw 角度环 PID，输出交给 Yaw 速度环。 */
extern const PID_Init_Config_s pid_gimbal_yaw_speed_config; /* 云台 Yaw 速度环 PID，输出为 DJI 电流命令刻度。 */
extern const PID_Init_Config_s pid_gimbal_pitch_angle_config; /* 云台 Pitch 角度环 PID，输出交给 Pitch 速度环。 */
extern const PID_Init_Config_s pid_gimbal_pitch_speed_config; /* 云台 Pitch 速度环 PID，输出为 DJI 电流命令刻度。 */
extern const PID_Init_Config_s pid_shoot_friction_speed_config; /* 左右摩擦轮共用速度环 PID，输出交给电流环。 */
extern const PID_Init_Config_s pid_shoot_friction_current_config; /* 左右摩擦轮共用电流环 PID，反馈为 DJI 原始刻度。 */
extern const PID_Init_Config_s pid_shoot_loader_angle_config; /* 拨弹盘角度环 PID，误差使用电机转子角度。 */
extern const PID_Init_Config_s pid_shoot_loader_speed_config; /* 拨弹盘速度环 PID，反馈单位为转子度/秒。 */
extern const PID_Init_Config_s pid_shoot_loader_current_config; /* 拨弹盘电流环 PID，反馈为 DJI 原始刻度。 */
extern const PID_Init_Config_s pid_imu_mpu6500_temperature_config; /* 当前 A 板 MPU6500 温控 PID，温差为 ℃、输出为占空比百分数。 */
extern const PID_Init_Config_s pid_imu_bmi088_temperature_config; /* 旧 BMI088 温控 PID，温差为 ℃、输出为定时器计数，不与 MPU6500 共用。 */

#endif
