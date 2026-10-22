#include "bsp_tuning_flash.h"
#include "main.h"
#include <stddef.h>

#define BANK_WORDS (128U*1024U/sizeof(uint32_t))
/* Linker-reserved sectors 22/23: 0x081C0000 and 0x081E0000. */
extern uint32_t __tuning_bank0_start__,__tuning_bank1_start__;
const volatile uint32_t *BSPTuningFlashBank(unsigned bank)
{return bank>1 ? NULL : bank ? &__tuning_bank1_start__ : &__tuning_bank0_start__;}
unsigned BSPTuningFlashBankWords(void) {return BANK_WORDS;}
static void FlushDataCache(void)
{
    if(FLASH->ACR & FLASH_ACR_DCEN) {
        __HAL_FLASH_DATA_CACHE_DISABLE();__HAL_FLASH_DATA_CACHE_RESET();__HAL_FLASH_DATA_CACHE_ENABLE();
    }
    __DSB();__ISB();
}
bool BSPTuningFlashProgram(void *context,unsigned bank,unsigned word,uint32_t value)
{
    (void)context;
    if(bank>1 || word>=BANK_WORDS) return false;
    uint32_t address=(uint32_t)(uintptr_t)BSPTuningFlashBank(bank)+word*sizeof(uint32_t);
    HAL_StatusTypeDef result=HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,address,value);
    FlushDataCache();return result==HAL_OK;
}
bool BSPTuningFlashErase(void *context,unsigned bank)
{
    (void)context;
    if(bank>1) return false;
    FLASH_EraseInitTypeDef erase={.TypeErase=FLASH_TYPEERASE_SECTORS,
        .Sector=bank ? FLASH_SECTOR_23 : FLASH_SECTOR_22,
        .NbSectors=1,.VoltageRange=FLASH_VOLTAGE_RANGE_3};
    uint32_t error=UINT32_MAX;
    HAL_StatusTypeDef result=HAL_FLASHEx_Erase(&erase,&error);
    FlushDataCache();return result==HAL_OK && error==UINT32_MAX;
}
bool BSPTuningFlashBegin(void)
{
    if(HAL_FLASH_Unlock()!=HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR|
        FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
    return true;
}
bool BSPTuningFlashEnd(void) {return HAL_FLASH_Lock()==HAL_OK;}
