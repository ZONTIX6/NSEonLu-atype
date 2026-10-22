#pragma once
#include <stdint.h>
float DWT_GetDeltaT(uint32_t *count);
float DWT_GetTimeline_s(void);
void DWT_Delay(float seconds);
uint32_t HAL_GetTick(void);
