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
    input.feedback_allowed=false; /* Ineligible modes do not use fusion feedback. */
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

static void TestSpinFrameAndPhaseLead(void)
{
    for (int direction=-1;direction<=1;direction+=2) {
        ChassisVelocityControlTrialInput_s input=Input();
        ChassisVelocityControlTrialOutput_s output;
        input.spin_mode=true;
        input.now_ms=125U;
        input.wheel_wz_radps=direction*6.4f;
        const float angle=input.wheel_wz_radps*.025f;
        input.fused_vx_mps=.20f;
        input.reference_vx_mps=.20f*cosf(angle);
        input.reference_vy_mps=.20f*sinf(angle);
        /* Intentional phase lead must not create a false lateral correction. */
        input.requested_vx_mps=.20f*cosf(angle+direction*.16f);
        input.requested_vy_mps=.20f*sinf(angle+direction*.16f);
        assert(ChassisVelocityControlTrialApply(&config,&input,&output));
        assert(output.feedback_active);
        Near(input.requested_vx_mps,output.vx_mps);
        Near(input.requested_vy_mps,output.vy_mps);
        Near(0.0f,output.correction_vx_mps);
        Near(0.0f,output.correction_vy_mps);

        /* Stopping must oppose velocity expressed in the current axes. */
        input.requested_vx_mps=input.requested_vy_mps=0.0f;
        input.reference_vx_mps=input.reference_vy_mps=0.0f;
        input.fused_vx_mps=.12f;
        assert(ChassisVelocityControlTrialApply(&config,&input,&output));
        Near(-.03f*cosf(angle),output.correction_vx_mps);
        Near(-.03f*sinf(angle),output.correction_vy_mps);
    }
}

static void TestSpinFallbackAndBounds(void)
{
    ChassisVelocityControlTrialInput_s input=Input();
    ChassisVelocityControlTrialOutput_s output;
    input.spin_mode=true;
    input.now_ms=5U;
    input.fused_tick_ms=UINT32_MAX-19U; /* 25 ms across timestamp wrap. */
    input.wheel_wz_radps=6.4f;
    input.reference_vx_mps=.10f;
    input.fused_vx_mps=-1.0f;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(output.feedback_active);
    Near(.05f,hypotf(output.correction_vx_mps,output.correction_vy_mps));
    assert(hypotf(output.vx_mps,output.vy_mps)<=config.max_speed_mps+1e-6f);

    input.wheel_wz_radps=NAN;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(!output.feedback_active);
    Near(input.requested_vx_mps,output.vx_mps);
    Near(0.0f,output.correction_vx_mps); /* No retained correction. */
    input.wheel_wz_radps=6.4f;
    input.reference_vx_mps=NAN;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(!output.feedback_active);
    Near(0.0f,output.correction_vy_mps);

    input.reference_vx_mps=.10f;
    input.now_ms=141U;
    input.fused_tick_ms=100U;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(!output.feedback_active);
    Near(0.0f,output.correction_vx_mps);

    input=Input(); /* Non-spin behavior ignores spin-only fields. */
    input.reference_vx_mps=input.wheel_wz_radps=NAN;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(output.feedback_active);
    Near(.225f,output.vx_mps);
}

static void TestTraceRetainsCorrectionsBetweenCaptures(void)
{
    ChassisVelocityControlTrace_s trace={0};
    ChassisVelocityControlTrialInput_s input=Input();
    ChassisVelocityControlTrialOutput_s output;
    const float rpm[4]={100,-200,300,-400};
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    Near(.2f,output.reference_vx_mps);
    Near(.1f,output.feedback_vx_mps);
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,100,42,rpm,true,true);
    assert(trace.control_cycles==1 && trace.feedback_cycles==1 && trace.correction_cycles==1);
    assert(trace.flags & CHASSIS_TRIAL_CORRECTION_NONZERO);
    assert(trace.flags & CHASSIS_TRIAL_LAST_CORRECTION);
    Near(.025f,trace.last_correction_vx_mps);
    assert(trace.last_correction_control_version==42 && trace.last_correction_tick_ms==100);
    Near(-400,trace.last_correction_target_rpm[3]);

    input.fused_vx_mps=.195f; /* Active within deadband must not count as correction. */
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,105,43,rpm,true,true);
    assert(trace.control_cycles==2 && trace.feedback_cycles==2 && trace.correction_cycles==1);
    assert(!(trace.flags & CHASSIS_TRIAL_CORRECTION_NONZERO));
    input.now_ms=141;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,141,44,rpm,true,true);
    assert(trace.fused_age_ms==41 && !(trace.flags & CHASSIS_TRIAL_FRESH));
    assert(!(trace.flags & CHASSIS_TRIAL_FEEDBACK_ACTIVE));
    Near(0,trace.correction_vx_mps);
    Near(.025f,trace.last_correction_vx_mps);
    assert(trace.last_correction_control_version==42);

    input=Input();
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,150,45,rpm,false,true);
    assert(trace.correction_cycles==2); /* Generated but not published. */
    assert(trace.last_correction_control_version==42);
    assert(!(trace.flags & CHASSIS_TRIAL_PUBLISHED));
    trace.control_cycles=UINT32_MAX;
    ChassisVelocityControlTrialRecord(&trace,NULL,NULL,NULL,155,46,rpm,true,false);
    assert(trace.control_cycles==0 && trace.feedback_cycles==3 && trace.correction_cycles==2);
    assert(!(trace.flags & (CHASSIS_TRIAL_ENABLED | CHASSIS_TRIAL_FEEDBACK_ACTIVE)));
    assert(trace.flags & CHASSIS_TRIAL_LAST_CORRECTION);
    input=Input(); input.now_ms=5; input.fused_tick_ms=UINT32_MAX-15;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,5,47,rpm,true,true);
    assert(trace.fused_age_ms==21 && (trace.flags & CHASSIS_TRIAL_FRESH));
}

int main(void)
{
    TestDisabledAndIneligible();
    TestFreshFusedCorrection();
    TestFallbackKeepsSpeedLimit();
    TestVectorAndCorrectionBounds();
    TestTimestampWrapAndCommandScale();
    TestInvalidConfigurationFailsClosed();
    TestSpinFrameAndPhaseLead();
    TestSpinFallbackAndBounds();
    TestTraceRetainsCorrectionsBetweenCaptures();
    puts("PASS: bounded chassis velocity-control trial");
    return 0;
}
