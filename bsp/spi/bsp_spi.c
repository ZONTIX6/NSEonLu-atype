#include "bsp_spi.h"
#include "bsp_dwt.h"
#include <stdlib.h>
#include <string.h>

static SPIInstance *spi_instance[MX_SPI_BUS_SLAVE_CNT];
static SPIInstance *active[SPI_DEVICE_CNT];
static uint8_t idx;
static uint8_t bus_free[SPI_DEVICE_CNT]={1,1};

static int Bus(const SPI_HandleTypeDef *handle)
{
    if(handle==NULL) return -1;
    if(handle->Instance==SPI1) return 0;
    if(handle->Instance==SPI5) return 1;
    return -1;
}
static bool Registered(const SPIInstance *instance)
{
    for(unsigned i=0;i<idx;++i) if(spi_instance[i]==instance) return true;
    return false;
}
static void Select(SPIInstance *instance,GPIO_PinState state)
{
    if(instance->cs_mode==SPI_CS_GPIO) HAL_GPIO_WritePin(instance->GPIOx,instance->cs_pin,state);
    instance->CS_State=(uint8_t)state;
}
static BSP_SPI_Result_e Acquire(SPIInstance *instance)
{
    if(instance==NULL || !Registered(instance)) return BSP_SPI_INVALID;
    int bus=Bus(instance->spi_handle);
    if(bus<0) return BSP_SPI_INVALID;
    uint32_t mask=__get_PRIMASK();__disable_irq();
    const bool busy=active[bus]!=NULL || instance->spi_handle->State!=HAL_SPI_STATE_READY ||
        instance->spi_handle->Lock==HAL_LOCKED;
    if(!busy) {active[bus]=instance;bus_free[bus]=0;}
    __set_PRIMASK(mask);
    if(busy) return BSP_SPI_BUSY;
    Select(instance,GPIO_PIN_RESET);
    return BSP_SPI_OK;
}
static void Release(SPIInstance *instance,BSP_SPI_Result_e result)
{
    Select(instance,GPIO_PIN_SET);
    instance->last_result=result;
    uint32_t mask=__get_PRIMASK();__disable_irq();
    const int bus=Bus(instance->spi_handle);
    if(bus>=0 && active[bus]==instance) {active[bus]=NULL;bus_free[bus]=1;}
    __set_PRIMASK(mask);
}
SPIInstance *SPIRegister(SPI_Init_Config_s *conf)
{
    if(conf==NULL || idx>=MX_SPI_BUS_SLAVE_CNT || Bus(conf->spi_handle)<0 ||
        conf->spi_work_mode>SPI_DMA_MODE || conf->cs_mode>SPI_CS_NONE ||
        (conf->cs_mode==SPI_CS_GPIO && (conf->GPIOx==NULL || conf->cs_pin==0))) return NULL;
    SPIInstance *instance=calloc(1,sizeof *instance);
    if(instance==NULL) return NULL;
    instance->spi_handle=conf->spi_handle;instance->GPIOx=conf->GPIOx;instance->cs_pin=conf->cs_pin;
    instance->cs_mode=conf->cs_mode;instance->spi_work_mode=conf->spi_work_mode;
    instance->callback=conf->callback;instance->id=conf->id;
    instance->cs_pin_state=&bus_free[Bus(conf->spi_handle)];instance->CS_State=1;
    spi_instance[idx++]=instance;
    return instance;
}
static BSP_SPI_Result_e WaitFlag(SPI_HandleTypeDef *handle,uint32_t flag,bool set,
                                 uint32_t start,uint32_t budget)
{
    for(;;) {
        const uint32_t status=handle->Instance->SR;
        if(status & (SPI_FLAG_OVR|SPI_FLAG_MODF|SPI_FLAG_CRCERR)) return BSP_SPI_ERROR;
        if(((status & flag)!=0)==set) return BSP_SPI_OK;
        if((uint32_t)(DWT_GetCycleCount()-start)>=budget) return BSP_SPI_TIMEOUT;
    }
}
BSP_SPI_Result_e SPITransferChecked(SPIInstance *instance,const uint8_t *tx,uint8_t *rx,
                                    uint16_t length,uint32_t timeout_us)
{
    const uint32_t frequency=DWT_GetFrequencyHz();
    if(instance==NULL || !Registered(instance) || (tx==NULL && rx==NULL) || length==0 ||
        timeout_us==0 || timeout_us>100000U || frequency==0 ||
        instance->spi_handle->Init.DataSize!=SPI_DATASIZE_8BIT ||
        instance->spi_handle->Init.Direction!=SPI_DIRECTION_2LINES) return BSP_SPI_INVALID;
    BSP_SPI_Result_e result=Acquire(instance);
    if(result!=BSP_SPI_OK) return result;
    SPI_HandleTypeDef *handle=instance->spi_handle;
    handle->Lock=HAL_LOCKED;handle->State=HAL_SPI_STATE_BUSY_TX_RX;handle->ErrorCode=HAL_SPI_ERROR_NONE;
    const uint32_t budget=(uint32_t)(((uint64_t)frequency*timeout_us+999999U)/1000000U);
    const uint32_t start=DWT_GetCycleCount();
    __HAL_SPI_ENABLE(handle);
    for(uint16_t i=0;i<length;++i) {
        result=WaitFlag(handle,SPI_FLAG_TXE,true,start,budget);
        if(result!=BSP_SPI_OK) break;
        *((volatile uint8_t *)&handle->Instance->DR)=tx==NULL ? 0xFFU : tx[i];
        result=WaitFlag(handle,SPI_FLAG_RXNE,true,start,budget);
        if(result!=BSP_SPI_OK) break;
        const uint8_t byte=*((volatile uint8_t *)&handle->Instance->DR);
        if(rx!=NULL) rx[i]=byte;
        if((uint32_t)(DWT_GetCycleCount()-start)>=budget) {result=BSP_SPI_TIMEOUT;break;}
    }
    if(result==BSP_SPI_OK) result=WaitFlag(handle,SPI_FLAG_TXE,true,start,budget);
    if(result==BSP_SPI_OK) result=WaitFlag(handle,SPI_FLAG_BSY,false,start,budget);
    if(result!=BSP_SPI_OK) {
        __HAL_SPI_DISABLE(handle);
        __HAL_SPI_CLEAR_OVRFLAG(handle);__HAL_SPI_CLEAR_MODFFLAG(handle);__HAL_SPI_CLEAR_CRCERRFLAG(handle);
        handle->ErrorCode=HAL_SPI_ERROR_FLAG;
    }
    handle->State=HAL_SPI_STATE_READY;handle->Lock=HAL_UNLOCKED;
    Release(instance,result);
    return result;
}
static void LegacyTransfer(SPIInstance *instance,uint8_t *tx,uint8_t *rx,uint8_t length)
{
    if(instance==NULL || length==0 || (tx==NULL && rx==NULL)) return;
    if(instance->spi_work_mode==SPI_BLOCK_MODE) {
        instance->last_result=SPITransferChecked(instance,tx,rx,length,50000U);
        return;
    }
    BSP_SPI_Result_e result=Acquire(instance);
    if(result!=BSP_SPI_OK) {instance->last_result=result;return;}
    instance->rx_buffer=rx;instance->rx_size=length;
    HAL_StatusTypeDef status;
    if(instance->spi_work_mode==SPI_DMA_MODE) {
        if(tx!=NULL && rx!=NULL) status=HAL_SPI_TransmitReceive_DMA(instance->spi_handle,tx,rx,length);
        else if(tx!=NULL) status=HAL_SPI_Transmit_DMA(instance->spi_handle,tx,length);
        else status=HAL_SPI_Receive_DMA(instance->spi_handle,rx,length);
    } else {
        if(tx!=NULL && rx!=NULL) status=HAL_SPI_TransmitReceive_IT(instance->spi_handle,tx,rx,length);
        else if(tx!=NULL) status=HAL_SPI_Transmit_IT(instance->spi_handle,tx,length);
        else status=HAL_SPI_Receive_IT(instance->spi_handle,rx,length);
    }
    if(status!=HAL_OK) Release(instance,status==HAL_BUSY ? BSP_SPI_BUSY : BSP_SPI_ERROR);
}
void SPITransmit(SPIInstance *instance,uint8_t *data,uint8_t length) {LegacyTransfer(instance,data,NULL,length);}
void SPIRecv(SPIInstance *instance,uint8_t *data,uint8_t length) {LegacyTransfer(instance,NULL,data,length);}
void SPITransRecv(SPIInstance *instance,uint8_t *rx,uint8_t *tx,uint8_t length) {LegacyTransfer(instance,tx,rx,length);}
void SPISetMode(SPIInstance *instance,SPI_TXRX_MODE_e mode)
{
    if(instance!=NULL && Registered(instance) && mode<=SPI_DMA_MODE &&
        active[Bus(instance->spi_handle)]!=instance) instance->spi_work_mode=mode;
}
static void Complete(SPI_HandleTypeDef *handle,BSP_SPI_Result_e result)
{
    int bus=Bus(handle);
    if(bus<0 || active[bus]==NULL) return;
    SPIInstance *instance=active[bus];
    Release(instance,result);
    if(instance->callback!=NULL) instance->callback(instance);
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *handle) {Complete(handle,BSP_SPI_OK);}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *handle) {Complete(handle,BSP_SPI_OK);}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *handle) {Complete(handle,BSP_SPI_OK);}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *handle) {Complete(handle,BSP_SPI_ERROR);}
void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *handle) {Complete(handle,BSP_SPI_ERROR);}
