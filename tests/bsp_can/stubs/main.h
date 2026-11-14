#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include "can.h"
void Error_Handler(void);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __enable_irq(void);
void __set_PRIMASK(uint32_t mask);
#endif
