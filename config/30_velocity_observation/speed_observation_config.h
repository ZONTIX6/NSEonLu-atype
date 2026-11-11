#ifndef SPEED_OBSERVATION_CONFIG_H
#define SPEED_OBSERVATION_CONFIG_H
#include "../22_chassis_force/chassis_control_config.h"
#include "../22_chassis_force/force_calibration_config.h"
#include "predictive_power_config.h"
/* 三个开关仅允许 0/1，默认均为 0。推荐在构建命令中切换，例如：
 * make ... CHASSIS_SPEED_CAPTURE_ENABLE=1 LOG_OUTPUT=2 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0
 * CMake 对同名选项添加 -D 前缀；开启 KF 还需 CHASSIS_SPEED_KF_SHADOW_ENABLE=1 CHASSIS_PREDICTIVE_POWER_MODE=1。
 * 编译命令优先于本文件默认值；切换组合使用独立构建目录并重新烧录。 */
#ifndef CHASSIS_SPEED_CAPTURE_ENABLE
#define CHASSIS_SPEED_CAPTURE_ENABLE 0 /* 0 关闭、1 启用轮速采集缓存与导出；默认关闭，导出要求 LOG_OUTPUT=2 或 3（USART2）。 */
#endif
#ifndef CHASSIS_SPEED_KF_SHADOW_ENABLE
#define CHASSIS_SPEED_KF_SHADOW_ENABLE 0 /* 0 关闭、1 启用轮速卡尔曼旁路观察；默认关闭，必须同时选择预测功率 SHADOW 模式。 */
#endif
#ifndef CHASSIS_SPEED_KF_EXPERIMENT_CONFIG
#define CHASSIS_SPEED_KF_EXPERIMENT_CONFIG 0 /* 0 使用默认 KF 参数；1 从 chassis_speed_kf_experiment.h 加载实验参数；默认 0，必须同时启用 KF_SHADOW。 */
#endif
#if (CHASSIS_SPEED_CAPTURE_ENABLE != 0 && CHASSIS_SPEED_CAPTURE_ENABLE != 1) || \
    (CHASSIS_SPEED_KF_SHADOW_ENABLE != 0 && CHASSIS_SPEED_KF_SHADOW_ENABLE != 1) || \
    (CHASSIS_SPEED_KF_EXPERIMENT_CONFIG != 0 && CHASSIS_SPEED_KF_EXPERIMENT_CONFIG != 1)
#error Speed observation options must be 0 or 1
#endif
/* 采集或 KF 观察只适用于 CHASSIS_CONTROL_MODE=CHASSIS_CONTROL_SPEED，
 * 且 CHASSIS_FORCE_CALIBRATION_ENABLE 必须为 0；两类观察可以同时开启。 */
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED || CHASSIS_FORCE_CALIBRATION_ENABLE
#error Speed observation requires the speed backend with force calibration disabled
#endif
#endif
#if CHASSIS_SPEED_CAPTURE_ENABLE && (!defined(LOG_OUTPUT) || (LOG_OUTPUT != 2 && LOG_OUTPUT != 3))
#error Speed capture requires LOG_OUTPUT=2 or 3 for USART2 export
#endif
#if CHASSIS_SPEED_KF_SHADOW_ENABLE && CHASSIS_PREDICTIVE_POWER_MODE != CHASSIS_PREDICTIVE_SHADOW
#error Wheel KF requires predictive shadow mode
#endif
/* 实验参数头须存在于编译 include 路径；其校验和由初始化逻辑检查。 */
#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG && !CHASSIS_SPEED_KF_SHADOW_ENABLE
#error Experimental KF configuration requires the wheel KF shadow
#endif
#endif
