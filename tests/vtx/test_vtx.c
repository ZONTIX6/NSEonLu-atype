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
uint32_t HAL_GetTick(void) {return now;}
int HAL_DMA_Init(DMA_HandleTypeDef *dma) {(void)dma;return 0;}
USARTInstance *USARTRegister(USART_Init_Config_s *config)
{uart.usart_handle=config->usart_handle;uart.recv_buff_size=config->recv_buff_size;uart.module_callback=config->module_callback;return &uart;}
void USARTServiceInit(USARTInstance *u) {(void)u;}
DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{(void)config;static DaemonInstance d;return &d;}
void DaemonReload(DaemonInstance *d) {d->online=true;++reloads;}
uint8_t DaemonIsOnline(DaemonInstance *d) {return d->online;}
static void feed(const uint8_t *data,unsigned length)
{memcpy(uart.recv_buff,data,length);uart.recv_data_len=length;uart.module_callback();}
int main(void)
{
    UART_HandleTypeDef handle={0};Vt02ControlInit(&handle);
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
    puts("PASS: VTX CRC/fragmentation, coherent snapshot, expiry and recovery");
}
