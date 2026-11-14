#include "chassis_imu_can_protocol.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t Frames[CHASSIS_IMU_CAN_FRAME_COUNT][CHASSIS_IMU_CAN_FRAME_BYTES];
static const ChassisImuCanSample_s sample = {
    .quaternion = {1, 0, 0, 0}, .accel_n_mps2 = {1.25f, -2.5f, 3.75f},
    .gyro_sensor_radps = {.125f, -.25f, .375f},
    .sequence = UINT32_C(0x12345678), .tick_ms = UINT32_C(0xfedcba98),
    .source_age_ms = 7, .valid = true,
};

static uint16_t ReferenceCrc(const uint8_t *bytes, unsigned length)
{
    uint16_t crc = UINT16_C(0xffff);
    while (length--) {
        crc ^= (uint16_t)*bytes++ << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000U) ? 0x1021U : 0U));
    }
    return crc;
}

static void Unpack(Frames frames, uint8_t payload[54])
{
    for (unsigned i = 0; i < 9; ++i) memcpy(payload + i * 6, frames[i] + 2, 6);
}

static void Repack(Frames frames, uint8_t payload[54])
{
    const uint16_t crc = ReferenceCrc(payload, 52);
    payload[52] = (uint8_t)crc; payload[53] = (uint8_t)(crc >> 8);
    for (unsigned i = 0; i < 9; ++i) memcpy(frames[i] + 2, payload + i * 6, 6);
}

static bool Feed(ChassisImuCanDecoder_s *decoder, Frames frames, uint32_t first)
{
    for (unsigned i = 0; i < 8; ++i)
        assert(!ChassisImuCanReceive(decoder, frames[i], 8, first + i));
    return ChassisImuCanReceive(decoder, frames[8], 8, first + 8);
}

static void AssertRecovered(ChassisImuCanDecoder_s *decoder, uint32_t first)
{
    Frames frames;
    assert(ChassisImuCanEncode(&sample, 42, frames));
    assert(Feed(decoder, frames, first));
    ChassisImuCanSnapshot_s out;
    assert(ChassisImuCanGetSnapshot(decoder, &out));
    assert(out.available && out.sample.valid && out.first_rx_tick_ms == first);
    assert(out.last_rx_tick_ms == first + 8);
}

static void TestRoundTripAndDuplicate(void)
{
    ChassisImuCanDecoder_s decoder;
    ChassisImuCanSnapshot_s out;
    Frames frames;
    uint8_t payload[54];
    ChassisImuCanDecoderInit(&decoder);
    assert(!ChassisImuCanGetSnapshot(&decoder, &out) && !out.available);
    assert(ReferenceCrc((const uint8_t *)"123456789", 9) == 0x29b1);
    assert(ChassisImuCanEncode(&sample, 255, frames));
    for (unsigned i = 0; i < 9; ++i) assert(frames[i][0] == i && frames[i][1] == 255);
    Unpack(frames, payload);
    const uint8_t header[12] = {1, 1, 7, 0, 0x78, 0x56, 0x34, 0x12, 0x98, 0xba, 0xdc, 0xfe};
    assert(memcmp(payload, header, sizeof header) == 0);
    assert(payload[12] == 0 && payload[13] == 0 && payload[14] == 0x80 && payload[15] == 0x3f);
    assert(payload[52] == 0x46 && payload[53] == 0xb7); /* Independently encoded fixture CRC. */
    assert(Feed(&decoder, frames, UINT32_MAX - 4U));
    assert(ChassisImuCanGetSnapshot(&decoder, &out));
    assert(out.first_rx_tick_ms == UINT32_MAX - 4U && out.last_rx_tick_ms == 3);
    assert(out.sample.sequence == 0x12345678 && out.sample.tick_ms == 0xfedcba98);
    assert(out.sample.source_age_ms == 7 && out.sample.valid && out.rejected_groups == 0);
    assert(out.sample.quaternion[0] == 1 && out.sample.quaternion[3] == 0);
    assert(out.sample.accel_n_mps2[1] == -2.5f && out.sample.gyro_sensor_radps[2] == .375f);

    assert(!Feed(&decoder, frames, 50));
    assert(ChassisImuCanGetSnapshot(&decoder, &out));
    assert(out.last_rx_tick_ms == 3 && out.rejected_groups == 1);
    assert(ChassisImuCanEncode(&sample, 0, frames));
    assert(Feed(&decoder, frames, 100)); /* Transfer tag wraps independently of source time. */
    assert(ChassisImuCanGetSnapshot(&decoder, &out) && out.last_rx_tick_ms == 108);
}

