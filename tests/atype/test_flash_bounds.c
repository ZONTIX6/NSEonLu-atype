#include "bsp_flash.h"
#include <assert.h>
#include <stdio.h>

static unsigned erases, writes;
static uint32_t last_sector, last_address;
HAL_StatusTypeDef HAL_FLASH_Unlock(void) {return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Lock(void) {return HAL_OK;}
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *e,uint32_t *error)
{++erases;last_sector=e->Sector;*error=0xffffffffU;return HAL_OK;}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type,uint32_t address,uint64_t value)
{(void)type;(void)value;++writes;last_address=address;return HAL_OK;}

int main(void)
{
    uint32_t data[2]={1,2};
    flash_erase_address(0x081c0000,1);
    assert(erases==1 && last_sector==22);
    flash_erase_address(0x081e0000,2);
    flash_erase_address(0x08200000,1);
    flash_erase_address(0,1);
    assert(erases==1);
    assert(get_next_flash_address(0x081ffffc)==0x08200000);
    assert(get_next_flash_address(0x08200000)==0);
    assert(flash_write_single_address(0x081dfffc,data,2)==-1 && writes==0);
    assert(flash_write_single_address(0x081dfffc,data,1)==0 && writes==1);
    assert(last_address==0x081dfffc);
    assert(flash_write_muli_address(0x081ffffc,0x08200000,data,2)==-1 && writes==1);
    assert(flash_write_muli_address(0x081ffffc,0x08200000,data,1)==0 && writes==2);
    assert(flash_write_muli_address(0x081e0001,0x08200000,data,1)==-1);
    assert(flash_write_single_address(0x081e0000,data,0)==-1);
    puts("PASS: F427 flash sectors, exclusive end and overflow boundaries");
}
