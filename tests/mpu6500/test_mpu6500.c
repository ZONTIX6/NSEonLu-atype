#include "mpu6500driver.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static SPIInstance spi;
static uint8_t registers[128],sample_bytes[14];
static unsigned transactions,last_length;
static bool fail_transfer;
uint32_t HAL_GetTick(void) {return 123;}
uint32_t DWT_GetCycleCount(void) {return 7000U+transactions;}
void DWT_Delay(float seconds) {(void)seconds;}
SPIInstance *SPIRegister(SPI_Init_Config_s *config) {(void)config;return &spi;}
BSP_SPI_Result_e SPITransferChecked(SPIInstance *instance,const uint8_t *tx,uint8_t *rx,uint16_t length,uint32_t timeout)
{
    assert(instance==&spi && timeout>0 && tx && rx);
    ++transactions;last_length=length;
    if(fail_transfer) return BSP_SPI_TIMEOUT;
    memset(rx,0,length);
    const uint8_t reg=tx[0]&0x7F;
    if(tx[0]&0x80) {
        if(reg==MPU6500_ACCEL_XOUT_H) {assert(length==15);memcpy(rx+1,sample_bytes,14);}
        else {
            assert(length==2);
            if(reg==MPU6500_I2C_SLV4_DI) {
                uint8_t ist_reg=registers[MPU6500_I2C_SLV4_REG];
                rx[1]=ist_reg==IST8310_WHO_AM_I ? IST8310_DEVICE_ID_A :
                    ist_reg==IST8310_AVGCNTL ? 0x24 : ist_reg==IST8310_PDCNTL ? 0xC0 : 0;
            } else rx[1]=registers[reg];
        }
    } else {assert(length==2);registers[reg]=tx[1];}
    return BSP_SPI_OK;
}
int main(int argc,char **argv)
{
    (void)argv;
    fail_transfer=argc>1;
    MPU6500_Init_Config_s config={.spi_config={0},.timeout_us=500};
    assert(MPU6500Register(NULL)==NULL);
    MPU6500Instance *device=MPU6500Register(&config);
    if(fail_transfer) {assert(device==NULL);assert(MPU6500Register(&config)==NULL);return 0;}
    assert(device && MPU6500Register(&config)==NULL);
    sample_bytes[0]=0x10;sample_bytes[2]=0xF0;sample_bytes[4]=0x10;
    sample_bytes[6]=0xD8;sample_bytes[7]=0xF0; /* -10000 / 333.87 + 21 */
    sample_bytes[8]=0x03;sample_bytes[9]=0xE8;sample_bytes[10]=0xFC;sample_bytes[11]=0x18;
    IMUSample_s sample;
    unsigned before=transactions;
    assert(MPU6500Read(device,&sample));
    assert(transactions==before+1 && last_length==15 && sample.valid && sample.tick_ms==123);
    assert(sample.rx_cycles==7000U+before+1U);
    assert(fabsf(sample.accel_m_s2[0]-9.8f)<.01f && sample.accel_m_s2[1]<0);
    assert(fabsf(sample.gyro_rad_s[0]-.5321126f)<.00001f && sample.gyro_rad_s[1]<0);
    assert(sample.temperature_c<0 && sample.temperature_c>-20);
    fail_transfer=true;
    assert(!MPU6500Read(device,&sample) && !sample.valid && sample.rx_cycles==0);
    assert(!MPU6500Read(NULL,&sample));assert(!MPU6500Read(device,NULL));
    puts("PASS: MPU6500 single-CS 14-byte samples, units and failed sample invalidation");
}
