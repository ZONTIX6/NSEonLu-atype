#include "vt02_control.h"
#include "bsp_usart.h"
#include "daemon.h"
#include "crc_ref.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static USARTInstance uart;
static uint32_t now;
static unsigned reloads;
static unsigned dma_inits;
static unsigned receive_restarts;
static DaemonInstance daemon;
static Daemon_Init_Config_s daemon_config;
uint32_t HAL_GetTick(void) {return now;}
int HAL_DMA_Init(DMA_HandleTypeDef *dma) {(void)dma;++dma_inits;return 0;}
bool USARTSupportsIdleStream(const UART_HandleTypeDef *handle)
{return handle && handle->hdmarx && handle->hdmarx->Init.FIFOMode==DMA_FIFOMODE_DISABLE;}
USARTInstance *USARTRegister(USART_Init_Config_s *config)
{uart.usart_handle=config->usart_handle;uart.recv_buff_size=config->recv_buff_size;uart.module_callback=config->module_callback;
    config->module_callback(); /* DMA IRQ can arrive before registration returns. */
    return &uart;}
void USARTServiceInit(USARTInstance *u) {assert(u==&uart);++receive_restarts;}
DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{daemon_config=*config;return &daemon;}
void DaemonReload(DaemonInstance *d) {d->online=true;++reloads;}
uint8_t DaemonIsOnline(DaemonInstance *d) {return d->online;}
static void feed(const uint8_t *data,unsigned length)
{memcpy(uart.recv_buff,data,length);uart.recv_data_len=length;uart.module_callback();}
int main(void)
{
    DMA_HandleTypeDef dma={.Init={.FIFOMode=1}};
    UART_HandleTypeDef handle={.hdmarx=&dma};
    assert(Vt02ControlInit(NULL)==NULL);
    assert(Vt02ControlInit(&handle)==NULL);
    assert(dma.Init.FIFOMode==1 && dma_inits==0);
    dma.Init.FIFOMode=DMA_FIFOMODE_DISABLE;
    assert(Vt02ControlInit(&handle)!=NULL);
    assert(Vt02ControlInit(&handle)!=NULL && dma_inits==0);
    Vt02_Ctrl_t snapshot;
    assert(!Vt02ControlGetSnapshot(&snapshot));
    uint8_t frame[21]={0xa5,12,0,1,0,4,3,0xec,0xff,30,0,0,0,1,0,1,0,0,0,0,0};
    Append_CRC8_Check_Sum(frame,5);Append_CRC16_Check_Sum(frame,sizeof frame);
    frame[20]^=1;feed(frame,sizeof frame);
    assert(!reloads && !Vt02ControlGetSnapshot(&snapshot));
    frame[20]^=1;now=100;feed(frame,8);assert(!reloads);
    feed(frame+8,sizeof frame-8);
    assert(reloads==1 && Vt02ControlGetSnapshot(&snapshot));
    assert(snapshot.mouse.x==-20 && snapshot.mouse.y==30 && snapshot.key[KEY_PRESS].keys==1);
    now=201;frame[20]^=1;feed(frame,sizeof frame);
    assert(reloads==1 && !Vt02ControlGetSnapshot(&snapshot));
    frame[20]^=1;feed(frame,sizeof frame);
    assert(reloads==2 && Vt02ControlGetSnapshot(&snapshot));
    daemon.online=false;daemon_config.callback(daemon_config.owner_id);
    assert(receive_restarts==1 && !Vt02ControlGetSnapshot(&snapshot));
    feed(frame,sizeof frame);assert(reloads==3 && Vt02ControlGetSnapshot(&snapshot));
    puts("PASS: VTX CRC/fragmentation, coherent snapshot, expiry and recovery");
}
