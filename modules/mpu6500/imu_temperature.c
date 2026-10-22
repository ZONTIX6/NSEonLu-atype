#include  "imu_temperature.h"
#include "bsp_pwm.h"
#include <math.h>
#include <stddef.h>
static PWMInstance *imu_pwm;
bool IMU_Temperature_Init(const PWM_Init_Config_s *config)
{
    if(config==NULL || imu_pwm!=NULL || config->htim==NULL ||
        !isfinite(config->period) || config->period<=0 ||
        !isfinite(config->dutyratio) || config->dutyratio<0 || config->dutyratio>1) return false;
    PWM_Init_Config_s owned=*config;
    imu_pwm = PWMRegister(&owned);
    return imu_pwm!=NULL;
}


void IMU_Temperature_Update(float duty_ratio)
{
    if(imu_pwm!=NULL) PWMSetDutyRatio(imu_pwm, isfinite(duty_ratio) ? fminf(1,fmaxf(0,duty_ratio)) : 0);
}
