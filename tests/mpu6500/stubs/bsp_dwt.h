#include <stdint.h>
void DWT_Delay(float delay_s);
static inline void DWT_Delay_ms(float delay_ms) {DWT_Delay(delay_ms*.001f);}
uint32_t HAL_GetTick(void);
uint32_t DWT_GetCycleCount(void);
