#include "oled.h"
#include "main.h"
#include "oled_transport.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#if OLED_USE_I2C
void OLED_operate_gram(pen_typedef pen);
#else
void OLED_clear(Pen_Typedef pen);
#endif

SPI_HandleTypeDef hspi1;
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c2;
GPIO_TypeDef test_port;
static unsigned command_mode,count;
static unsigned char captured[10000][2];
static bool transfer_fails;
static SPIInstance spi;
static IICInstance iic;
static GPIOInstance pins[2];
static unsigned pin_count;
SPIInstance *SPIRegister(SPI_Init_Config_s *config)
{assert(config->spi_handle==&hspi1 && config->cs_mode==SPI_CS_NONE);return &spi;}
BSP_SPI_Result_e SPITransferChecked(SPIInstance *s,const uint8_t *tx,uint8_t *rx,uint16_t n,uint32_t timeout)
{assert(s==&spi && rx==NULL);if(transfer_fails)return BSP_SPI_ERROR;
    HAL_SPI_Transmit(&hspi1,(uint8_t *)tx,n,timeout);return BSP_SPI_OK;}
IICInstance *IICRegister(IIC_Init_Config_s *config)
{assert(config->handle==&hi2c2 && config->dev_address==0x3C);iic.handle=config->handle;iic.dev_address=config->dev_address<<1;return &iic;}
bool IICTransmitChecked(IICInstance *i,const uint8_t *tx,uint16_t n,uint32_t timeout)
{if(transfer_fails)return false;HAL_I2C_Master_Transmit(i->handle,i->dev_address,(uint8_t *)tx,n,timeout);return true;}
GPIOInstance *GPIORegister(GPIO_Init_Config_s *config)
{assert(pin_count<2);pins[pin_count]=(GPIOInstance){.GPIOx=config->GPIOx,.GPIO_Pin=config->GPIO_Pin};return &pins[pin_count++];}
void GPIOSet(GPIOInstance *pin) {HAL_GPIO_WritePin(pin->GPIOx,pin->GPIO_Pin,GPIO_PIN_SET);}
void GPIOReset(GPIOInstance *pin) {HAL_GPIO_WritePin(pin->GPIOx,pin->GPIO_Pin,GPIO_PIN_RESET);}
void DWT_Delay_ms(float milliseconds) {(void)milliseconds;}
void HAL_GPIO_WritePin(GPIO_TypeDef *port,uint16_t pin,unsigned state)
{assert(port==&test_port);if(pin==OLED_DC_Pin) command_mode=state;}
static void Capture(unsigned mode,uint8_t byte)
{assert(count<10000);captured[count][0]=(unsigned char)mode;captured[count++][1]=byte;}
int HAL_SPI_Transmit(SPI_HandleTypeDef *h,uint8_t *data,uint16_t n,uint32_t timeout)
{assert(h==&hspi1 && n==1 && timeout>0);Capture(command_mode,data[0]);return 0;}
int HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h,uint16_t address,uint8_t *data,uint16_t n,uint32_t timeout)
{assert(h==&hi2c2 && address==0x78 && n==2 && timeout>0);Capture(data[0]==0x40,data[1]);return 0;}
int HAL_ADC_Start(ADC_HandleTypeDef *h) {(void)h;return 0;}
int HAL_ADC_PollForConversion(ADC_HandleTypeDef *h,uint32_t t) {(void)h;(void)t;return 0;}
unsigned HAL_ADC_GetState(ADC_HandleTypeDef *h) {(void)h;return 1;}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h) {(void)h;return 4095;}
int main(int argc,char **argv)
{
    bool record=argc>1 && strcmp(argv[1],"--record")==0;
    OLED_init();assert(count==0); /* No implicit hardware registration. */
    OLED_Init_Config_s config={.backend=OLED_USE_I2C ? OLED_BACKEND_I2C : OLED_BACKEND_SPI,
        .spi={.spi_handle=&hspi1,.cs_mode=SPI_CS_NONE},
        .iic={.handle=&hi2c2,.dev_address=0x3C},
        .dc={.GPIOx=&test_port,.GPIO_Pin=OLED_DC_Pin},
        .reset={.GPIOx=&test_port,.GPIO_Pin=OLED_RST_Pin}};
    assert(!OLEDRegister(NULL));assert(OLEDRegister(&config));assert(!OLEDRegister(&config));
    OLED_init();
#if OLED_USE_I2C
    OLED_operate_gram(PEN_CLEAR);OLED_refresh_gram();OLED_draw_point(3,9,PEN_WRITE);
#else
    OLED_clear(Pen_Clear);OLED_refresh_gram();OLED_draw_point(3,9,Pen_Write);
#endif
    OLED_refresh_gram();
    const char *filename=OLED_USE_I2C ? "baseline-i2c.txt" : "baseline-spi.txt";
    FILE *file=fopen(filename,record ? "wb" : "rb");assert(file);
    for(unsigned i=0;i<count;++i) {
        if(record) fprintf(file,"%02X%02X\n",captured[i][0],captured[i][1]);
        else {unsigned mode,byte;assert(fscanf(file,"%2X%2X\n",&mode,&byte)==2);
            assert(mode==captured[i][0] && byte==captured[i][1]);}
    }
    if(!record) assert(fgetc(file)==EOF);
    fclose(file);
    assert(OLEDTransportHealthy());
    unsigned before=count;transfer_fails=true;OLED_refresh_gram();
    assert(!OLEDTransportHealthy() && count==before && OLEDTransportFailureCount()==1);
    printf("PASS: OLED %s %u command/data bytes match baseline\n",OLED_USE_I2C?"I2C":"SPI",count);
}
