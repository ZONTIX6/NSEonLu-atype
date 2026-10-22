#include "can_comm.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CAN_HandleTypeDef hcan1,hcan2;
static uint32_t tick;
static CANInstance *bus;
static unsigned damage;
uint32_t HAL_GetTick(void) {return tick;}
CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    bus=calloc(1,sizeof *bus);bus->id=config->id;
    bus->can_module_callback=config->can_module_callback;return bus;
}
void CANSetDLC(CANInstance *can,uint8_t length) {can->txconf.DLC=length;}
uint8_t CANTransmit(CANInstance *can,float timeout)
{
    (void)timeout;can->rx_len=can->txconf.DLC;
    memcpy(can->rx_buff,can->tx_buff,can->rx_len);
    if(damage && can->rx_len>3) can->rx_buff[3]^=1;
    can->can_module_callback(can);return 1;
}
DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{(void)config;static DaemonInstance d;d.online=1;return &d;}
void DaemonReload(DaemonInstance *d) {(void)d;}
uint8_t DaemonIsOnline(DaemonInstance *d) {return d->online;}

int main(void)
{
    CANComm_Init_Config_s config={.send_data_len=100,.recv_data_len=100};
    CANCommInstance *comm=CANCommInit(&config);
    assert(comm);
    uint8_t input[100],output[100];
    for(unsigned i=0;i<100;++i) input[i]=(uint8_t)(i+1);
    assert(!CANCommCopy(comm,output,sizeof output));
    tick=10;CANCommSend(comm,input);
    uint32_t received=0;
    assert(CANCommCopyAt(comm,output,sizeof output,&received) && !memcmp(input,output,sizeof input));
    assert(received==10);
    tick=20;assert(CANCommCopyFreshAt(comm,output,sizeof output,&received,10));
    tick=21;assert(!CANCommCopyFreshAt(comm,output,sizeof output,&received,10));
    tick=110;assert(CANCommCopy(comm,output,sizeof output));
    tick=111;assert(!CANCommCopy(comm,output,sizeof output));
    damage=1;CANCommSend(comm,input);
    assert(!CANCommCopy(comm,output,sizeof output));
    damage=0;tick=112;CANCommSend(comm,input);
    assert(CANCommCopy(comm,output,sizeof output));
    assert(!CANCommCopy(comm,output,99));
    config.send_data_len=129;
    assert(CANCommInit(&config)==NULL);
    puts("PASS: dual-board fragments, CRC, sizes and receive freshness");
}
