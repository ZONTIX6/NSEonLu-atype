#include "bsp_pwm.h"
#include "imu_temperature.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
TIM_HandleTypeDef htim3;
static PWMInstance pwm;
PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    assert(config->htim==&htim3 && config->channel==TIM_CHANNEL_2);
    assert(fabsf(config->period-0.0001f)<0.0000001f);
    assert(config->dutyratio==0.5f);
    return &pwm;
}
void PWMSetDutyRatio(PWMInstance *instance,float ratio)
{assert(instance==&pwm);pwm.duty=ratio;}
int main(void)
{
    PWM_Init_Config_s config={.htim=&htim3,.channel=TIM_CHANNEL_2,.period=.0001f,.dutyratio=.5f};
    assert(!IMU_Temperature_Init(NULL));
    assert(IMU_Temperature_Init(&config));
    assert(!IMU_Temperature_Init(&config));
    IMU_Temperature_Update(.6f);
    assert(fabsf(pwm.duty-0.6f)<0.0001f);
    IMU_Temperature_Update(0);assert(pwm.duty==0);
    IMU_Temperature_Update(1);assert(pwm.duty==1);
    IMU_Temperature_Update(NAN);assert(pwm.duty==0);
    puts("PASS: heater PWM uses 100 us period and fractional duty");
}