static void TestFragmentFaults(void)
{
    Frames frames;
    assert(ChassisImuCanEncode(&sample, 1, frames));
    for (unsigned fault = 0; fault < 7; ++fault) {
        ChassisImuCanDecoder_s decoder;
        ChassisImuCanSnapshot_s out;
        ChassisImuCanDecoderInit(&decoder);
        assert(!ChassisImuCanReceive(&decoder, frames[0], 8, 100));
        uint8_t changed[8]; memcpy(changed, frames[1], 8);
        switch (fault) {
        case 0: assert(!ChassisImuCanReceive(&decoder, frames[2], 8, 101)); break; /* Lost frame 1. */
        case 1:
            assert(!ChassisImuCanReceive(&decoder, frames[1], 8, 101));
            assert(!ChassisImuCanReceive(&decoder, frames[1], 8, 102)); break;
        case 2: changed[1] = 2; assert(!ChassisImuCanReceive(&decoder, changed, 8, 101)); break;
        case 3: assert(!ChassisImuCanReceive(&decoder, frames[1], 8, 111)); break;
        case 4: assert(!ChassisImuCanReceive(&decoder, frames[1], 7, 101)); break;
        case 5: changed[0] = 9; assert(!ChassisImuCanReceive(&decoder, changed, 8, 101)); break;
        default: assert(!ChassisImuCanReceive(&decoder, NULL, 8, 101)); break;
        }
        for (unsigned i = 2; i < 9; ++i)
            assert(!ChassisImuCanReceive(&decoder, frames[i], 8, 102 + i));
        assert(!ChassisImuCanGetSnapshot(&decoder, &out));
        assert(out.rejected_groups >= 1);
        AssertRecovered(&decoder, 200);
    }
    ChassisImuCanDecoder_s decoder;
    ChassisImuCanDecoderInit(&decoder);
    assert(!ChassisImuCanReceive(&decoder, frames[0], 8, 100));
    AssertRecovered(&decoder, 101); /* New index 0 replaces an unfinished group. */
    ChassisImuCanDecoderInit(&decoder);
    assert(!ChassisImuCanReceive(&decoder, frames[0], 8, 100));
    for (unsigned i = 1; i < 8; ++i)
        assert(!ChassisImuCanReceive(&decoder, frames[i], 8, 110));
    assert(ChassisImuCanReceive(&decoder, frames[8], 8, 110)); /* Inclusive deadline. */
}

static void TestPayloadFaults(void)
{
    for (unsigned fault = 0; fault < 8; ++fault) {
        ChassisImuCanDecoder_s decoder;
        ChassisImuCanSnapshot_s out;
        Frames frames;
        uint8_t payload[54];
        ChassisImuCanDecoderInit(&decoder);
        assert(ChassisImuCanEncode(&sample, 1, frames));
        Unpack(frames, payload);
        switch (fault) {
        case 0: payload[0] = 2; break;
        case 1: payload[1] = 3; break;
        case 2: payload[28] ^= 1; break; /* Finite acceleration, but keep old CRC below. */
        case 3: payload[14] = 0xc0; payload[15] = 0x7f; break; /* q NaN. */
        case 4: payload[30] = 0x80; payload[31] = 0x7f; break; /* accel +Inf. */
        case 5: payload[42] = 0x80; payload[43] = 0xff; break; /* gyro -Inf. */
        case 6: memset(payload + 12, 0, 16); break; /* Zero quaternion. */
        default: payload[15] = 0x40; break; /* q.w = 4, norm too large. */
        }
        if (fault != 2) Repack(frames, payload);
        else for (unsigned i = 0; i < 9; ++i) memcpy(frames[i] + 2, payload + 6U * i, 6);
        assert(!Feed(&decoder, frames, 100));
        assert(!ChassisImuCanGetSnapshot(&decoder, &out) && out.rejected_groups == 1);
        AssertRecovered(&decoder, 200);
    }
}

static void TestInvalidSourceAndEncoderValidation(void)
{
    ChassisImuCanDecoder_s decoder;
    ChassisImuCanSnapshot_s out;
    ChassisImuCanSample_s invalid = sample;
    Frames frames;
    uint8_t payload[54];
    invalid.quaternion[0] = NAN;
    assert(!ChassisImuCanEncode(&invalid, 1, frames));
    invalid = sample; invalid.accel_n_mps2[1] = INFINITY;
    assert(!ChassisImuCanEncode(&invalid, 1, frames));
    invalid = sample; invalid.gyro_sensor_radps[2] = NAN;
    assert(!ChassisImuCanEncode(&invalid, 1, frames));
    invalid.valid = false; invalid.source_age_ms = UINT16_MAX;
    assert(ChassisImuCanEncode(&invalid, 1, frames));
    Unpack(frames, payload);
    for (unsigned i = 12; i < 52; ++i) assert(payload[i] == 0);
    ChassisImuCanDecoderInit(&decoder);
    assert(Feed(&decoder, frames, 100));
    assert(ChassisImuCanGetSnapshot(&decoder, &out));
    assert(!out.sample.valid && out.sample.sequence == sample.sequence && out.sample.tick_ms == sample.tick_ms);
    assert(out.sample.source_age_ms == UINT16_MAX && out.sample.quaternion[0] == 0);
    invalid = sample; invalid.quaternion[0] = 0;
    assert(!ChassisImuCanEncode(&invalid, 2, frames));
    invalid.quaternion[0] = .5f; invalid.quaternion[1] = .5f;
    assert(ChassisImuCanEncode(&invalid, 2, frames)); /* Norm squared exactly .5. */
    assert(Feed(&decoder, frames, 200));
    invalid.quaternion[0] = 1; invalid.quaternion[2] = .5f;
    assert(ChassisImuCanEncode(&invalid, 3, frames)); /* Norm squared exactly 1.5. */
    assert(Feed(&decoder, frames, 300));
    assert(!ChassisImuCanEncode(NULL, 0, frames));
    assert(!ChassisImuCanEncode(&sample, 0, NULL));
    assert(!ChassisImuCanReceive(NULL, frames[0], 8, 0));
    assert(!ChassisImuCanGetSnapshot(NULL, &out));
    assert(!ChassisImuCanGetSnapshot(&decoder, NULL));
}

int main(void)
{
    TestRoundTripAndDuplicate();
    TestFragmentFaults();
    TestPayloadFaults();
    TestInvalidSourceAndEncoderValidation();
    puts("PASS: chassis IMU CAN protocol, assembly faults, source validity and wraparound");
    return 0;
}
