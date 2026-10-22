#ifndef TEST_OLED_SPI_H
#define TEST_OLED_SPI_H
#include "main.h"
typedef enum {SPI_CS_GPIO,SPI_CS_NONE} SPI_CS_Mode_e;
typedef enum {BSP_SPI_OK,BSP_SPI_BUSY,BSP_SPI_TIMEOUT,BSP_SPI_ERROR,BSP_SPI_INVALID} BSP_SPI_Result_e;
#define SPI_BLOCK_MODE 0
typedef struct {unsigned marker;} SPIInstance;
typedef struct {SPI_HandleTypeDef *spi_handle;GPIO_TypeDef *GPIOx;uint16_t cs_pin;SPI_CS_Mode_e cs_mode;unsigned spi_work_mode;} SPI_Init_Config_s;
SPIInstance *SPIRegister(SPI_Init_Config_s *config);
BSP_SPI_Result_e SPITransferChecked(SPIInstance *,const uint8_t *,uint8_t *,uint16_t,uint32_t);
#endif
