#include "oled_transport.h"
#include <stddef.h>
static SPIInstance *spi;
static IICInstance *iic;
static GPIOInstance *dc,*reset;
static OLED_Init_Config_s configuration;
static bool started,ready,healthy;
static uint32_t failures;

bool OLEDRegister(const OLED_Init_Config_s *config)
{
    if(config==NULL || started || (config->backend!=OLED_BACKEND_SPI && config->backend!=OLED_BACKEND_I2C)) return false;
    if(config->backend==OLED_BACKEND_SPI && (config->spi.spi_handle==NULL || config->dc.GPIOx==NULL ||
        config->dc.GPIO_Pin==0 || config->reset.GPIOx==NULL || config->reset.GPIO_Pin==0)) return false;
    if(config->backend==OLED_BACKEND_I2C && (config->iic.handle==NULL || config->iic.dev_address>0x7F)) return false;
    configuration=*config;started=true;
    if(config->backend==OLED_BACKEND_SPI) {
        SPI_Init_Config_s bus=config->spi;bus.spi_work_mode=SPI_BLOCK_MODE;
        spi=SPIRegister(&bus);
        if(spi==NULL) return false;
        GPIO_Init_Config_s pin=config->dc;
        dc=GPIORegister(&pin);pin=config->reset;reset=GPIORegister(&pin);
        if(dc==NULL || reset==NULL) return false;
    } else {
        IIC_Init_Config_s bus=config->iic;bus.work_mode=IIC_BLOCK_MODE;
        iic=IICRegister(&bus);
        if(iic==NULL) return false;
    }
    ready=healthy=true;return true;
}
bool OLEDTransportBegin(void) {healthy=ready;return ready;}
bool OLEDTransportWrite(uint8_t byte,bool data)
{
    if(!ready || !healthy) return false;
    bool success;
    if(spi!=NULL) {
        if(data) GPIOSet(dc);else GPIOReset(dc);
        success=SPITransferChecked(spi,&byte,NULL,1,10000)==BSP_SPI_OK;
    } else {
        const uint8_t frame[2]={data ? 0x40 : 0,byte};
        success=IICTransmitChecked(iic,frame,2,10);
    }
    if(!success) {healthy=false;++failures;}
    return success;
}
void OLEDTransportReset(bool high)
{
    if(ready && reset!=NULL) {if(high) GPIOSet(reset);else GPIOReset(reset);}
}
bool OLEDTransportHealthy(void) {return ready && healthy;}
uint32_t OLEDTransportFailureCount(void) {return failures;}
OLED_button_e OLEDReadButton(void)
{
    OLED_button_e button=OLED_BUTTON_NONE;
    return ready && configuration.read_button!=NULL &&
        configuration.read_button(configuration.button_context,&button) ? button : OLED_BUTTON_NONE;
}
