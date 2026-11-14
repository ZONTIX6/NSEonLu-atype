#include "bsp_can.h"
#include "main.h"
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static CAN_TypeDef registers[2]={{.MCR=CAN_MCR_INRQ},{.MCR=CAN_MCR_INRQ}};
CAN_HandleTypeDef hcan1={.Instance=&registers[0],.State=HAL_CAN_STATE_READY};
CAN_HandleTypeDef hcan2={.Instance=&registers[1],.State=HAL_CAN_STATE_READY};
static uint32_t frequency=1000000U,cycles,cycle_reads,clear_after_reads;
static uint32_t primask=1U,initial_primask=1U;
static unsigned notifications[2],transmits,errors;
static uint32_t notification_mask[2];
static bool fail_notification,expect_error;
static jmp_buf error_return;

static unsigned Bus(CAN_HandleTypeDef *handle)
{
    assert(handle==&hcan1 || handle==&hcan2);
    return handle==&hcan1 ? 0U : 1U;
}
uint32_t __get_PRIMASK(void) {return primask;}
void __disable_irq(void) {primask=1;}
void __enable_irq(void) {primask=0;}
void __set_PRIMASK(uint32_t mask) {primask=mask;}
/* RobotInit masks the timer interrupt, so the original HAL timeout never advances. */
uint32_t HAL_GetTick(void) {return 123U;}
uint32_t DWT_GetFrequencyHz(void) {return frequency;}
uint32_t DWT_GetCycleCount(void)
{
    ++cycle_reads;
    cycles+=1000U; /* One millisecond per read at the fixture's 1 MHz clock. */
    if(clear_after_reads && cycle_reads>=clear_after_reads &&
       !(registers[0].MCR&CAN_MCR_INRQ)) registers[0].MSR=0;
    return cycles;
}
float DWT_GetTimeline_ms(void) {return (float)cycles/1000.0f;}
void Error_Handler(void)
{
    assert(expect_error);
    assert(primask==initial_primask);
    ++errors;
    longjmp(error_return,1);
}
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *handle,uint32_t flags)
{
    const unsigned bus=Bus(handle);
    assert(handle->State==HAL_CAN_STATE_LISTENING);
    assert((handle->Instance->MSR&CAN_MSR_INAK)==0);
    assert(flags && (flags&~(CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_RX_FIFO1_MSG_PENDING))==0);
    assert((notification_mask[bus]&flags)==0); /* No duplicate activation on later registration. */
    ++notifications[bus];
    if(fail_notification) return HAL_ERROR;
    notification_mask[bus]|=flags;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *handle,CAN_FilterTypeDef *filter)
{
    const unsigned bus=Bus(handle);
    assert(filter->FilterActivation==CAN_FILTER_ENABLE);
    assert(filter->SlaveStartFilterBank==14);
    assert(bus==0 ? filter->FilterBank<14 : filter->FilterBank>=14);
    return HAL_OK;
}
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *handle)
{
    assert(handle->State==HAL_CAN_STATE_LISTENING);
    return 3;
}
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *handle,CAN_TxHeaderTypeDef *header,
                                     uint8_t *data,uint32_t *mailbox)
{
    static const uint8_t expected[8]={10,11,12,13,14,15,16,17};
    assert(handle==&hcan1 && handle->State==HAL_CAN_STATE_LISTENING);
    assert(header->StdId==0x520 && header->DLC==8);
    assert(memcmp(data,expected,8)==0);
    assert(primask==initial_primask);
    *mailbox=1;
    ++transmits;
    return HAL_OK;
}
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *handle,uint32_t fifo)
{(void)handle;(void)fifo;return 0;}
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *handle,uint32_t fifo,
                                     CAN_RxHeaderTypeDef *header,uint8_t *data)
{(void)handle;(void)fifo;(void)header;(void)data;assert(false);return HAL_ERROR;}

