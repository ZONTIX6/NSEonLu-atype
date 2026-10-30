#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H
#include <stdint.h>
static inline uint32_t __get_PRIMASK(void) {return 0;}
static inline void __disable_irq(void) {}
static inline void __set_PRIMASK(uint32_t mask) {(void)mask;}
#endif
