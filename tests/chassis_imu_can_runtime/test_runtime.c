#include "chassis_imu_can.h"
#include "bsp_can.h"
#include "ins_task.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

CAN_HandleTypeDef hcan1={1},hcan2={2};
static CANInstance registered;
static unsigned registrations,critical;
static uint32_t tick;
uint32_t HAL_GetTick(void) { return tick; }
void TestEnterCritical(void) { ++critical; }
void TestExitCritical(void) { assert(critical); --critical; }
CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    assert(config->can_handle==&hcan2 && config->tx_id==0x520 && config->rx_id==0x520);
    registered=(CANInstance){.can_handle=config->can_handle,.tx_id=config->tx_id,
        .rx_id=config->rx_id,.can_module_callback=config->can_module_callback,.dlc=8};
    ++registrations;
    return &registered;
}
void CANSetDLC(CANInstance *instance,uint8_t length)
{ assert(instance==&registered && length==8);instance->dlc=length; }

#if IMU_CAPTURE_CAN_TX_ENABLE
static INSMotionSnapshot_s imu={.quaternion={1,0,0,0},.accel_n_mps2={.25f,-.5f,1},
    .gyro_sensor_radps={2,-3,4},.tick_ms=UINT32_MAX-4,.sequence=123,.valid=true};