static CANInstance *Register(CAN_HandleTypeDef *handle,uint32_t id)
{
    CAN_Init_Config_s config={.can_handle=handle,.tx_id=id,.rx_id=id};
    return CANRegister(&config);
}
static void CheckStarted(CAN_HandleTypeDef *handle)
{
    assert(handle->State==HAL_CAN_STATE_LISTENING && handle->ErrorCode==HAL_CAN_ERROR_NONE);
    assert((handle->Instance->MCR&CAN_MCR_INRQ)==0);
    assert(notification_mask[Bus(handle)]==(CAN_IT_RX_FIFO0_MSG_PENDING|CAN_IT_RX_FIFO1_MSG_PENDING));
}

int main(int argc,char **argv)
{
    assert(argc==2);
    const char *scenario=argv[1];
    if(strcmp(scenario,"unmasked")==0) initial_primask=primask=0;
    if(strcmp(scenario,"timeout")==0 || strcmp(scenario,"wrap_timeout")==0) {
        registers[0].MSR=CAN_MSR_INAK;
        if(strcmp(scenario,"wrap_timeout")==0) cycles=UINT32_MAX-4999U;
        expect_error=true;
    } else if(strcmp(scenario,"no_clock")==0) {
        frequency=0;registers[0].MSR=CAN_MSR_INAK;expect_error=true;
    } else if(strcmp(scenario,"not_ready")==0) {
        hcan1.State=HAL_CAN_STATE_RESET;expect_error=true;
    } else if(strcmp(scenario,"notification_error")==0) {
        fail_notification=true;expect_error=true;
    } else if(strcmp(scenario,"delayed_ack")==0) {
        registers[0].MSR=CAN_MSR_INAK;clear_after_reads=4;
    } else if(strcmp(scenario,"bus_isolation")==0) {
        /* Unused CAN2 never acknowledges startup; CAN1 must still be usable. */
        registers[1].MSR=CAN_MSR_INAK;
    } else assert(strcmp(scenario,"success")==0 || strcmp(scenario,"unmasked")==0);

    if(setjmp(error_return)==0) {
        CANInstance *first=Register(&hcan1,0x520);
        assert(!expect_error && first!=NULL);
        CheckStarted(&hcan1);
        assert(hcan2.State==HAL_CAN_STATE_READY && registers[1].MCR==CAN_MCR_INRQ);
        assert(notifications[1]==0);
        const unsigned notified=notifications[0];
        assert(Register(&hcan1,0x521)!=NULL);
        assert(notifications[0]==notified);

        /* Motor initialization sends commands immediately after registration. */
        for(unsigned i=0;i<8;++i) first->tx_buff[i]=(uint8_t)(10+i);
        assert(CANTransmit(first,1.0f)==1 && transmits==1);
        if(strcmp(scenario,"bus_isolation")!=0) {
            assert(Register(&hcan2,0x522)!=NULL);
            CheckStarted(&hcan2);
            assert(notifications[0]==notified);
        }
        assert(errors==0);
    } else {
        assert(expect_error && errors==1 && notifications[1]==0);
        if(strcmp(scenario,"timeout")==0 || strcmp(scenario,"wrap_timeout")==0) {
            assert(hcan1.State==HAL_CAN_STATE_ERROR);
            assert(hcan1.ErrorCode&HAL_CAN_ERROR_TIMEOUT);
            assert(cycle_reads>=10 && cycle_reads<=13); /* Bounded to roughly 10 ms. */
            assert(notifications[0]==0);
        } else if(strcmp(scenario,"no_clock")==0) {
            assert(registers[0].MCR==CAN_MCR_INRQ && notifications[0]==0);
            assert(cycle_reads<=1);
        } else if(strcmp(scenario,"not_ready")==0) {
            assert(hcan1.State==HAL_CAN_STATE_RESET);
            assert(hcan1.ErrorCode&HAL_CAN_ERROR_NOT_READY);
            assert(registers[0].MCR==CAN_MCR_INRQ && notifications[0]==0);
        } else {
            assert(strcmp(scenario,"notification_error")==0);
            assert(notifications[0]==1 && notification_mask[0]==0);
        }
    }
    assert(primask==initial_primask);
    printf("PASS: CAN startup %s\n",scenario);
    return 0;
}
