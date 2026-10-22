#ifndef TEST_TIM_H
#define TEST_TIM_H
#include <stdint.h>
typedef struct {uint32_t ARR,CCR;} TIM_TypeDef;
typedef struct {uint32_t Prescaler;} TIM_InitTypeDef;
typedef struct {TIM_TypeDef *Instance;TIM_InitTypeDef Init;int Channel;} TIM_HandleTypeDef;
extern TIM_TypeDef timer_regs;
#define APB1PERIPH_BASE ((uintptr_t)&timer_regs)
#define APB2PERIPH_BASE (APB1PERIPH_BASE+0x10000U)
#define TIM_CHANNEL_2 4U
#define __HAL_TIM_SetAutoreload(h,v) ((h)->Instance->ARR=(uint32_t)(v))
#define __HAL_TIM_SetCompare(h,c,v) ((void)(c),(h)->Instance->CCR=(uint32_t)(v))
#define HAL_OK 0
int HAL_TIM_PWM_Start(TIM_HandleTypeDef *h,uint32_t channel);
void HAL_TIM_PWM_Stop(TIM_HandleTypeDef *h,uint32_t channel);
void HAL_TIM_PWM_Start_DMA(TIM_HandleTypeDef *h,uint32_t channel,uint32_t *data,uint32_t size);
#endif
