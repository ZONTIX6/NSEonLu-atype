#ifndef TUNING_FLASH_H
#define TUNING_FLASH_H
#include <stdbool.h>
#include <stdint.h>
typedef bool (*TuningFlashSaveGuard)(void *context,uint32_t now_ms);
bool TuningFlashRegisterSaveGuard(TuningFlashSaveGuard guard,void *context);
void TuningFlashInit(void);
/* Telemetry task only; saves pending values only with fresh stopped controls. */
void TuningFlashPoll(void);
#endif
