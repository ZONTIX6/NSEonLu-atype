#ifndef IMU_TEMPERATURE_H
#define IMU_TEMPERATURE_H

#include "bsp_pwm.h"
#include <stdbool.h>
bool IMU_Temperature_Init(const PWM_Init_Config_s *config);
/* Fractional duty 0..1. Invalid or failed samples must request zero. */
void IMU_Temperature_Update(float duty_ratio);
#endif
