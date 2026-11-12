#ifndef CHASSIS_IMU_CAN_H
#define CHASSIS_IMU_CAN_H

#include "imu_capture_config.h"
#include "chassis_imu_sample.h"

#if CHASSIS_IMU_CAN_RX_ENABLE
/* Register the DM-IMU-L1 observation endpoint before enabling IRQs. */
void ChassisImuCanInit(void);
/* Copy the latest complete group, even if stale; return receive freshness.
 * Tick/sequence are generated on complete reception, with unknown sensor delay.
 * Caller must not refresh timestamps on reads or treat these as hardware time. */
bool ChassisImuCanReadSnapshot(ChassisImuCanSnapshot_s *snapshot,uint32_t now_ms);
#endif
#endif
