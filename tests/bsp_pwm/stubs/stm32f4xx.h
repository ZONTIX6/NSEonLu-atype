#ifndef TEST_RCC_H
#define TEST_RCC_H
#include "tim.h"
typedef struct {uint32_t CFGR;} RCC_TypeDef;
extern RCC_TypeDef fake_rcc;
#define RCC (&fake_rcc)
#define RCC_CFGR_PPRE1 7U
#define RCC_CFGR_PPRE1_Pos 0U
#define RCC_CFGR_PPRE2 56U
#define RCC_CFGR_PPRE2_Pos 3U
extern const uint8_t APBPrescTable[8];
uint32_t HAL_RCC_GetPCLK1Freq(void);
uint32_t HAL_RCC_GetPCLK2Freq(void);
#endif
