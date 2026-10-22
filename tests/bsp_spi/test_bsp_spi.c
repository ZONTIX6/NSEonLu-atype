#include "bsp_spi.h"
#include "bsp_spi.h" /* Public headers must tolerate repeated inclusion. */
#include <assert.h>
#include <stdio.h>

SPI_TypeDef test_spi1,test_spi5;
static uint32_t cycles,frequency=1000000;
static unsigned low_count,high_count,callbacks;
static HAL_StatusTypeDef async_result=HAL_OK;
uint32_t DWT_GetFrequencyHz(void) {return frequency;}
uint32_t DWT_GetCycleCount(void) {return cycles++;}
void HAL_GPIO_WritePin(GPIO_TypeDef *port,uint16_t pin,GPIO_PinState state)
{assert(port && pin);if(state) ++high_count;else ++low_count;}
static void Callback(SPIInstance *instance) {(void)instance;++callbacks;}
#define STUB(name) HAL_StatusTypeDef name(SPI_HandleTypeDef *h,uint8_t *data,uint16_t n) \
{(void)h;(void)data;(void)n;return async_result;}
STUB(HAL_SPI_Transmit_DMA) STUB(HAL_SPI_Transmit_IT)
STUB(HAL_SPI_Receive_DMA) STUB(HAL_SPI_Receive_IT)
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef *h,uint8_t *tx,uint8_t *rx,uint16_t n)
{(void)rx;return HAL_SPI_Transmit_DMA(h,tx,n);}
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *h,uint8_t *tx,uint8_t *rx,uint16_t n)
{(void)rx;return HAL_SPI_Transmit_IT(h,tx,n);}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *);
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *);
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *);

int main(void)
{
    SPI_HandleTypeDef h1={.Instance=SPI1,.State=HAL_SPI_STATE_READY};
    SPI_HandleTypeDef h5={.Instance=SPI5,.State=HAL_SPI_STATE_READY};
    GPIO_TypeDef port;
    SPI_Init_Config_s config={.spi_handle=&h5,.GPIOx=&port,.cs_pin=1,.callback=Callback};
    assert(SPIRegister(NULL)==NULL);
    SPIInstance *device=SPIRegister(&config);assert(device);
    config.spi_handle=&h1;config.cs_mode=SPI_CS_NONE;config.GPIOx=NULL;config.cs_pin=0;
    SPIInstance *no_cs=SPIRegister(&config);assert(no_cs);
    uint8_t tx[]={0x80,0x12,0x34},rx[3];
    assert(SPITransferChecked(NULL,tx,rx,3,10)==BSP_SPI_INVALID);
    assert(SPITransferChecked(device,NULL,NULL,3,10)==BSP_SPI_INVALID);
    frequency=0;assert(SPITransferChecked(device,tx,rx,3,10)==BSP_SPI_INVALID);frequency=1000000;
    test_spi5.SR=SPI_FLAG_TXE|SPI_FLAG_RXNE;
    assert(SPITransferChecked(device,tx,rx,3,10)==BSP_SPI_OK);
    assert(rx[0]==tx[0] && rx[2]==tx[2]);
    assert(low_count==1 && high_count==1 && h5.State==HAL_SPI_STATE_READY);
    unsigned pins=low_count+high_count;
    test_spi1.SR=SPI_FLAG_TXE|SPI_FLAG_RXNE;
    assert(SPITransferChecked(no_cs,tx,rx,3,10)==BSP_SPI_OK);
    assert(low_count+high_count==pins);
    /* System tick is absent: the cycle clock still bounds polling and releases CS. */
    test_spi5.SR=0;
    assert(SPITransferChecked(device,tx,rx,3,10)==BSP_SPI_TIMEOUT);
    assert(low_count==2 && high_count==2 && h5.State==HAL_SPI_STATE_READY);
    test_spi5.SR=SPI_FLAG_MODF;
    assert(SPITransferChecked(device,tx,rx,3,10)==BSP_SPI_ERROR);
    assert(low_count==3 && high_count==3);
    h5.State=HAL_SPI_STATE_BUSY_TX_RX;
    assert(SPITransferChecked(device,tx,rx,3,10)==BSP_SPI_BUSY);
    assert(low_count==3 && high_count==3);h5.State=HAL_SPI_STATE_READY;
    SPISetMode(no_cs,SPI_DMA_MODE);
    SPITransRecv(no_cs,rx,tx,3);
    assert(SPITransferChecked(no_cs,tx,rx,3,10)==BSP_SPI_BUSY);
    HAL_SPI_TxRxCpltCallback(&h1);HAL_SPI_RxCpltCallback(&h1);
    assert(callbacks==1);
    assert(SPITransferChecked(no_cs,tx,rx,3,10)==BSP_SPI_OK);
    SPITransRecv(no_cs,rx,tx,3);HAL_SPI_ErrorCallback(&h1);HAL_SPI_ErrorCallback(&h1);
    assert(callbacks==2 && no_cs->last_result==BSP_SPI_ERROR);
    async_result=HAL_ERROR;SPITransmit(no_cs,tx,3);
    assert(no_cs->last_result==BSP_SPI_ERROR);
    async_result=HAL_OK;SPITransmit(no_cs,tx,3);HAL_SPI_TxCpltCallback(&h1);
    assert(callbacks==3);
    cycles=UINT32_MAX-4;test_spi5.SR=0;
    assert(SPITransferChecked(device,tx,rx,3,10)==BSP_SPI_TIMEOUT);
    puts("PASS: SPI1/SPI5 checked transactions, bounded timeout and async ownership");
}
