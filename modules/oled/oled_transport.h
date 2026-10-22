#ifndef OLED_TRANSPORT_H
#define OLED_TRANSPORT_H
#include "bsp_spi.h"
#include "bsp_iic.h"
#include "bsp_gpio.h"
#include "oled_input.h"
#include <stdbool.h>
typedef enum { OLED_BACKEND_SPI, OLED_BACKEND_I2C } OLEDBackend_e;
typedef struct
{
    OLEDBackend_e backend;
    SPI_Init_Config_s spi;
    IIC_Init_Config_s iic;
    GPIO_Init_Config_s dc,reset;
    bool (*read_button)(void *context,OLED_button_e *button);
    void *button_context;
} OLED_Init_Config_s;
bool OLEDRegister(const OLED_Init_Config_s *config);
bool OLEDTransportBegin(void);
bool OLEDTransportWrite(uint8_t byte,bool data);
void OLEDTransportReset(bool high);
bool OLEDTransportHealthy(void);
uint32_t OLEDTransportFailureCount(void);
OLED_button_e OLEDReadButton(void);
#endif