static uint8_t sent[9][8];
static unsigned calls,fail_at,aborts;
static uint32_t pending,abort_mask,send_tick_step;
static bool abort_stuck;
bool INSGetMotionSnapshot(INSMotionSnapshot_s *out) { *out=imu;return imu.valid; }
uint8_t CANTransmit(CANInstance *instance,float timeout)
{
    assert(instance==&registered && instance->can_handle==&hcan2 && instance->tx_id==0x520);
    assert(instance->dlc==8 && timeout>0 && timeout<=1.0f);
    ++calls;
    /* Four high-priority motor feedback frames can occupy the bus for 0.5 ms. */
    if(timeout<.5f) return 0;
    if(calls==fail_at) return 0;
    assert(calls<=9);
    memcpy(sent[calls-1],instance->tx_buff,8);
    instance->tx_mailbox=1U<<((calls-1)%3);
    /* Model recently submitted mailboxes as pending until explicitly cleared. */
    pending|=instance->tx_mailbox;
    tick+=send_tick_step;
    return 1;
}
uint32_t HAL_CAN_IsTxMessagePending(CAN_HandleTypeDef *can,uint32_t mask)
{ assert(can==&hcan2);return (pending&mask)!=0; }
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef *can,uint32_t mask)
{
    assert(can==&hcan2 && mask && (mask&~7U)==0);
    ++aborts;abort_mask=mask;
    if(!abort_stuck) pending&=~mask;
    return HAL_OK;
}
static ChassisImuCanSnapshot_s DecodeSent(void)
{
    ChassisImuCanDecoder_s decoder;
    ChassisImuCanSnapshot_s out;
    ChassisImuCanDecoderInit(&decoder);
    for(unsigned i=0;i<9;++i)
        assert(ChassisImuCanReceive(&decoder,sent[i],8,500+i)==(i==8));
    assert(ChassisImuCanGetSnapshot(&decoder,&out));
    return out;
}
int main(void)
{
    ChassisImuCanSend(0);assert(calls==0);
    ChassisImuCanInit();ChassisImuCanInit();
    assert(registrations==1 && registered.can_module_callback==NULL);
    ChassisImuCanSend(0);
    assert(calls==9 && ChassisImuCanTxDropCount()==0);
    ChassisImuCanSnapshot_s out=DecodeSent();
    assert(out.sample.sequence==123 && out.sample.tick_ms==UINT32_MAX-4);
    assert(out.sample.source_age_ms==5 && out.sample.valid && out.sample.accel_n_mps2[1]==-.5f);
    const uint8_t first_tag=sent[0][1];
    ChassisImuCanSend(14);assert(calls==9);

    /* A failed third enqueue must terminate this group and cancel its mailboxes. */
    pending=0;calls=0;fail_at=3;tick=20;imu.tick_ms=20;
    ChassisImuCanSend(20);
    assert(calls==3 && ChassisImuCanTxDropCount()==1 && aborts==1 && abort_mask==3);
    assert(pending==0 && sent[0][1]!=(uint8_t)first_tag);

    /* A complete enqueue still pending at the next period is cancelled before new data. */
    calls=0;fail_at=0;tick=40;imu.tick_ms=40;
    ChassisImuCanSend(40);assert(calls==9 && pending==7);
    calls=0;tick=60;imu.tick_ms=60;
    ChassisImuCanSend(60);
    assert(calls==9 && aborts==2 && abort_mask==7 && ChassisImuCanTxDropCount()==2);

    /* An abort which has not completed prevents a new transfer from mixing in. */
    calls=0;tick=80;abort_stuck=true;
    ChassisImuCanSend(80);
    assert(calls==0 && aborts==3 && ChassisImuCanTxDropCount()>=3);
    abort_stuck=false;pending=0;calls=0;tick=100;imu.valid=false;
    imu.quaternion[0]=NAN;
    ChassisImuCanSend(100);assert(calls==9);
    out=DecodeSent();assert(!out.sample.valid && out.sample.quaternion[0]==0);

    /* A preemption or slow bus that crosses the assembly budget aborts the group. */
    pending=0;calls=0;tick=120;imu.tick_ms=120;imu.valid=true;imu.quaternion[0]=1;
    send_tick_step=11;
    const uint32_t drops_before_timeout=ChassisImuCanTxDropCount();
    ChassisImuCanSend(120);
    assert(calls==1 && pending==0 && ChassisImuCanTxDropCount()==drops_before_timeout+1);
    send_tick_step=0;calls=0;tick=140;imu.tick_ms=119;
    ChassisImuCanSend(140);assert(calls==9);
    out=DecodeSent();assert(!out.sample.valid && out.sample.source_age_ms==21);

    /* A 20 ms vTaskDelayUntil task may run one tick late, then on time.
     * Do not drop the 180 ms group merely because 180-161 is only 19 ms. */
    pending=0;calls=0;tick=161;imu.tick_ms=161;
    ChassisImuCanSend(161);assert(calls==9);
    pending=0;calls=0;tick=180;imu.tick_ms=180;
    ChassisImuCanSend(180);assert(calls==9);
    out=DecodeSent();assert(out.sample.valid && out.sample.tick_ms==180);

    /* Catch-up invocations shorter than the fusion minimum are still skipped. */
    pending=0;calls=0;tick=194;imu.tick_ms=194;
    ChassisImuCanSend(194);assert(calls==0);
    tick=200;imu.tick_ms=200;
    ChassisImuCanSend(200);assert(calls==9);
    puts("PASS: CAN IMU TX cadence, source time, cancellation and diagnostic-only ID");
    return 0;
}
#else
uint8_t CANTransmit(CANInstance *instance,float timeout)
{ (void)instance;(void)timeout;assert(!"RX board must not transmit through this module");return 0; }
uint32_t HAL_CAN_IsTxMessagePending(CAN_HandleTypeDef *can,uint32_t mask)
{ (void)can;(void)mask;assert(0);return 0; }
HAL_StatusTypeDef HAL_CAN_AbortTxRequest(CAN_HandleTypeDef *can,uint32_t mask)
{ (void)can;(void)mask;assert(0);return HAL_ERROR; }
static void Receive(const ChassisImuCanSample_s *sample,uint8_t tag,uint32_t start)
{
    uint8_t frames[9][8];
    assert(ChassisImuCanEncode(sample,tag,frames));
    for(unsigned i=0;i<9;++i) {
        memcpy(registered.rx_buff,frames[i],8);registered.rx_len=8;tick=start+i;
        registered.can_module_callback(&registered);
    }
}
int main(void)
{
    ChassisImuCanSnapshot_s out;
    assert(!ChassisImuCanReadSnapshot(NULL,0));
    assert(!ChassisImuCanReadSnapshot(&out,0) && !out.available);
    ChassisImuCanInit();ChassisImuCanInit();
    assert(registrations==1 && registered.can_module_callback);
    ChassisImuCanSample_s sample={.quaternion={1,0,0,0},.sequence=7,
        .tick_ms=123456789,.source_age_ms=5,.valid=true};
    Receive(&sample,1,UINT32_MAX-4);
    assert(ChassisImuCanReadSnapshot(&out,4) && critical==0);
    assert(out.available && out.sample.tick_ms==123456789 && out.sample.sequence==7);
    assert(out.first_rx_tick_ms==UINT32_MAX-4 && out.last_rx_tick_ms==3);
    assert(ChassisImuCanReadSnapshot(&out,50)); /* 55 ms local + 5 ms source. */
    assert(!ChassisImuCanReadSnapshot(&out,51) && out.sample.valid);
    sample.sequence=8;sample.valid=false;Receive(&sample,2,100);
    assert(!ChassisImuCanReadSnapshot(&out,108) && out.available && !out.sample.valid);
    sample.sequence=9;sample.valid=true;sample.source_age_ms=21;Receive(&sample,3,200);
    assert(!ChassisImuCanReadSnapshot(&out,208) && out.sample.valid);
    sample.sequence=10;sample.source_age_ms=UINT16_MAX;Receive(&sample,4,300);
    assert(!ChassisImuCanReadSnapshot(&out,308));
    registered.rx_len=7;tick=400;registered.can_module_callback(&registered);
    assert(!ChassisImuCanReadSnapshot(&out,400) && out.rejected_groups>0);
    puts("PASS: CAN IMU RX age bounds, independent clocks, invalid sources and coherent snapshot");
    return 0;
}
#endif
