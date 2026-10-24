#include "chassis_velocity.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ChassisVelocityConfig_s config = {
    .omni_x=true, .wheel_radius_m=.06f, .reduction_ratio=19,
    .wheel_base_m=.35f, .track_width_m=.30f, .direction={1,1,1,1}};
static ChassisVelocityInput_s input = {.now_ms=100, .online_mask=15,
    .feedback_tick_ms={99,98,97,96}};
static void near(float got, float want) { assert(isfinite(got) && fabsf(got-want)<.00002f); }
static void check(float a,float b,float c,float d,float x,float y,float spin)
{
    input.rpm[0]=a;input.rpm[1]=b;input.rpm[2]=c;input.rpm[3]=d;
    ChassisVelocity_s out={0};
    assert(ChassisEstimateVelocity(&config,&input,&out));
    assert(out.valid && out.tick_ms==96);
    near(out.vx_mps,x);near(out.vy_mps,y);near(out.wz_rpm,spin*60);
}
int main(void)
{
    /* Independent physical fixtures: 1900 rotor rpm = 100 wheel rpm,
       circumference .376991118 m, tangential speed .628318531 m/s. */
    check(0,0,0,0,0,0,0);
    check(-1900,1900,-1900,1900,.888576588f,0,0);
    check(1900,1900,-1900,-1900,0,.888576588f,0);
    check(1900,1900,1900,1900,0,0,.435142635f);
    check(-1900,-1900,-1900,-1900,0,0,-.435142635f);
    check(1900,5700,-1900,1900,.888576588f,.888576588f,.435142635f);
    config.direction[1]=-1;config.direction[3]=-1;
    check(-1900,-1900,-1900,-1900,.888576588f,0,0);
    config.direction[1]=config.direction[3]=1;
    config.omni_x=false;
    check(-1900,-1900,1900,1900,.628318531f,0,0);
    check(-1900,1900,-1900,1900,0,.628318531f,0);
    check(-1900,-1900,-1900,-1900,0,0,.307692308f);
    check(-5700,-1900,-1900,1900,.628318531f,.628318531f,.307692308f);
    config.omni_x=true;
    ChassisVelocity_s out={0};
    input.now_ms=116;assert(ChassisEstimateVelocity(&config,&input,&out));
    input.now_ms=117;assert(!ChassisEstimateVelocity(&config,&input,&out));
    assert(!out.valid);near(out.vx_mps,0);near(out.wz_rpm,0);
    input.now_ms=100;input.online_mask=7;
    assert(!ChassisEstimateVelocity(&config,&input,&out));
    input.online_mask=15;input.rpm[2]=NAN;
    assert(!ChassisEstimateVelocity(&config,&input,&out));
    input.rpm[2]=0;config.reduction_ratio=0;
    assert(!ChassisEstimateVelocity(&config,&input,&out));
    config.reduction_ratio=19;config.wheel_radius_m=-.06f;
    assert(!ChassisEstimateVelocity(&config,&input,&out));
    config.wheel_radius_m=.06f;config.track_width_m=INFINITY;
    assert(!ChassisEstimateVelocity(&config,&input,&out));
    config.track_width_m=.3f;config.direction[0]=0;
    assert(!ChassisEstimateVelocity(&config,&input,&out));
    config.direction[0]=1;
    input.now_ms=3;
    for(unsigned i=0;i<4;++i) input.feedback_tick_ms[i]=UINT32_MAX-1;
    assert(ChassisEstimateVelocity(&config,&input,&out));assert(out.tick_ms==UINT32_MAX-1);
    assert(!ChassisEstimateVelocity(NULL,&input,&out));assert(!out.valid);
    assert(!ChassisEstimateVelocity(&config,NULL,&out));
    assert(!ChassisEstimateVelocity(&config,&input,NULL));
    puts("PASS: physical chassis velocity");return 0;
}
