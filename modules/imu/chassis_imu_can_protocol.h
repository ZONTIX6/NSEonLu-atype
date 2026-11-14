#ifndef CHASSIS_IMU_CAN_PROTOCOL_H
#define CHASSIS_IMU_CAN_PROTOCOL_H

#include "chassis_imu_sample.h"

#define CHASSIS_IMU_CAN_DATA_ID 0x520U
#define CHASSIS_IMU_CAN_FRAME_COUNT 9U
#define CHASSIS_IMU_CAN_FRAME_BYTES 8U
#define CHASSIS_IMU_CAN_PAYLOAD_BYTES 54U

/* Caller-owned storage. Only the receive context mutates it; synchronize copies
 * from another context. Rejections preserve the last complete sample's times. */
typedef struct {
    uint8_t payload[CHASSIS_IMU_CAN_PAYLOAD_BYTES];
    uint8_t next_index, transfer_tag, last_transfer_tag;
    uint16_t last_crc;
    uint32_t first_rx_tick_ms, rejected_groups;
    bool assembling, have_last;
    ChassisImuCanSnapshot_s latest;
} ChassisImuCanDecoder_s;

/* Fixed little-endian payload: version/flags/age/sequence/tick at 0/1/2/4/8,
 * float32 q/accel/gyro at 12/28/40, CRC16/CCITT-FALSE at 52 (little endian).
 * Each CAN data frame is index, transfer_tag, six consecutive payload bytes.
 * Invalid source packets retain identity/time but encode all vectors as zero. */
bool ChassisImuCanEncode(const ChassisImuCanSample_s *sample, uint8_t transfer_tag,
                        uint8_t frames[CHASSIS_IMU_CAN_FRAME_COUNT][CHASSIS_IMU_CAN_FRAME_BYTES]);
void ChassisImuCanDecoderInit(ChassisImuCanDecoder_s *decoder);
/* True only on a new complete packet. Lost, duplicate, reordered, malformed or
 * mixed fragments reject the pending group; a new index 0 starts recovery.
 * Exact repetition of the latest accepted tag+CRC never refreshes its times.
 * The caller separately bounds source age, transport delay and receive age. */
bool ChassisImuCanReceive(ChassisImuCanDecoder_s *decoder, const uint8_t *frame,
                         size_t length, uint32_t now_ms);
bool ChassisImuCanGetSnapshot(const ChassisImuCanDecoder_s *decoder,
                             ChassisImuCanSnapshot_s *snapshot);

#endif
