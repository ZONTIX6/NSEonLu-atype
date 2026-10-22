#ifndef TEST_OLED_GPIO_H
#define TEST_OLED_GPIO_H
#include "main.h"
#define GPIO_EXTI_MODE_NONE 0
typedef struct {GPIO_TypeDef *GPIOx;uint16_t GPIO_Pin;} GPIOInstance;
typedef struct {GPIO_TypeDef *GPIOx;uint16_t GPIO_Pin;unsigned exti_mode;} GPIO_Init_Config_s;
GPIOInstance *GPIORegister(GPIO_Init_Config_s *config);
void GPIOSet(GPIOInstance *instance);
void GPIOReset(GPIOInstance *instance);
#endif
