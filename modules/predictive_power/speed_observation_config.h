#ifndef SPEED_OBSERVATION_CONFIG_H
#define SPEED_OBSERVATION_CONFIG_H
#include "../algorithm/chassis_control_config.h"
#include "../chassis_force_calibration/force_calibration_config.h"
#include "predictive_power_config.h"
#ifndef CHASSIS_SPEED_CAPTURE_ENABLE
#define CHASSIS_SPEED_CAPTURE_ENABLE 0
#endif
#ifndef CHASSIS_SPEED_KF_SHADOW_ENABLE
#define CHASSIS_SPEED_KF_SHADOW_ENABLE 0
#endif
#ifndef CHASSIS_SPEED_KF_EXPERIMENT_CONFIG
#define CHASSIS_SPEED_KF_EXPERIMENT_CONFIG 0
#endif
#if (CHASSIS_SPEED_CAPTURE_ENABLE != 0 && CHASSIS_SPEED_CAPTURE_ENABLE != 1) || \
    (CHASSIS_SPEED_KF_SHADOW_ENABLE != 0 && CHASSIS_SPEED_KF_SHADOW_ENABLE != 1) || \
    (CHASSIS_SPEED_KF_EXPERIMENT_CONFIG != 0 && CHASSIS_SPEED_KF_EXPERIMENT_CONFIG != 1)
#error Speed observation options must be 0 or 1
#endif
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
#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG && !CHASSIS_SPEED_KF_SHADOW_ENABLE
#error Experimental KF configuration requires the wheel KF shadow
#endif
#endif
