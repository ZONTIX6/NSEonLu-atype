#include "chassis_spin.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ChassisSpinConfig_s Config(void)
{
    return (ChassisSpinConfig_s){.fixed_wz=6000,.min_wz=1000,.max_wz=8000,
        .gain_wz_per_w_s=80,.rise_wz_per_s=1200,.fall_wz_per_s=2400,
        .power_lpf_tau_s=.05f,.power_deadband_w=1,.settle_time_s=0,.budget_drop_reset_w=1,
        .extension_enabled=true,.hard_wz=10000,.body_hard_rps=2,.wheel_hard_rpm=12000,
        .extension_rise_wz_per_s=600};
}
static ChassisSpinInput_s Input(void)
{
    ChassisSpinInput_s i={.spin_enabled=true,.control_enabled=true,.feedback_valid=true,
        .increase_allowed=true,.evaluate_tracking=true,.extension_path=true,
        .velocity_valid=true,.target_power_w=80,.measured_power_w=50,.power_ceiling_w=82,
        .body_wz_per_wz=.0001f};
    for(unsigned w=0;w<4;++w) i.wheel_rpm_per_wz[w]=1;
    return i;
}
static ChassisSpinState_s State(void)
{
    return (ChassisSpinState_s){.initialized=true,.target_wz=8000,.applied_wz=8000,
        .filtered_power_w=50,.previous_target_power_w=80};
}
static void Sample(ChassisSpinInput_s *i,const ChassisSpinState_s *s)
{
    i->now_ms+=5;i->power_tick_ms=i->now_ms;i->velocity_tick_ms=i->now_ms;
    i->actual_body_wz_rps=s->applied_wz*i->body_wz_per_wz+i->body_wz_offset;
    for(unsigned w=0;w<4;++w) {
        ++i->feedback_sequence[w];i->feedback_tick_ms[w]=i->now_ms;
        i->actual_rpm[w]=i->target_rpm[w]=i->wheel_rpm_offset[w]+s->applied_wz*i->wheel_rpm_per_wz[w];
    }
}
static float Step(const ChassisSpinConfig_s *c,ChassisSpinInput_s *i,ChassisSpinState_s *s)
{
    float target=-1;assert(ChassisSpinUpdate(c,i,.005f,s,&target));
    assert(fabsf(s->target_wz-target)<.001f && s->applied_wz==target);
    return target;
}
static void Qualify(ChassisSpinConfig_s *c,ChassisSpinInput_s *i,ChassisSpinState_s *s)
{
    for(unsigned n=0;n<45;++n) {Sample(i,s);Step(c,i,s);}
    assert(s->extension_qualified && s->limit_state==CHASSIS_SPIN_LIMIT_EXTENDED);
}
static void HealthyExtensionAndPowerArrival(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    for(unsigned n=0;n<100;++n) {
        Sample(&i,&s);float old=s.applied_wz,target=Step(&c,&i,&s);
        assert(target-old<=3.001f);
        if(n<40) assert(target<=8000);
    }
    assert(s.extension_qualified && s.applied_wz>8000);
    i.measured_power_w=80;
    for(unsigned n=0;n<200;++n) {Sample(&i,&s);Step(&c,&i,&s);}
    assert(s.extension_qualified && s.applied_wz>8000);
    i.spin_enabled=false;Step(&c,&i,&s);assert(!s.extension_qualified && s.applied_wz==0);
    i.spin_enabled=true;Sample(&i,&s);Step(&c,&i,&s);assert(!s.extension_qualified);
}
static void RepeatedFramesAndOneWheelCannotQualify(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    Sample(&i,&s);Step(&c,&i,&s);
    for(unsigned n=0;n<100;++n) Step(&c,&i,&s);
    assert(s.extension_confirm_s==0 && !s.extension_qualified);
    for(unsigned n=0;n<70;++n) {
        Sample(&i,&s);i.feedback_sequence[2]=1;Step(&c,&i,&s);
    }
    assert(!s.extension_qualified);
    s=State();i=Input();
    for(unsigned n=0;n<70;++n) {
        Sample(&i,&s);i.actual_rpm[2]-=700;Step(&c,&i,&s);
    }
    assert(!s.extension_qualified && (s.block_reasons&CHASSIS_SPIN_BLOCK_TRACKING));
}
static void TrackingHoldAndTranslationWithdrawSmoothly(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    Qualify(&c,&i,&s);s.applied_wz=s.target_wz=9000;
    Sample(&i,&s);i.actual_rpm[1]-=600;float old=s.applied_wz;
    assert(Step(&c,&i,&s)==old);
    for(unsigned n=0;n<25;++n) {
        Sample(&i,&s);i.actual_rpm[1]-=600;old=s.applied_wz;
        assert(Step(&c,&i,&s)<=old && old-s.applied_wz<=12.001f);
    }
    assert(!s.extension_qualified && s.applied_wz<9000);
    s=State();i=Input();Qualify(&c,&i,&s);s.applied_wz=s.target_wz=9000;
    i.translation_mps=.2f;Sample(&i,&s);old=s.applied_wz;Step(&c,&i,&s);
    assert(!s.extension_qualified && s.applied_wz<old && old-s.applied_wz<=12.001f);
}
static void HardBoundsAndActualOverspeed(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    Qualify(&c,&i,&s);c.wheel_hard_rpm=8100;
    for(unsigned n=0;n<100;++n) {Sample(&i,&s);Step(&c,&i,&s);assert(s.applied_wz<=8100);}
    assert(s.block_reasons&CHASSIS_SPIN_BLOCK_WHEEL_SPEED);
    c=Config();i=Input();s=State();Qualify(&c,&i,&s);c.body_hard_rps=.81f;
    for(unsigned n=0;n<100;++n) {Sample(&i,&s);Step(&c,&i,&s);assert(s.applied_wz<=8100.01f);}
    assert(s.block_reasons&CHASSIS_SPIN_BLOCK_BODY_SPEED);
    c=Config();i=Input();s=State();Qualify(&c,&i,&s);s.applied_wz=s.target_wz=9500;
    c.hard_wz=8500;Sample(&i,&s);Step(&c,&i,&s);assert(s.applied_wz<=8500);
    c=Config();i=Input();s=State();Qualify(&c,&i,&s);Sample(&i,&s);
    i.actual_rpm[0]=13000;float old=s.applied_wz;Step(&c,&i,&s);
    assert(s.applied_wz<old && old-s.applied_wz<=12.001f);
    assert(!s.extension_qualified);
}
static void ConfigurationAndScope(void)
{
    ChassisSpinConfig_s c=Config();assert(ChassisSpinConfigIsValid(&c));
    c.hard_wz=7999;assert(!ChassisSpinConfigIsValid(&c));c=Config();
    c.body_hard_rps=NAN;assert(!ChassisSpinConfigIsValid(&c));c=Config();
    c.wheel_hard_rpm=0;assert(!ChassisSpinConfigIsValid(&c));c=Config();
    c.extension_rise_wz_per_s=1201;assert(!ChassisSpinConfigIsValid(&c));c=Config();
    ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();i.extension_path=false;
    for(unsigned n=0;n<100;++n) {Sample(&i,&s);Step(&c,&i,&s);}
    assert(!s.extension_qualified && s.applied_wz<=8000);
}
static void DisabledExtensionStillAllowsPowerDescent(void)
{
    ChassisSpinConfig_s c=Config();c.extension_enabled=false;c.hard_wz=0;
    c.body_hard_rps=c.wheel_hard_rpm=0;
    ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    s.applied_wz=s.target_wz=7000;s.filtered_power_w=100;i.measured_power_w=100;
    Sample(&i,&s);assert(Step(&c,&i,&s)<7000);
    for(unsigned n=0;n<1000;++n) {Sample(&i,&s);Step(&c,&i,&s);}
    assert(s.applied_wz>=c.min_wz);
    i.measured_power_w=50;s=State();
    Sample(&i,&s);i.actual_rpm[0]-=600;
    assert(Step(&c,&i,&s)==8000); /* One lagging sample holds before the 100 ms descent. */
}
static void PersistentLagReturnsBelowSoftToReachableDemand(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    for(unsigned n=0;n<300;++n) {
        Sample(&i,&s);i.actual_rpm[1]=6000;float old=s.applied_wz;
        Step(&c,&i,&s);assert(old-s.applied_wz<=12.001f);
    }
    assert(s.applied_wz<=6500.01f);
}
static void StartupAndNegativeWheelGeometry(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s={0};
    for(unsigned w=0;w<4;++w) i.wheel_rpm_per_wz[w]=-1;
    for(unsigned n=0;n<100;++n) {Sample(&i,&s);Step(&c,&i,&s);}
    assert(s.applied_wz>0);
    s=State();Qualify(&c,&i,&s);
    i.wheel_rpm_offset[0]=-4000;c.wheel_hard_rpm=12000;
    Sample(&i,&s);Step(&c,&i,&s);
    assert(s.applied_wz<=8000 && (s.limit_state==CHASSIS_SPIN_LIMIT_HARD));
}
static void FaultsBudgetDropAndRetuneClearQualification(void)
{
    for(unsigned fault=0;fault<5;++fault) {
        ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
        Qualify(&c,&i,&s);Sample(&i,&s);
        if(fault==0) i.feedback_valid=false;
        if(fault==1) i.braking=true;
        if(fault==2) i.energy_recovering=true;
        if(fault==3) i.target_power_w=60;
        if(fault==4) {
            float out;assert(!ChassisSpinUpdate(&c,&i,.03f,&s,&out));
        } else Step(&c,&i,&s);
        assert(!s.extension_qualified && s.extension_confirm_s==0);
        i=Input();Sample(&i,&s);Step(&c,&i,&s);assert(!s.extension_qualified);
    }
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    Qualify(&c,&i,&s);s.applied_wz=s.target_wz=9500;
    c.hard_wz=8500;assert(ChassisSpinRetune(&c,&s));Sample(&i,&s);Step(&c,&i,&s);
    assert(s.applied_wz<=8500 && !s.extension_qualified);
}
static void HardBoundaryReasonIsImmediateAndOverspeedPeaksPersist(void)
{
    ChassisSpinConfig_s c=Config();ChassisSpinInput_s i=Input();ChassisSpinState_s s=State();
    Qualify(&c,&i,&s);c.wheel_hard_rpm=s.applied_wz+1;
    Sample(&i,&s);Step(&c,&i,&s);
    assert(s.limit_state==CHASSIS_SPIN_LIMIT_HARD && (s.block_reasons&CHASSIS_SPIN_BLOCK_WHEEL_SPEED));
    c=Config();i=Input();s=State();Qualify(&c,&i,&s);Sample(&i,&s);
    i.actual_rpm[0]=-13000;i.actual_body_wz_rps=-2.1f;Step(&c,&i,&s);
    assert(s.peak_wheel_rpm==13000 && fabsf(s.peak_body_rps-2.1f)<.001f);
    Sample(&i,&s);Step(&c,&i,&s);assert(s.peak_wheel_rpm==13000);
    i.feedback_valid=false;Step(&c,&i,&s);assert(s.peak_wheel_rpm==13000);
    i.feedback_valid=true;Sample(&i,&s);Step(&c,&i,&s);
    assert(ChassisSpinRetune(&c,&s));assert(s.peak_wheel_rpm==13000);
    i.spin_enabled=false;Step(&c,&i,&s);assert(s.peak_wheel_rpm==0 && s.peak_body_rps==0);
}
int main(void)
{
    HealthyExtensionAndPowerArrival();RepeatedFramesAndOneWheelCannotQualify();
    TrackingHoldAndTranslationWithdrawSmoothly();HardBoundsAndActualOverspeed();ConfigurationAndScope();
    DisabledExtensionStillAllowsPowerDescent();
    PersistentLagReturnsBelowSoftToReachableDemand();
    StartupAndNegativeWheelGeometry();
    FaultsBudgetDropAndRetuneClearQualification();
    HardBoundaryReasonIsImmediateAndOverspeedPeaksPersist();
    puts("PASS: spin soft extension, sample qualification, tracking and hard bounds");
}
