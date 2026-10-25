#include "chassis_force_geometry.h"
#include "chassis_force_allocator.h"
#include "chassis_force_intent.h"
#include "chassis_force_control.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static ChassisForceConfig_s calibrated(void)
{
    ChassisForceConfig_s c=ChassisForceDefaultConfig();
    c.calibration_valid=true;
    c.force_max_n=50; c.spin_torque_nm=2;
    c.spin_start_torque_nm=.2f; c.spin_torque_max_nm=4;
    c.velocity_limit_mps=4; c.omega_limit_rad_s=12; c.wheel_limit_rpm=12000;
    c.force_rise_n_s=100; c.force_fall_n_s=200;
    c.torque_rise_nm_s=2; c.torque_fall_nm_s=4;
    c.brake_torque_nm=1; c.brake_stop_rad_s=.2f; c.brake_timeout_ms=500;
    c.stall_current_command=4000; c.stall_rpm=20; c.stall_timeout_ms=200;
    c.temp_derate_c=65; c.temp_stop_c=85;
    c.speed_prediction_rpm=100; c.startup_current_limit=1500;
    for(unsigned w=0;w<4;++w) {
        c.wheel[w].x_m=(w<2 ? .2f : -.2f);
        c.wheel[w].y_m=(w%2==0 ? .2f : -.2f);
        c.wheel[w].dx=.7071067812f;
        c.wheel[w].dy=(w==0 || w==3) ? -.7071067812f : .7071067812f;
        c.wheel[w].radius_m=.075f; c.wheel[w].reduction=19;
        c.wheel[w].k_out_nm_a=.3f;
        c.wheel[w].current_limit=10000; c.wheel[w].force_limit_n=50;
        c.wheel[w].current_slew=1000000;
    }
    c.checksum=ChassisForceConfigChecksum(&c);
    return c;
}

static void geometry(void)
{
    ChassisForceConfig_s c=calibrated(); ChassisForceGeometry_s g;
    ChassisForceConfig_s defaults=ChassisForceDefaultConfig();
    assert(!ChassisForceConfigValid(&defaults));
    defaults.calibration_valid=true;defaults.checksum=ChassisForceConfigChecksum(&defaults);
    assert(!ChassisForceConfigValid(&defaults)); /* A flag alone never substitutes for calibration. */
    assert(!ChassisForceConfigValid(&(ChassisForceConfig_s){0}));
    assert(ChassisForceGeometryInit(&c,&g));
    float request[3]={12,-7,1.3f}, current[4], reconstructed[3];
    ChassisForceToCurrent(&g,request,current);
    ChassisForceFromCurrent(&g,current,reconstructed);
    for(unsigned k=0;k<3;++k) assert(fabsf(request[k]-reconstructed[k])<.0001f);
    float rpm[4];
    for(unsigned w=0;w<4;++w) {
        float u=0; for(unsigned k=0;k<3;++k) u+=g.h[w][k]*request[k];
        rpm[w]=u/c.wheel[w].radius_m*c.wheel[w].reduction*60/(2*3.141592653589793f);
    }
    ChassisForceEstimate(&g,&c,rpm,reconstructed);
    for(unsigned k=0;k<3;++k) assert(fabsf(request[k]-reconstructed[k])<.0001f);
    c.wheel[0].k_out_nm_a=NAN;c.checksum=ChassisForceConfigChecksum(&c);
    assert(!ChassisForceGeometryInit(&c,&g));
    c=calibrated();c.wheel[0].dx=2;c.checksum=ChassisForceConfigChecksum(&c);
    assert(!ChassisForceGeometryInit(&c,&g));
}

static void protocol(void)
{
    ChassisForceIntent_s i={.command_seq=0x12345678,.motion_mode=CHASSIS_FORCE_TRANSLATE,
        .input_x=1,.force_scale=.6f,.spin_direction=1,.input_permitted=1,.source_id=CHASSIS_FORCE_SOURCE_REMOTE};
    uint8_t wire[44];ChassisForceIntent_s decoded;
    assert(ChassisForceIntentEncode(&i,wire));
    assert(wire[0]==1 && wire[2]==1 && wire[4]==0x78 && wire[7]==0x12);
    assert(wire[10]==0x80 && wire[11]==0x3f && wire[38]==0);
    assert(ChassisForceIntentDecode(wire,44,&decoded));
    assert(decoded.command_seq==i.command_seq && decoded.input_x==1);
    wire[38]=1;assert(!ChassisForceIntentDecode(wire,44,&decoded));wire[38]=0;
    assert(!ChassisForceIntentDecode(wire,43,&decoded));
    ChassisForceIntentReceiver_s r={0};
    assert(ChassisForceIntentAccept(&r,&i,UINT32_MAX-2));
    assert(!ChassisForceIntentAccept(&r,&i,1));
    assert(ChassisForceIntentFresh(&r,2,0));
    assert(!ChassisForceIntentFresh(&r,9,0));
    i.command_seq++;i.source_age_ms=UINT32_MAX;
    assert(!ChassisForceIntentAccept(&r,&i,3));
}

