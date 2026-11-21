#include "../../application/chassis/chassis_velocity_control_trial.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static const ChassisVelocityControlTrialConfig_s config={
    .max_speed_mps=.30f,
    .proportional_gain=.25f,
    .max_correction_mps=.05f,
    .deadband_mps=.02f,
    .max_sample_age_ms=40U,
};

static ChassisVelocityControlTrialInput_s Input(void)
{
    return (ChassisVelocityControlTrialInput_s){
        .enabled=true,
        .speed_limit_allowed=true,
        .feedback_allowed=true,
        .fused_snapshot_valid=true,
        .fused_status=true,
        .wheel_valid=true,
        .now_ms=100U,
        .fused_tick_ms=100U,
        .requested_vx_mps=.20f,
        .requested_vy_mps=0.0f,
        .fused_vx_mps=.10f,
        .fused_vy_mps=0.0f,
    };
}

static void Near(float expected, float actual)
{
    assert(isfinite(actual));
    assert(fabsf(expected-actual)<1e-5f);
}

static void TestDisabledAndIneligible(void)
{
    ChassisVelocityControlTrialOutput_s output;
    ChassisVelocityControlTrialInput_s input=Input();
    input.enabled=false;
    input.requested_vx_mps=.50f;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.50f,output.vx_mps);
    assert(!output.command_limited && !output.feedback_active);

    input=Input();
    input.feedback_allowed=false; /* Spin and other non-test modes do not use fusion feedback. */
    input.requested_vx_mps=.40f;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,output.vx_mps);
    assert(output.command_limited && !output.feedback_active);
}

static void TestFreshFusedCorrection(void)
{
    ChassisVelocityControlTrialOutput_s output;
    ChassisVelocityControlTrialInput_s input=Input();
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.225f,output.vx_mps);
    Near(.025f,output.correction_vx_mps);
    Near(0.0f,output.vy_mps);
    assert(output.command_limited && output.feedback_active);

    input.fused_vx_mps=.195f; /* Inside the vector deadband. */
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.20f,output.vx_mps);
    Near(0.0f,output.correction_vx_mps);
    assert(output.feedback_active);
}

static void TestFallbackKeepsSpeedLimit(void)
{
    ChassisVelocityControlTrialOutput_s output;
    ChassisVelocityControlTrialInput_s input=Input();
    input.requested_vx_mps=.40f;
    input.fused_snapshot_valid=false;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,output.vx_mps);
    assert(output.command_limited && !output.feedback_active);

    input=Input(); input.requested_vx_mps=.40f; input.fused_status=false;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,output.vx_mps);
    assert(!output.feedback_active);

    input=Input(); input.requested_vx_mps=.40f; input.wheel_valid=false;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,output.vx_mps);
    assert(!output.feedback_active);

    input=Input(); input.requested_vx_mps=.40f; input.now_ms=141U; input.fused_tick_ms=100U;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,output.vx_mps);
    assert(!output.feedback_active); /* 41 ms is over the trial limit. */
}

static void TestVectorAndCorrectionBounds(void)
{
    ChassisVelocityControlTrialOutput_s output;
    ChassisVelocityControlTrialInput_s input=Input();
    input.feedback_allowed=false;
    input.requested_vx_mps=.30f;
    input.requested_vy_mps=.30f;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,hypotf(output.vx_mps,output.vy_mps));

    input=Input();
    input.requested_vx_mps=.10f;
    input.fused_vx_mps=-1.0f;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.05f,hypotf(output.correction_vx_mps,output.correction_vy_mps));

    input=Input();
    input.requested_vx_mps=.29f;
    input.fused_vx_mps=0.0f;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.30f,hypotf(output.vx_mps,output.vy_mps)); /* Final vector cap includes correction. */
}

static void TestTimestampWrapAndCommandScale(void)
{
    ChassisVelocityControlTrialOutput_s output;
    ChassisVelocityControlTrialInput_s input=Input();
    input.now_ms=5U;
    input.fused_tick_ms=UINT32_MAX-15U; /* Unsigned age is 21 ms across wrap. */
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(output.feedback_active);

    float scale=0.0f;
    assert(ChassisVelocityControlTrialCommandScale(.076f,19.0f,&scale));
    Near(.076f*6.283185307179586f/(360.0f*19.0f),scale);
    assert(!ChassisVelocityControlTrialCommandScale(.076f,0.0f,&scale));
}

static void TestInvalidConfigurationFailsClosed(void)
{
    ChassisVelocityControlTrialConfig_s invalid=config;
    ChassisVelocityControlTrialInput_s input=Input();
    ChassisVelocityControlTrialOutput_s output;
    invalid.proportional_gain=2.0f;
    assert(!ChassisVelocityControlTrialApply(&invalid,&input,&output));
    Near(0.0f,output.vx_mps);
    Near(0.0f,output.vy_mps);
    assert(output.command_limited && !output.feedback_active);
}

int main(void)
{
    TestDisabledAndIneligible();
    TestFreshFusedCorrection();
    TestFallbackKeepsSpeedLimit();
    TestVectorAndCorrectionBounds();
    TestTimestampWrapAndCommandScale();
    TestInvalidConfigurationFailsClosed();
    puts("PASS: bounded chassis velocity-control trial");
    return 0;
}
