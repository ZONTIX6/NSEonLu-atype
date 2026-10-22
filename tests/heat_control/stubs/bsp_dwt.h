#ifndef HEAT_CONTROL_TEST_DWT_H
#define HEAT_CONTROL_TEST_DWT_H

#include <stdint.h>

float DWT_GetDeltaT(uint32_t *cnt_last);
uint32_t HAL_GetTick(void);

#endif
