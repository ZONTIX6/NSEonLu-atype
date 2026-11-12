#ifndef DM_IMU_CAN_PROTOCOL_H
#define DM_IMU_CAN_PROTOCOL_H

#include "chassis_imu_sample.h"

/* Caller-owned; receive context owns mutations. Synchronize snapshot copies.
 * DM supplies neither source timestamps nor group tags: local receive time is
 * only an estimate, and the ordered assembly window cannot prove simultaneity. */
typedef struct {
    float accel_sensor_mps2[3], gyro_sensor_radps[3];
    uint32_t first_rx_tick_ms, rejected_groups;
    uint8_t next_type;
    ChassisImuCanSnapshot_s latest;
} DmImuCanDecoder_s;

void DmImuCanDecoderInit(DmImuCanDecoder_s *decoder);
/* Accept 1(accel), 2(gyro), optional 3(Euler), 4(quaternion), within 10ms.
 * True only when publishing a complete valid group. Published groups' first
 * receive times are at least 18ms apart: nominal 50Hz from 100Hz input, with
 * +/-1ms receive jitter tolerated without skipping an extra group.
 * sample.tick_ms and both rx times use the receiver clock; source_age_ms=0 is
 * unknown internal sensor delay, not proof of a zero-age source measurement. */
bool DmImuCanReceive(DmImuCanDecoder_s *decoder, const uint8_t *frame, size_t length, uint32_t now_ms);
/* Copies the latest complete publication without refreshing its timestamps. */
bool DmImuCanGetSnapshot(const DmImuCanDecoder_s *decoder, ChassisImuCanSnapshot_s *snapshot);

#endif
