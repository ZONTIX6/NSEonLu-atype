#ifndef TUNING_FLASH_H
#define TUNING_FLASH_H
#include <stdbool.h>
#include <stdint.h>
typedef bool (*TuningFlashSaveGuard)(void *context,uint32_t now_ms);
/* Register once after Init, before task startup; context must outlive polling.
 * Called with scheduling suspended immediately before writing. The guard must
 * only inspect fresh state: no blocking, allocation, or communication. */
bool TuningFlashRegisterSaveGuard(TuningFlashSaveGuard guard,void *context);
void TuningFlashInit(void);
/* Telemetry task only; saves pending values only with fresh stopped controls. */
void TuningFlashPoll(void);
#endif
