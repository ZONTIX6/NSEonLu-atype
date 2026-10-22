#ifndef TEST_OLED_IIC_H
#define TEST_OLED_IIC_H
#include "main.h"
#include <stdbool.h>
#define IIC_BLOCK_MODE 0
typedef struct {I2C_HandleTypeDef *handle;uint8_t dev_address;} IICInstance;
typedef struct {I2C_HandleTypeDef *handle;uint8_t dev_address;unsigned work_mode;} IIC_Init_Config_s;
IICInstance *IICRegister(IIC_Init_Config_s *config);
bool IICTransmitChecked(IICInstance *,const uint8_t *,uint16_t,uint32_t);
#endif
