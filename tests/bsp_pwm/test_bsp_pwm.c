#include "bsp_pwm.h"
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
TIM_TypeDef timer_regs;
RCC_TypeDef fake_rcc;
const uint8_t APBPrescTable[8]={0,0,0,0,1,2,3,4};
static bool start_fails;
uint32_t HAL_RCC_GetPCLK1Freq(void) {return 45000000;}
uint32_t HAL_RCC_GetPCLK2Freq(void) {return 90000000;}
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *h,uint32_t channel) {(void)h;(void)channel;return start_fails ? 1 : HAL_OK;}
void HAL_TIM_PWM_Stop(TIM_HandleTypeDef *h,uint32_t channel) {(void)h;(void)channel;}
void HAL_TIM_PWM_Start_DMA(TIM_HandleTypeDef *h,uint32_t channel,uint32_t *data,uint32_t size)
{(void)h;(void)channel;(void)data;(void)size;}
int main(void)
{
    fake_rcc.CFGR=4;
    TIM_HandleTypeDef timer={.Instance=&timer_regs,.Init={.Prescaler=89}};
    PWM_Init_Config_s config={.htim=&timer,.channel=TIM_CHANNEL_2,.period=.0001f,.dutyratio=.5f};
    assert(PWMRegister(NULL)==NULL);
    config.period=NAN;assert(PWMRegister(&config)==NULL);config.period=.0001f;
    start_fails=true;assert(PWMRegister(&config)==NULL);start_fails=false;
    PWMInstance *pwm=PWMRegister(&config);assert(pwm);
    assert(timer_regs.ARR==99 && timer_regs.CCR==50);
    PWMSetDutyRatio(pwm,1);assert(timer_regs.CCR==100);
    PWMSetDutyRatio(pwm,NAN);assert(timer_regs.CCR==0);
    puts("PASS: 100 us PWM counter period, fractional duty and failed startup");
}
