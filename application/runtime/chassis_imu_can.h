#ifndef CHASSIS_IMU_CAN_H
#define CHASSIS_IMU_CAN_H

#include "imu_capture_config.h"
#include "chassis_imu_can_protocol.h"

#if IMU_CAPTURE_CAN_TX_ENABLE || CHASSIS_IMU_CAN_RX_ENABLE
/* Register one diagnostic-only CAN1 / 0x520 endpoint before enabling IRQs. */
void ChassisImuCanInit(void);
#endif
#if IMU_CAPTURE_CAN_TX_ENABLE
/* Low-priority task only; bounded per-frame wait and at most one group / 20 ms. */
void ChassisImuCanSend(uint32_t now_ms);
uint32_t ChassisImuCanTxDropCount(void);
#endif
#if CHASSIS_IMU_CAN_RX_ENABLE
/* Always copies the latest complete group and reject count, including stale or
 * source-invalid data. Return value adds local freshness to source validity.
 * Source tick belongs to the second board and is never subtracted from now_ms. */
bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *snapshot,uint32_t now_ms);
#endif

#endif