static void allocation(void)
{
    ChassisForceAllocationInput_s i={.budget_w=8};
    for(unsigned w=0;w<4;++w) {
        i.min_current[w]=-10000;i.max_current[w]=10000;
        i.models[w]=(MotorPowerModelConfig_s){.k0=2,.k1=-4,.k4=1,.current_conversion=1000};
        i.high[w]=4000;
    }
    ChassisForceAllocationOutput_s o;
    assert(ChassisForceAllocate(&i,&o));
    assert(o.valid && o.alpha>.99f);
    i.budget_w=0;
    assert(ChassisForceAllocate(&i,&o));
    assert(o.alpha>.85f && o.alpha<.86f); /* Largest root (4+sqrt(8))/8. */
    /* Independent dense search, including negative model regions and sign crossings. */
    for(unsigned n=0;n<40;++n) {
        i.budget_w=(float)(n%11);
        for(unsigned w=0;w<4;++w) {i.high[w]=(float)(1700+(n*173+w*733)%7000);i.low[w]=(float)(-7000+(n*359+w*977)%14000);}
        assert(ChassisForceAllocate(&i,&o));
        float best=-1, candidate[4];
        for(unsigned j=0;j<=10000;++j) {
            const float t=j*.0001f;
            for(unsigned w=0;w<4;++w) candidate[w]=o.alpha*i.high[w]+t*i.low[w];
            if(ChassisForceCandidateValid(&i,candidate)) best=t;
        }
        assert(best<0 || o.beta+0.0002f>=best);
        assert(ChassisForceCandidateValid(&i,o.current));
    }
    i.high[0]=NAN;assert(!ChassisForceAllocate(&i,&o));
    i=(ChassisForceAllocationInput_s){.budget_w=12.0799561f};
    const float high[4]={9901.2998f,9180.31934f,4639.32422f,3512.09082f};
    const float k0[4]={11.1158924f,6.46253777f,5.81265163f,3.8898387f};
    const float k1[4]={-9.8611412f,-9.35404968f,-5.77775574f,-12.1935387f};
    const float k4[4]={1.09355593f,1.26355219f,.0313176438f,1.94164717f};
    for(unsigned w=0;w<4;++w) {
        i.high[w]=high[w];i.min_current[w]=-15000;i.max_current[w]=15000;
        i.models[w]=(MotorPowerModelConfig_s){.k0=k0[w],.k1=k1[w],.k4=k4[w],.current_conversion=1000};
    }
    assert(ChassisForceAllocate(&i,&o));assert(o.alpha>.8276f);
}

static void command_and_spin(void)
{
    ChassisForceConfig_s c=calibrated();
    ChassisForceCommandState_s s={0};ChassisForceCommandOutput_s o;
    ChassisForceIntent_s i={.input_x=1,.force_scale=1,.spin_direction=1,
        .input_permitted=1,.motion_mode=CHASSIS_FORCE_TRANSLATE};
    assert(ChassisForceCommand(&c,&i,0,0,.002f,&s,&o));
    assert(fabsf(o.high[0]-.2f)<.00001f);
    i.relative_yaw_rad=1.5707963268f;
    assert(ChassisForceCommand(&c,&i,0,0,.002f,&s,&o));
    assert(fabsf(o.high[0])<.00001f && fabsf(o.high[1]-.4f)<.00001f);
    i.motion_mode=CHASSIS_FORCE_SPIN;
    assert(ChassisForceCommand(&c,&i,2,1,.002f,&s,&o));
    assert(o.low[2]>0);
    i.motion_mode=CHASSIS_FORCE_TRANSLATE;
    assert(ChassisForceCommand(&c,&i,2,2,.002f,&s,&o));
#if CHASSIS_FORCE_EXIT_MODE == CHASSIS_FORCE_EXIT_BRAKE
    assert(o.high[2]<0);
    assert(ChassisForceCommand(&c,&i,-.1f,3,.002f,&s,&o));
    assert(o.high[2]==0);
    assert(ChassisForceCommand(&c,&i,2,4,.002f,&s,&o));
    assert(o.high[2]==0);
#else
    assert(o.high[2]==0 && o.low[2]==0);
#endif
    ChassisForceSpinPowerState_s p={0};
    ChassisForceSpinPowerInput_s power={.enabled=true,.growth_allowed=true,.target_w=50,
        .load_w=40,.meter_valid=true,.now_ms=1,.meter_tick_ms=1,.last_submission_ok=true};
    float torque;
    for(unsigned k=0;k<5;++k) {
        power.now_ms=power.meter_tick_ms=1+k*5;
        assert(ChassisForceSpinPowerUpdate(&c,&power,&p,&torque));
    }
    const float before=torque;
    assert(ChassisForceSpinPowerUpdate(&c,&power,&p,&torque));assert(torque==before);
    assert(p.growth_permitted); /* A slow consumer must see a persistent permission between samples. */
    power.now_ms=power.meter_tick_ms+=5;
    assert(ChassisForceSpinPowerUpdate(&c,&power,&p,&torque));assert(torque>before);
    ChassisForceSpinPowerProject(&p,.0001f,true);assert(p.torque_nm<=.0001f);
    power.now_ms+=21;assert(ChassisForceSpinPowerUpdate(&c,&power,&p,&torque));
    assert(p.fresh_samples==0 && !p.growth_permitted);
}

