#include "dm_imu_can_protocol.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define GRAVITY 9.80665f
typedef uint8_t Frames[3][8];

/* Only test stimulus construction. The bit-layout regression below uses a
 * hand-authored fixture independent of this packer. */
static unsigned Map(float v, float extent, unsigned max)
{
    return (unsigned)lroundf((v + extent) * (float)max / (2.0f * extent));
}

static void Make(Frames frames, const float q[4], const float accel[3])
{
    memset(frames, 0, sizeof(Frames));
    frames[0][0] = 1; frames[1][0] = 2; frames[2][0] = 4;
    for (unsigned i = 0; i < 3; ++i) {
        const unsigned a = Map(accel[i], 235.2f, 65535);
        const unsigned g = Map((float)i - 1.0f, 34.88f, 65535);
        frames[0][2 + 2*i] = (uint8_t)a;
        frames[0][3 + 2*i] = (uint8_t)(a >> 8);
        frames[1][2 + 2*i] = (uint8_t)g;
        frames[1][3 + 2*i] = (uint8_t)(g >> 8);
    }
    unsigned u[4];
    for (unsigned i = 0; i < 4; ++i) u[i] = Map(q[i], 1.0f, 16383);
    frames[2][1] = (uint8_t)(u[0] >> 6);
    frames[2][2] = (uint8_t)((u[0] << 2) | (u[1] >> 12));
    frames[2][3] = (uint8_t)(u[1] >> 4);
    frames[2][4] = (uint8_t)((u[1] << 4) | (u[2] >> 10));
    frames[2][5] = (uint8_t)(u[2] >> 2);
    frames[2][6] = (uint8_t)((u[2] << 6) | (u[3] >> 8));
    frames[2][7] = (uint8_t)u[3];
}

static bool Feed(DmImuCanDecoder_s *decoder, Frames frames, uint32_t first)
{
    assert(!DmImuCanReceive(decoder, frames[0], 8, first));
    assert(!DmImuCanReceive(decoder, frames[1], 8, first + 1U));
    return DmImuCanReceive(decoder, frames[2], 8, first + 2U);
}

static void Near(float actual, float expected, float tolerance)
{
    assert(isfinite(actual) && fabsf(actual - expected) <= tolerance);
}

static void TestFixtureAndNavigationAcceleration(void)
{
    DmImuCanDecoder_s decoder;
    ChassisImuCanSnapshot_s out;
    Frames frames;
    const float identity[4] = {1, 0, 0, 0}, still[3] = {0, 0, GRAVITY};
    DmImuCanDecoderInit(&decoder);
    assert(!DmImuCanGetSnapshot(&decoder, &out) && !out.available);
    Make(frames, identity, still);
    assert(Feed(&decoder, frames, 100));
    assert(DmImuCanGetSnapshot(&decoder, &out));
    assert(out.available && out.sample.valid && out.sample.sequence == 1);
    assert(out.sample.tick_ms == 100 && out.first_rx_tick_ms == 100 && out.last_rx_tick_ms == 102);
    assert(out.sample.source_age_ms == 0 && out.rejected_groups == 0);
    for (unsigned i = 0; i < 3; ++i) {
        Near(out.sample.accel_n_mps2[i], 0, .015f);
        Near(out.sample.gyro_sensor_radps[i], (float)i - 1.0f, .001f);
    }

    /* W=0x3333, X=0x3999, Y=Z=0x2000. DATA[2] bit2 is W bit0;
     * the vendor's 0xF8 mask loses it, unlike the protocol's 0xFC. */
    const uint8_t fixture[8] = {4, 0xcc, 0xcf, 0x99, 0x98, 0x00, 0x20, 0x00};
    memcpy(frames[2], fixture, 8);
    assert(Feed(&decoder, frames, 120));
    assert(DmImuCanGetSnapshot(&decoder, &out));
    const float w = 2.0f * 13107.0f / 16383.0f - 1.0f;
    const float x = 2.0f * 14745.0f / 16383.0f - 1.0f;
    const float zero = 1.0f / 16383.0f;
    const float norm = sqrtf(w*w + x*x + 2*zero*zero);
    Near(out.sample.quaternion[0], w/norm, 1e-6f);
    Near(out.sample.quaternion[1], x/norm, 1e-6f);

    const float roll90[4] = {.70710678f, .70710678f, 0, 0};
    const float tilted_still[3] = {0, GRAVITY, 0};
    Make(frames, roll90, tilted_still);
    assert(Feed(&decoder, frames, 140));
    assert(DmImuCanGetSnapshot(&decoder, &out));
    for (unsigned i = 0; i < 3; ++i) Near(out.sample.accel_n_mps2[i], 0, .015f);

    const float yaw90[4] = {.70710678f, 0, 0, .70710678f};
    const float moving[3] = {2, 0, GRAVITY};
    Make(frames, yaw90, moving);
    assert(Feed(&decoder, frames, 160));
    assert(DmImuCanGetSnapshot(&decoder, &out));
    Near(out.sample.accel_n_mps2[0], 0, .015f);
    Near(out.sample.accel_n_mps2[1], 2, .015f);
    Near(out.sample.accel_n_mps2[2], 0, .015f);
}

