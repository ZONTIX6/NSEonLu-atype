#include "chassis_imu_can.h"

#if CHASSIS_IMU_CAN_RX_ENABLE
#include "bsp_can.h"
#include "main.h"
#include "board_config.h"
#include "chassis_control_config.h"
#include "robot_def.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dm_imu_can_protocol.h"
#include "../../config/92_imu/dm_imu_config.h"

#if CHASSIS_IMU_CAN_RX_ENABLE && (!defined(ONE_BOARD) || CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED || !CHASSIS_VELOCITY_FUSION_ENABLE || CHASSIS_VELOCITY_FUSION_CALIBRATED || !VOFA_TELEMETRY_ENABLE || (VOFA_TELEMETRY_MODE != 1 && VOFA_TELEMETRY_MODE != 2))
#error CHASSIS_IMU_CAN_RX_ENABLE requires ONE_BOARD speed control, velocity observation without local gimbal calibration and measurement telemetry (VOFA mode 1 or 2).
#endif

static CANInstance *endpoint;
static DmImuCanDecoder_s decoder;

static void ReceiveIMU(CANInstance *instance)
{
    (void)DmImuCanReceive(&decoder,instance->rx_buff,instance->rx_len,HAL_GetTick());
}

bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *snapshot,uint32_t now_ms)
{
    if(!snapshot) return false;
    taskENTER_CRITICAL();
    const bool available=DmImuCanGetSnapshot(&decoder,snapshot);
    taskEXIT_CRITICAL();
    /* DM has no source timestamp. Freshness bounds receive age only. */
    return available && snapshot->sample.valid &&
        (uint32_t)(now_ms-snapshot->first_rx_tick_ms)<=60U;
}

void ChassisImuCanInit(void)
{
    if(endpoint) return;
    /* Receive only. tx_id selects the BSP FIFO; no configuration is sent. */
    CAN_Init_Config_s config={.can_handle=DM_IMU_CAN_BUS==2 ? &hcan2 : &hcan1,
        .tx_id=DM_IMU_CAN_RX_ID,.rx_id=DM_IMU_CAN_RX_ID,.can_module_callback=ReceiveIMU};
    DmImuCanDecoderInit(&decoder);
    endpoint=CANRegister(&config);
    if(endpoint) CANSetDLC(endpoint,8U);
}
#endif