static void runtime(void)
{
    ChassisForceConfig_s c=calibrated();ChassisForceController_s controller;
    assert(ChassisForceControlInit(&c,&controller));
    ChassisForceCycleInput_s i={.now_ms=1,.dt_s=.002f,.control_enabled=true,
        .config_version=1,.safety_epoch=1,.command_received_ms=1,
        .intent={.input_x=1,.force_scale=1,.spin_direction=1,.input_permitted=1,
            .motion_mode=CHASSIS_FORCE_TRANSLATE,.safety_reset=1},
        .online_mask=15,.enabled_mask=15,.model_budget_w=100,.physical_target_w=98};
    for(unsigned w=0;w<4;++w) {
        i.feedback_tick_ms[w]=1;i.models[w]=(MotorPowerModelConfig_s){.k4=1,.current_conversion=1000};
    }
    ChassisForceCycleOutput_s o;
    for(unsigned k=0;k<5;++k) {
        assert(ChassisForceControlStep(&controller,&i,&o));
        assert(ChassisForceControlRecord(&controller,o.cycle_seq,o.current,true,i.now_ms));
    }
    assert(o.state==CHASSIS_FORCE_RUN || o.state==CHASSIS_FORCE_LIMITED);
    const uint32_t cycle=o.cycle_seq;
    assert(!ChassisForceControlRecord(&controller,cycle,o.current,true,i.now_ms));
    i.actual_rpm[0]=NAN;
    assert(!ChassisForceControlStep(&controller,&i,&o));
    for(unsigned w=0;w<4;++w) assert(o.current[w]==0);
    assert(o.state==CHASSIS_FORCE_FAULT);
    i.actual_rpm[0]=0;i.intent.input_x=0;
    for(unsigned k=0;k<4;++k) {
        (void)ChassisForceControlStep(&controller,&i,&o);
        (void)ChassisForceControlRecord(&controller,o.cycle_seq,o.current,true,i.now_ms);
    }
    i.models[0].k1=NAN;
    assert(!ChassisForceControlStep(&controller,&i,&o));
    assert(o.faults&CHASSIS_FORCE_ERROR_NUMERIC);
    i.models[0].k1=0;i.control_enabled=false;
    (void)ChassisForceControlStep(&controller,&i,&o);
    const int16_t nonzero[4]={1,0,0,0};
    assert(!ChassisForceControlValidateFinal(&controller,nonzero));
}

static void adaptive_quantization(void)
{
#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE
    ChassisForceConfig_s c=calibrated();ChassisForceController_s controller;
    assert(ChassisForceControlInit(&c,&controller));
    ChassisForceCycleInput_s i={.dt_s=.002f,.control_enabled=true,.growth_allowed=true,
        .config_version=1,.safety_epoch=1,.intent={.force_scale=1,.spin_direction=1,
            .input_permitted=1,.motion_mode=CHASSIS_FORCE_SPIN},.online_mask=15,.enabled_mask=15,
        .meter_valid=true,.load_power_w=40,.physical_target_w=50,.model_budget_w=100};
    ChassisForceCycleOutput_s o;
    for(unsigned w=0;w<4;++w) i.models[w]=(MotorPowerModelConfig_s){.k4=1,.current_conversion=1000};
    for(unsigned k=0;k<1000;++k) {
        i.now_ms=1+k*2;i.command_received_ms=i.now_ms;i.meter_tick_ms=1+(k/5)*10;
        for(unsigned w=0;w<4;++w) i.feedback_tick_ms[w]=i.now_ms;
        assert(ChassisForceControlStep(&controller,&i,&o));
        assert(ChassisForceControlRecord(&controller,o.cycle_seq,o.current,true,i.now_ms));
    }
    assert(fabsf(o.torque_nm)>.1f);
    /* Reverse wheel motion must not turn electrical braking into a power-growth window. */
    const float before_braking=controller.spin.torque_nm;
    for(unsigned w=0;w<4;++w) i.actual_rpm[w]=-copysignf(100,controller.sent_current[w]);
    for(unsigned k=0;k<50;++k) {
        i.now_ms+=2;i.command_received_ms=i.now_ms;i.meter_tick_ms=i.now_ms;
        for(unsigned w=0;w<4;++w) i.feedback_tick_ms[w]=i.now_ms;
        assert(ChassisForceControlStep(&controller,&i,&o));
        assert(!o.growth_permitted);
        assert(ChassisForceControlRecord(&controller,o.cycle_seq,o.current,true,i.now_ms));
    }
    assert(controller.spin.torque_nm<=before_braking);
#endif
}

int main(void)
{
    geometry(); protocol(); allocation(); command_and_spin(); runtime(); adaptive_quantization();
    puts("force geometry/protocol/allocation/control: OK");
    return 0;
}
