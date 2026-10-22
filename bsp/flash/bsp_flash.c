#include "bsp_flash.h"
#include <stddef.h>
#include <string.h>

static const uint32_t sector_start[25] = {
    ADDR_FLASH_SECTOR_0, ADDR_FLASH_SECTOR_1, ADDR_FLASH_SECTOR_2, ADDR_FLASH_SECTOR_3,
    ADDR_FLASH_SECTOR_4, ADDR_FLASH_SECTOR_5, ADDR_FLASH_SECTOR_6, ADDR_FLASH_SECTOR_7,
    ADDR_FLASH_SECTOR_8, ADDR_FLASH_SECTOR_9, ADDR_FLASH_SECTOR_10, ADDR_FLASH_SECTOR_11,
    ADDR_FLASH_SECTOR_12, ADDR_FLASH_SECTOR_13, ADDR_FLASH_SECTOR_14, ADDR_FLASH_SECTOR_15,
    ADDR_FLASH_SECTOR_16, ADDR_FLASH_SECTOR_17, ADDR_FLASH_SECTOR_18, ADDR_FLASH_SECTOR_19,
    ADDR_FLASH_SECTOR_20, ADDR_FLASH_SECTOR_21, ADDR_FLASH_SECTOR_22, ADDR_FLASH_SECTOR_23,
    FLASH_END_ADDR
};

static unsigned Sector(uint32_t address)
{
    for (unsigned i=0;i<24;++i)
        if (address>=sector_start[i] && address<sector_start[i+1]) return i;
    return 24;
}

uint32_t get_next_flash_address(uint32_t address)
{
    const unsigned sector=Sector(address);
    return sector<24 ? sector_start[sector+1] : 0;
}

void flash_erase_address(uint32_t address, uint16_t len)
{
    const unsigned sector=Sector(address);
    if (sector>=24 || len==0 || len>24-sector) return;
    FLASH_EraseInitTypeDef erase={.Sector=sector,.TypeErase=FLASH_TYPEERASE_SECTORS,
        .VoltageRange=FLASH_VOLTAGE_RANGE_3,.NbSectors=len};
    uint32_t error;
    if (HAL_FLASH_Unlock()!=HAL_OK) return;
    (void)HAL_FLASHEx_Erase(&erase,&error);
    (void)HAL_FLASH_Lock();
}

/* end_address is exclusive. Validate all words before programming the first. */
int8_t flash_write_muli_address(uint32_t start_address, uint32_t end_address,
                                uint32_t *buf, uint32_t len)
{
    if (buf==NULL || len==0 || (start_address&3U) || (end_address&3U) ||
        start_address<ADDR_FLASH_SECTOR_0 || end_address>FLASH_END_ADDR ||
        start_address>=end_address || len>(end_address-start_address)/4U) return -1;
    if (HAL_FLASH_Unlock()!=HAL_OK) return -1;
    int8_t result=0;
    for (uint32_t i=0;i<len;++i) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,start_address+i*4U,buf[i])!=HAL_OK) {
            result=-1;break;
        }
    }
    if (HAL_FLASH_Lock()!=HAL_OK) result=-1;
    return result;
}

int8_t flash_write_single_address(uint32_t start_address, uint32_t *buf, uint32_t len)
{
    return flash_write_muli_address(start_address,get_next_flash_address(start_address),buf,len);
}

void flash_read(uint32_t address, uint32_t *buf, uint32_t len)
{
    if (buf==NULL || (address&3U) || address<ADDR_FLASH_SECTOR_0 ||
        address>=FLASH_END_ADDR || len>(FLASH_END_ADDR-address)/4U) return;
    memcpy(buf,(const void *)(uintptr_t)address,len*4U);
}
