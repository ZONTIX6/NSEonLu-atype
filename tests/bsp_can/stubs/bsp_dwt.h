#ifndef TEST_BSP_DWT_H
#define TEST_BSP_DWT_H
#include <stdint.h>
uint32_t DWT_GetCycleCount(void);
uint32_t DWT_GetFrequencyHz(void);
float DWT_GetTimeline_ms(void);
#endif
