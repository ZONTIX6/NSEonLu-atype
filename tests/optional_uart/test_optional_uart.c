#include "TFminiPlus.h"
#include "HC05.h"
#include "daemon.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static USARTInstance uart;
static unsigned reloads,sends;
static uint8_t sent[16];
static unsigned sent_length;
USARTInstance *USARTRegister(USART_Init_Config_s *config)
{uart.module_callback=config->module_callback;return &uart;}
void USARTServiceInit(USARTInstance *instance) {(void)instance;}
void USARTSend(USARTInstance *instance,uint8_t *data,uint16_t length,USART_TRANSFER_MODE mode)
{(void)instance;assert(mode==USART_TRANSFER_IT);++sends;sent_length=length;memcpy(sent,data,length);}
DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{(void)config;static DaemonInstance d;return &d;}
void DaemonReload(DaemonInstance *d) {(void)d;++reloads;}

int main(void)
{
    TFminiPlus_Recv_s *sample=TFminiPlusInit(NULL);
    const uint8_t frame[9]={0x59,0x59,0x64,0,0xc8,0,0x81,0x08,0x67};
    memcpy(uart.recv_buff,frame,9);uart.recv_data_len=8;uart.module_callback();
    assert(reloads==0 && sample->distance==0);
    uart.recv_data_len=9;uart.recv_buff[8]^=1;uart.module_callback();assert(reloads==0);
    uart.recv_buff[8]^=1;uart.module_callback();
    assert(reloads==1 && sample->distance==100 && sample->strength==200);
    assert(sample->temperature==16.125f);
    HC05Init(NULL);
    uint8_t data[5]={1,2,3,4,5};
    HC05_SendData(data,5);assert(!sends);
    HC05_SendData(data,2);
    assert(sends==1 && sent_length==4 && !memcmp(sent,(uint8_t[]){0xaa,1,2,0x55},4));
    puts("PASS: optional UART frame validation, units and send bounds");
}
