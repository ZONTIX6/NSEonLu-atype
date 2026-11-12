#include "dm_imu_can_protocol.h"
#include "../../config/92_imu/dm_imu_config.h"

#include <math.h>
#include <string.h>

static float Decode(unsigned value, float extent, unsigned maximum)
{
    return (float)value * (2.0f * extent / (float)maximum) - extent;
}

static void DecodeVector(float output[3], const uint8_t frame[8], float extent)
{
    for (unsigned i = 0; i < 3; ++i) {
        const unsigned raw = (unsigned)frame[2 + 2*i] | (unsigned)frame[3 + 2*i] << 8;
        output[i] = Decode(raw, extent, 65535U);
    }
}

static bool DecodeQuaternion(float output[4], const uint8_t frame[8])
{
    /* Manual V1.2 bit layout. Vendor example's 0xF8 loses W's lowest bit. */
    const unsigned raw[4] = {
        (unsigned)frame[1] << 6 | (unsigned)(frame[2] & 0xfcU) >> 2,
        (unsigned)(frame[2] & 3U) << 12 | (unsigned)frame[3] << 4 | (unsigned)frame[4] >> 4,
        (unsigned)(frame[4] & 15U) << 10 | (unsigned)frame[5] << 2 | (unsigned)frame[6] >> 6,
        (unsigned)(frame[6] & 63U) << 8 | (unsigned)frame[7]
    };
    float norm_squared = 0;
    for (unsigned i = 0; i < 4; ++i) {
        output[i] = Decode(raw[i], 1.0f, 16383U);
        norm_squared += output[i] * output[i];
    }
    const float norm = sqrtf(norm_squared);
    if (!(norm >= .98f && norm <= 1.02f)) return false;
    for (unsigned i = 0; i < 4; ++i) output[i] /= norm;
    return true;
}

static void NavigationAcceleration(float output[3], const float q[4], const float accel[3])
{
    /* Assumption to verify by tilt tests on the actual firmware: Hamilton q
     * maps sensor to navigation, +Z up. DM acceleration includes gravity. */
    const float w=q[0], x=q[1], y=q[2], z=q[3];
    output[0] = (1-2*(y*y+z*z))*accel[0] + 2*(x*y-w*z)*accel[1] + 2*(x*z+w*y)*accel[2];
    output[1] = 2*(x*y+w*z)*accel[0] + (1-2*(x*x+z*z))*accel[1] + 2*(y*z-w*x)*accel[2];
    output[2] = 2*(x*z-w*y)*accel[0] + 2*(y*z+w*x)*accel[1] + (1-2*(x*x+y*y))*accel[2] - DM_IMU_GRAVITY_MPS2;
}

void DmImuCanDecoderInit(DmImuCanDecoder_s *decoder)
{
    if (decoder) *decoder = (DmImuCanDecoder_s){0};
}

static bool Reject(DmImuCanDecoder_s *decoder)
{
    decoder->next_type = 0;
    if (decoder->rejected_groups != UINT32_MAX) ++decoder->rejected_groups;
    return false;
}

bool DmImuCanReceive(DmImuCanDecoder_s *decoder, const uint8_t *frame,
                     size_t length, uint32_t now_ms)
{
    if (!decoder) return false;
    if (!frame || length != 8U || frame[0] < 1U || frame[0] > 4U)
        return Reject(decoder);
    if (frame[0] == 1U) {
        if (decoder->next_type) (void)Reject(decoder);
        DecodeVector(decoder->accel_sensor_mps2, frame, 235.2f);
        decoder->first_rx_tick_ms = now_ms;
        decoder->next_type = 2;
        return false;
    }
    /* Once a group has failed, continuations cannot reuse any partial vector. */
    if (!decoder->next_type) return false;
    if ((uint32_t)(now_ms - decoder->first_rx_tick_ms) > 10U)
        return Reject(decoder);
    if (frame[0] == 3U && decoder->next_type == 4U) {
        decoder->next_type = 5; /* Only one optional Euler frame. */
        return false;
    }
    if (frame[0] != decoder->next_type && !(frame[0] == 4U && decoder->next_type == 5U))
        return Reject(decoder);
    if (frame[0] == 2U) {
        DecodeVector(decoder->gyro_sensor_radps, frame, 34.88f);
        decoder->next_type = 4;
        return false;
    }
    decoder->next_type = 0;
    ChassisImuCanSample_s sample = {0};
    if (!DecodeQuaternion(sample.quaternion, frame)) return Reject(decoder);
    /* Validate every complete group. With 100 Hz input, 18 ms selects about
     * every other group while allowing +/-1 ms jitter at each endpoint.
     * A strict 20 ms gate can skip a 19 ms group and create a >30 ms gap,
     * which resets the downstream 15..30 ms center/fusion pipeline. */
    if (decoder->latest.available &&
        (uint32_t)(decoder->first_rx_tick_ms - decoder->latest.first_rx_tick_ms) < 18U)
        return false;
    sample.sequence = decoder->latest.sample.sequence + 1U;
    sample.tick_ms = decoder->first_rx_tick_ms;
    sample.valid = true;
    memcpy(sample.gyro_sensor_radps, decoder->gyro_sensor_radps, sizeof sample.gyro_sensor_radps);
    NavigationAcceleration(sample.accel_n_mps2, sample.quaternion, decoder->accel_sensor_mps2);
    decoder->latest = (ChassisImuCanSnapshot_s){.sample = sample,
        .first_rx_tick_ms = decoder->first_rx_tick_ms, .last_rx_tick_ms = now_ms,
        .rejected_groups = decoder->rejected_groups, .available = true};
    return true;
}

bool DmImuCanGetSnapshot(const DmImuCanDecoder_s *decoder, ChassisImuCanSnapshot_s *snapshot)
{
    if (!decoder || !snapshot) return false;
    *snapshot = decoder->latest;
    snapshot->rejected_groups = decoder->rejected_groups;
    return snapshot->available;
}
