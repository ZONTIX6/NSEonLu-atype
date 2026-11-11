#include "chassis_velocity_fusion.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static const ChassisVelocityFusionConfig_s config = {
    .wheel_variance = .01f,
    .accel_variance = .04f,
    .jerk_density = .25f,
    .max_velocity_residual_mps = .5f,
    .max_age_us = 150000,
    .max_skew_us = 110000,
    .max_gap_us = 150000,
};

static ChassisVelocityFusionInput_s Sample(uint32_t time_us)
{
    return (ChassisVelocityFusionInput_s){
        .now_us = time_us, .wheel_us = time_us, .imu_us = time_us,
        .wheel_valid = true, .imu_valid = true,
    };
}

static void Near(float actual, float expected)
{
    assert(isfinite(actual));
    assert(fabsf(actual - expected) < .00001f);
}

static void AccelerationChangesBothVelocityAxes(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(1000000);
    sample.accel_n_mps2[0] = 1;
    sample.accel_n_mps2[1] = -2;
    ChassisVelocityFusionOutput_s out;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    sample.now_us = sample.imu_us = 1100000;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused && out.sample_us == 1100000);
    /* A 100 ms interval under [1, -2] m/s² changes velocity by [.1, -.2] m/s. */
    Near(out.velocity_n_mps[0], .1f);
    Near(out.velocity_n_mps[1], -.2f);

    sample = Sample(1200000);
    sample.accel_n_mps2[0] = 1;
    sample.accel_n_mps2[1] = -2;
    sample.wheel_n_mps[0] = .12f;
    sample.wheel_n_mps[1] = -.24f;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    /* The new wheel fix pulls the inertial prediction toward measured velocity. */
    assert(out.velocity_n_mps[0] > .12f && out.velocity_n_mps[0] < .20f);
    assert(out.velocity_n_mps[1] < -.24f && out.velocity_n_mps[1] > -.40f);
}

static void RepeatedSamplesHaveNoEffect(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(1000);
    sample.wheel_n_mps[0] = .25f;
    ChassisVelocityFusionOutput_s out;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    ChassisVelocityFusion_s reference = state;
    for (unsigned i = 0; i < 5; ++i) {
        sample.now_us += 1000;
        ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
        assert(out.valid && out.fused && out.sample_us == 1000);
        Near(out.velocity_n_mps[0], .25f);
    }
    sample = Sample(20000);
    sample.wheel_n_mps[0] = .4f;
    ChassisVelocityFusionOutput_s expected;
    ChassisVelocityFusionUpdate(&reference, &config, &sample, &expected);
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    Near(out.velocity_n_mps[0], expected.velocity_n_mps[0]);
}

static void EarlierImuSampleCorrectsBeforeWheelSample(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(1000000);
    sample.wheel_n_mps[0] = .2f;
    sample.accel_n_mps2[0] = 1;
    ChassisVelocityFusionOutput_s out;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);

    sample = Sample(1080000);
    sample.imu_us = 1040000;
    sample.accel_n_mps2[0] = 2;
    /* At 40 ms, P_va=.0018 and S=.09 so the new acceleration gives
       v=.26 and a=1.5555556. At 80 ms that predicts v=.3222222. */
    sample.wheel_n_mps[0] = .322222222f;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused && out.sample_us == 1080000);
    Near(out.velocity_n_mps[0], .322222222f);
}

static void MissingOrConflictingImuFallsBackToWheels(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(1000000);
    sample.imu_valid = false;
    sample.wheel_n_mps[1] = .25f;
    ChassisVelocityFusionOutput_s out;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused && out.sample_us == 1000000);
    Near(out.velocity_n_mps[1], .25f);

    sample = Sample(1010000);
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    sample = Sample(1020000);
    sample.wheel_n_mps[0] = 1;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused && out.sample_us == 1020000);
    Near(out.velocity_n_mps[0], 1);
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused);

    sample = Sample(1030000);
    sample.wheel_n_mps[0] = 1;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    sample = Sample(1040000);
    sample.accel_n_mps2[0] = NAN;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused);
}

static void StaleOrSkewedSamplesAreRejected(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(1000000);
    ChassisVelocityFusionOutput_s out;
    sample.imu_us -= 120000;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused);
    sample.imu_us += 120000;
    sample.wheel_us++;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(!out.valid && !out.fused);
    sample = Sample(1001000);
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    sample.wheel_us--;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(!out.valid);
    sample = Sample(1201000);
    sample.imu_us = 1001000;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused);
    sample.wheel_us = 1001000;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(!out.valid);
}

static void WheelLossAndTimingGapClearHistory(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(1000000);
    sample.wheel_n_mps[0] = .6f;
    ChassisVelocityFusionOutput_s out;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    sample = Sample(1160000);
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && !out.fused);
    Near(out.velocity_n_mps[0], 0);
    sample = Sample(1170000);
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    sample.wheel_valid = false;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(!out.valid && !out.fused);
    sample = Sample(1180000);
    sample.wheel_n_mps[0] = -.4f;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused);
    Near(out.velocity_n_mps[0], -.4f);
}

static void ClockWrapPreservesIntegration(void)
{
    ChassisVelocityFusion_s state;
    ChassisVelocityFusionReset(&state);
    ChassisVelocityFusionInput_s sample = Sample(UINT32_MAX - 49999);
    sample.accel_n_mps2[0] = 2;
    ChassisVelocityFusionOutput_s out;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    sample.imu_us = sample.now_us = 50000;
    ChassisVelocityFusionUpdate(&state, &config, &sample, &out);
    assert(out.valid && out.fused && out.sample_us == 50000);
    Near(out.velocity_n_mps[0], .2f);
}

int main(void)
{
    AccelerationChangesBothVelocityAxes();
    RepeatedSamplesHaveNoEffect();
    EarlierImuSampleCorrectsBeforeWheelSample();
    MissingOrConflictingImuFallsBackToWheels();
    StaleOrSkewedSamplesAreRejected();
    WheelLossAndTimingGapClearHistory();
    ClockWrapPreservesIntegration();
    puts("PASS: chassis planar velocity fusion");
    return 0;
}
