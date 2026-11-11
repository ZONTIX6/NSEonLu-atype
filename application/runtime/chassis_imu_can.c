#include "chassis_imu_can.h"

#if IMU_CAPTURE_CAN_TX_ENABLE || CHASSIS_IMU_CAN_RX_ENABLE
#include "bsp_can.h"
#include "main.h"
#include "board_config.h"
#include "chassis_control_config.h"
#include "robot_def.h"
#include <string.h>

#if CHASSIS_IMU_CAN_RX_ENABLE && (!defined(ONE_BOARD) || CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED || !CHASSIS_VELOCITY_FUSION_ENABLE || CHASSIS_VELOCITY_FUSION_CALIBRATED || !VOFA_TELEMETRY_ENABLE || VOFA_TELEMETRY_MODE != 1)
#error CHASSIS_IMU_CAN_RX_ENABLE requires ONE_BOARD speed control, velocity observation without local gimbal calibration and FireWater.
#endif

static CANInstance *endpoint;

#if CHASSIS_IMU_CAN_RX_ENABLE
#include "FreeRTOS.h"
#include "task.h"
static ChassisImuCanDecoder_s decoder;

static void ReceiveIMU(CANInstance *instance)
{
    (void)ChassisImuCanReceive(&decoder,instance->rx_buff,instance->rx_len,HAL_GetTick());
}

bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *snapshot,uint32_t now_ms)
{
    if(!snapshot) return false;
    taskENTER_CRITICAL();
    const bool available=ChassisImuCanGetSnapshot(&decoder,snapshot);
    taskEXIT_CRITICAL();
    const uint32_t local_age=now_ms-snapshot->first_rx_tick_ms;
    return available && snapshot->sample.valid && snapshot->sample.source_age_ms<=20U &&
        local_age<=60U && snapshot->sample.source_age_ms<=60U-local_age;
}
#endif

void ChassisImuCanInit(void)
{
    if(endpoint) return;
    CAN_Init_Config_s config={.can_handle=&hcan1,.tx_id=CHASSIS_IMU_CAN_DATA_ID,
        .rx_id=CHASSIS_IMU_CAN_DATA_ID};
#if CHASSIS_IMU_CAN_RX_ENABLE
    ChassisImuCanDecoderInit(&decoder);
    config.can_module_callback=ReceiveIMU;
#endif
    /* TX has no receive callback: this endpoint never decodes its own traffic. */
    endpoint=CANRegister(&config);
    if(endpoint) CANSetDLC(endpoint,CHASSIS_IMU_CAN_FRAME_BYTES);
}

#if IMU_CAPTURE_CAN_TX_ENABLE
#include "ins_task.h"
static volatile uint32_t tx_drops;
static uint32_t last_attempt_ms,owned_mailboxes;
static uint8_t next_tag;
static bool attempted,group_queued;

uint32_t ChassisImuCanTxDropCount(void) { return tx_drops; }

static uint32_t PendingOwned(void)
{
    uint32_t pending=0;
    for(uint32_t mailbox=1;mailbox<=4;mailbox<<=1)
        if((owned_mailboxes&mailbox) && HAL_CAN_IsTxMessagePending(endpoint->can_handle,mailbox))
            pending|=mailbox;
    return pending;
}

static void AbortOwned(void)
{
    const uint32_t pending=PendingOwned();
    if(pending) (void)HAL_CAN_AbortTxRequest(endpoint->can_handle,pending);
    owned_mailboxes=PendingOwned();
}

void ChassisImuCanSend(uint32_t now_ms)
{
    if(!endpoint || (attempted && (uint32_t)(now_ms-last_attempt_ms)<20U)) return;
    attempted=true;last_attempt_ms=now_ms;
    const uint8_t tag=next_tag++;

    /* Enqueue success is not bus delivery. Cancel this module's old pending
     * frames before starting another tag; an incomplete abort skips this turn. */
    if(PendingOwned()) {
        if(group_queued) ++tx_drops;
        group_queued=false;
        AbortOwned();
        if(owned_mailboxes) { ++tx_drops;return; }
    }
    owned_mailboxes=0;group_queued=false;

    INSMotionSnapshot_s motion={0};
    (void)INSGetMotionSnapshot(&motion);
    const uint32_t started_ms=HAL_GetTick();
    const uint32_t source_age=started_ms-motion.tick_ms;
    ChassisImuCanSample_s sample={.sequence=motion.sequence,.tick_ms=motion.tick_ms,
        .source_age_ms=source_age<UINT16_MAX ? (uint16_t)source_age : UINT16_MAX,
        .valid=motion.valid && source_age<=20U};
    memcpy(sample.quaternion,motion.quaternion,sizeof sample.quaternion);
    memcpy(sample.accel_n_mps2,motion.accel_n_mps2,sizeof sample.accel_n_mps2);
    memcpy(sample.gyro_sensor_radps,motion.gyro_sensor_radps,sizeof sample.gyro_sensor_radps);
    uint8_t frames[CHASSIS_IMU_CAN_FRAME_COUNT][CHASSIS_IMU_CAN_FRAME_BYTES];
    if(!ChassisImuCanEncode(&sample,tag,frames)) { ++tx_drops;return; }
    for(unsigned i=0;i<CHASSIS_IMU_CAN_FRAME_COUNT;++i) {
        memcpy(endpoint->tx_buff,frames[i],CHASSIS_IMU_CAN_FRAME_BYTES);
        if(!CANTransmit(endpoint,1.0f)) { ++tx_drops;AbortOwned();return; }
        owned_mailboxes|=endpoint->tx_mailbox;
        if((uint32_t)(HAL_GetTick()-started_ms)>CHASSIS_IMU_CAN_ASSEMBLY_MAX_MS) {
            ++tx_drops;AbortOwned();return;
        }
    }
    group_queued=true;
}
#endif
#endif
