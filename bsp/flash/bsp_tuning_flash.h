#ifndef BSP_TUNING_FLASH_H
#define BSP_TUNING_FLASH_H
#include <stdbool.h>
#include <stdint.h>
const volatile uint32_t *BSPTuningFlashBank(unsigned bank);
unsigned BSPTuningFlashBankWords(void);
bool BSPTuningFlashProgram(void *context,unsigned bank,unsigned word,uint32_t value);
bool BSPTuningFlashErase(void *context,unsigned bank);
bool BSPTuningFlashBegin(void);
bool BSPTuningFlashEnd(void);
#endif
