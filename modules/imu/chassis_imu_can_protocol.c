#include "chassis_imu_can_protocol.h"

#include <float.h>
#include <math.h>
#include <string.h>

_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
               "IMU CAN requires IEEE 754 float32");

static uint16_t Crc16(const uint8_t *bytes, size_t length)
{
    uint16_t crc = UINT16_C(0xffff);
    while (length--) {
        crc ^= (uint16_t)*bytes++ << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000U) ? 0x1021U : 0U));
    }
    return crc;
}

static void WriteU32(uint8_t *bytes, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (8U * i));
}

static uint32_t ReadU32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static void WriteVector(uint8_t *bytes, const float *values, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        uint32_t bits;
        memcpy(&bits, &values[i], sizeof bits);
        WriteU32(bytes + 4U * i, bits);
    }
}

static void ReadVector(float *values, const uint8_t *bytes, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        const uint32_t bits = ReadU32(bytes + 4U * i);
        memcpy(&values[i], &bits, sizeof bits);
    }
}

static bool ValidVectors(const ChassisImuCanSample_s *sample)
{
    float norm_squared = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (!isfinite(sample->quaternion[i])) return false;
        norm_squared += sample->quaternion[i] * sample->quaternion[i];
    }
    if (!(norm_squared >= .5f && norm_squared <= 1.5f)) return false;
    for (unsigned i = 0; i < 3; ++i)
        if (!isfinite(sample->accel_n_mps2[i]) || !isfinite(sample->gyro_sensor_radps[i]))
            return false;
    return true;
}

bool ChassisImuCanEncode(const ChassisImuCanSample_s *sample, uint8_t transfer_tag,
                        uint8_t frames[CHASSIS_IMU_CAN_FRAME_COUNT][CHASSIS_IMU_CAN_FRAME_BYTES])
{
    if (!sample || !frames || (sample->valid && !ValidVectors(sample))) return false;
    uint8_t payload[CHASSIS_IMU_CAN_PAYLOAD_BYTES] = {1, sample->valid ? 1 : 0};
    payload[2] = (uint8_t)sample->source_age_ms;
    payload[3] = (uint8_t)(sample->source_age_ms >> 8);
    WriteU32(payload + 4, sample->sequence);
    WriteU32(payload + 8, sample->tick_ms);
    if (sample->valid) {
        WriteVector(payload + 12, sample->quaternion, 4);
        WriteVector(payload + 28, sample->accel_n_mps2, 3);
        WriteVector(payload + 40, sample->gyro_sensor_radps, 3);
    }
    const uint16_t crc = Crc16(payload, 52);
    payload[52] = (uint8_t)crc; payload[53] = (uint8_t)(crc >> 8);
    for (unsigned i = 0; i < CHASSIS_IMU_CAN_FRAME_COUNT; ++i) {
        frames[i][0] = (uint8_t)i;
        frames[i][1] = transfer_tag;
        memcpy(frames[i] + 2, payload + 6U * i, 6);
    }
    return true;
}

void ChassisImuCanDecoderInit(ChassisImuCanDecoder_s *decoder)
{
    if (decoder) *decoder = (ChassisImuCanDecoder_s){0};
}

static bool Reject(ChassisImuCanDecoder_s *decoder)
{
    decoder->assembling = false;
    decoder->next_index = 0;
    if (decoder->rejected_groups != UINT32_MAX) ++decoder->rejected_groups;
    return false;
}

bool ChassisImuCanReceive(ChassisImuCanDecoder_s *decoder, const uint8_t *frame,
                         size_t length, uint32_t now_ms)
{
    if (!decoder) return false;
    if (!frame || length != CHASSIS_IMU_CAN_FRAME_BYTES || frame[0] >= CHASSIS_IMU_CAN_FRAME_COUNT)
        return Reject(decoder);
    if (frame[0] == 0) {
        if (decoder->assembling) (void)Reject(decoder);
        decoder->assembling = true;
        decoder->next_index = 0;
        decoder->transfer_tag = frame[1];
        decoder->first_rx_tick_ms = now_ms;
    }
    /* Ignore continuations after a rejected/missing start, without counting
     * the same discarded group once for every subsequent fragment. */
    if (!decoder->assembling) return false;
    if (frame[0] != decoder->next_index || frame[1] != decoder->transfer_tag ||
        (uint32_t)(now_ms - decoder->first_rx_tick_ms) > CHASSIS_IMU_CAN_ASSEMBLY_MAX_MS)
        return Reject(decoder);
    memcpy(decoder->payload + 6U * decoder->next_index, frame + 2, 6);
    if (++decoder->next_index != CHASSIS_IMU_CAN_FRAME_COUNT) return false;
    decoder->assembling = false;
    decoder->next_index = 0;

    const uint8_t *payload = decoder->payload;
    const uint16_t crc = (uint16_t)((uint16_t)payload[52] | (uint16_t)payload[53] << 8);
    if (payload[0] != 1 || (payload[1] & ~1U) || Crc16(payload, 52) != crc ||
        (decoder->have_last && decoder->last_transfer_tag == decoder->transfer_tag && decoder->last_crc == crc))
        return Reject(decoder);
    ChassisImuCanSample_s sample = {
        .sequence = ReadU32(payload + 4), .tick_ms = ReadU32(payload + 8),
        .source_age_ms = (uint16_t)((uint16_t)payload[2] | (uint16_t)payload[3] << 8),
        .valid = (payload[1] & 1U) != 0,
    };
    if (sample.valid) {
        ReadVector(sample.quaternion, payload + 12, 4);
        ReadVector(sample.accel_n_mps2, payload + 28, 3);
        ReadVector(sample.gyro_sensor_radps, payload + 40, 3);
        if (!ValidVectors(&sample)) return Reject(decoder);
    }
    decoder->latest = (ChassisImuCanSnapshot_s){.sample = sample,
        .first_rx_tick_ms = decoder->first_rx_tick_ms, .last_rx_tick_ms = now_ms,
        .rejected_groups = decoder->rejected_groups, .available = true};
    decoder->last_transfer_tag = decoder->transfer_tag;
    decoder->last_crc = crc;
    decoder->have_last = true;
    return true;
}

bool ChassisImuCanGetSnapshot(const ChassisImuCanDecoder_s *decoder,
                             ChassisImuCanSnapshot_s *snapshot)
{
    if (!decoder || !snapshot) return false;
    *snapshot = decoder->latest;
    snapshot->rejected_groups = decoder->rejected_groups;
    return snapshot->available;
}