static void TestFaultsAndRecovery(void)
{
    const float q[4] = {1, 0, 0, 0}, a[3] = {0, 0, GRAVITY};
    Frames frames;
    Make(frames, q, a);
    for (unsigned fault = 0; fault < 8; ++fault) {
        DmImuCanDecoder_s decoder;
        ChassisImuCanSnapshot_s out;
        DmImuCanDecoderInit(&decoder);
        assert(Feed(&decoder, frames, 50));
        assert(!DmImuCanReceive(&decoder, frames[0], 8, 100));
        uint8_t bad[8] = {0};
        switch (fault) {
        case 0: assert(!DmImuCanReceive(&decoder, frames[2], 8, 101)); break;
        case 1: assert(!DmImuCanReceive(&decoder, frames[1], 7, 101)); break;
        case 2: assert(!DmImuCanReceive(&decoder, NULL, 8, 101)); break;
        case 3: assert(!DmImuCanReceive(&decoder, frames[1], 8, 111)); break;
        case 4:
            assert(!DmImuCanReceive(&decoder, frames[1], 8, 101));
            assert(!DmImuCanReceive(&decoder, frames[1], 8, 102)); break;
        case 5:
            assert(!DmImuCanReceive(&decoder, frames[1], 8, 101));
            bad[0] = 4; /* (-1,-1,-1,-1): norm 2. */
            assert(!DmImuCanReceive(&decoder, bad, 8, 102)); break;
        case 6: assert(!DmImuCanReceive(&decoder, bad, 8, 101)); break;
        default: assert(!DmImuCanReceive(&decoder, frames[1], 8, 99)); break;
        }
        assert(!DmImuCanReceive(&decoder, frames[2], 8, 112));
        assert(DmImuCanGetSnapshot(&decoder, &out));
        assert(out.sample.sequence == 1 && out.first_rx_tick_ms == 50 && out.last_rx_tick_ms == 52);
        assert(out.rejected_groups >= 1);
        assert(Feed(&decoder, frames, 200));
        assert(DmImuCanGetSnapshot(&decoder, &out) && out.sample.sequence == 2);
    }
    DmImuCanDecoder_s decoder;
    ChassisImuCanSnapshot_s out;
    DmImuCanDecoderInit(&decoder);
    assert(!DmImuCanReceive(&decoder, frames[0], 8, 10));
    assert(Feed(&decoder, frames, 20)); /* A new acceleration restarts an incomplete group. */
    assert(DmImuCanGetSnapshot(&decoder, &out) && out.rejected_groups == 1);
    assert(!DmImuCanReceive(&decoder, frames[0], 8, 40));
    assert(!DmImuCanReceive(&decoder, frames[1], 8, 41));
    const uint8_t euler[8] = {3};
    assert(!DmImuCanReceive(&decoder, euler, 8, 42));
    assert(DmImuCanReceive(&decoder, frames[2], 8, 50)); /* Inclusive 10ms. */
    assert(!DmImuCanReceive(NULL, frames[0], 8, 1));
    assert(!DmImuCanGetSnapshot(NULL, &out));
    assert(!DmImuCanGetSnapshot(&decoder, NULL));
}

static void TestPublicationSpacing(void)
{
    const float q[4] = {1, 0, 0, 0}, a[3] = {0, 0, GRAVITY};
    Frames frames;
    Make(frames, q, a);
    const struct { uint32_t gap; bool publish; } cases[] = {
        {14, false}, {15, false}, {17, false}, {18, true}, {19, true}, {20, true}
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        DmImuCanDecoder_s decoder;
        ChassisImuCanSnapshot_s out;
        DmImuCanDecoderInit(&decoder);
        assert(Feed(&decoder, frames, 100));
        assert(Feed(&decoder, frames, 100 + cases[i].gap) == cases[i].publish);
        assert(DmImuCanGetSnapshot(&decoder, &out));
        assert(out.sample.sequence == (cases[i].publish ? 2U : 1U));
        assert(out.first_rx_tick_ms == (cases[i].publish ? 100 + cases[i].gap : 100U));
        assert(out.last_rx_tick_ms == out.first_rx_tick_ms + 2U);
        assert(out.rejected_groups == 0);
    }
}

static void TestRateLimitAndWrap(void)
{
    const float q[4] = {1, 0, 0, 0}, a[3] = {0, 0, GRAVITY};
    Frames frames;
    Make(frames, q, a);
    DmImuCanDecoder_s decoder;
    ChassisImuCanSnapshot_s out;
    DmImuCanDecoderInit(&decoder);
    assert(Feed(&decoder, frames, UINT32_MAX - 10U));
    assert(!Feed(&decoder, frames, UINT32_MAX));
    assert(DmImuCanGetSnapshot(&decoder, &out));
    assert(out.sample.sequence == 1 && out.first_rx_tick_ms == UINT32_MAX - 10U);
    assert(out.last_rx_tick_ms == UINT32_MAX - 8U && out.rejected_groups == 0);
    assert(Feed(&decoder, frames, 7)); /* Inclusive 18 ms across clock wrap. */
    assert(DmImuCanGetSnapshot(&decoder, &out));
    assert(out.sample.sequence == 2 && out.sample.tick_ms == 7 && out.last_rx_tick_ms == 9);
    assert(!Feed(&decoder, frames, 24));
    assert(Feed(&decoder, frames, 27));
    assert(Feed(&decoder, frames, 5000)); /* A long silence needs no decoder reinit. */
    assert(DmImuCanGetSnapshot(&decoder, &out) && out.sample.sequence == 4);
}

int main(void)
{
    TestFixtureAndNavigationAcceleration();
    TestFaultsAndRecovery();
    TestPublicationSpacing();
    TestRateLimitAndWrap();
    puts("PASS: DM IMU CAN bit layout, gravity, faults, rate limit and clock wrap");
    return 0;
}
