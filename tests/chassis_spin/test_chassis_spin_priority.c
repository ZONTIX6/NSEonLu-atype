#include "chassis_spin.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const ChassisSpinConfig_s c={.fixed_wz=6000,.min_wz=1000,.max_wz=8000,
    .gain_wz_per_w_s=80,.rise_wz_per_s=1200,.fall_wz_per_s=2400,.power_lpf_tau_s=.05f,
    .power_deadband_w=1,.settle_time_s=.05f,.budget_drop_reset_w=1};
static ChassisSpinInput_s Input(void)
{
    return (ChassisSpinInput_s){.spin_enabled=true,.control_enabled=true,.feedback_valid=true,
        .increase_allowed=true,.target_power_w=78.4f,.measured_power_w=10,.evaluate_tracking=true};
}
static void Feedback(ChassisSpinInput_s *i,unsigned n,float actual,float target)
{
    i->now_ms=100+n*5;
    for(unsigned w=0;w<4;++w) {
        i->feedback_tick_ms[w]=i->now_ms;i->feedback_sequence[w]=n+1;
        i->actual_rpm[w]=actual;i->target_rpm[w]=target;
    }
}
static void EntryRampsAndInvalidFeedbackStops(void)
{
    ChassisSpinState_s s={0};ChassisSpinInput_s i=Input();float target=0,previous=0;
    for(unsigned n=0;n<50;++n) {
        Feedback(&i,n,0,0);
        assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));
        assert(target>=previous && target-previous<=6.01f);
        previous=target;
    }
    assert(target>0 && target<1000);
    i.feedback_valid=false;
    assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));assert(target==0);
    i.feedback_valid=true;i.control_enabled=false;
    assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));assert(target==0);
}
static void TrendHasLimitedRateAndCannotRenewOnOldSamples(void)
{
    ChassisSpinConfig_s fast=c;fast.settle_time_s=0;
    ChassisSpinState_s s={0};ChassisSpinInput_s i=Input();float target=0,previous=0;
    for(unsigned n=0;n<18;++n) {
        Feedback(&i,n,500+n*10,2000+n*10);
        assert(ChassisSpinUpdate(&fast,&i,.005f,&s,&target));
        assert(target-previous<=1.51f); /* 25% of 1200/s. */
        previous=target;
    }
    assert(target>0);
    for(unsigned n=18;n<50;++n) {
        Feedback(&i,n,670,2170); /* No further acceleration or error improvement. */
        assert(ChassisSpinUpdate(&fast,&i,.005f,&s,&target));
    }
    assert(s.block_reasons & CHASSIS_SPIN_BLOCK_TRACKING);
    previous=target;
    for(unsigned n=0;n<100;++n) {
        i.now_ms+=5; /* Reusing this frame cannot manufacture a new trend. */
        assert(ChassisSpinUpdate(&fast,&i,.005f,&s,&target));
    }
    assert(target<=previous);
}
static void HardLimitsAndModelLimitHaveDistinctReasons(void)
{
    ChassisSpinState_s s={0};ChassisSpinInput_s i=Input();float target;
    i.model_limited=true;
    for(unsigned n=0;n<50;++n) {
        Feedback(&i,n,0,0);assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));
    }
    assert(target==0 && (s.block_reasons & CHASSIS_SPIN_BLOCK_MODEL));
    i.model_limited=false;i.current_saturated=true;
    Feedback(&i,51,0,0);assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));
    assert(target==0 && (s.block_reasons & CHASSIS_SPIN_BLOCK_CURRENT));
    i.current_saturated=false;i.guard_limited=true;
    Feedback(&i,52,0,0);assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));
    assert(target==0 && (s.block_reasons & CHASSIS_SPIN_BLOCK_GUARD));
    i.guard_limited=false;i.meter_stale=true;
    Feedback(&i,53,0,0);assert(ChassisSpinUpdate(&c,&i,.005f,&s,&target));
    assert(target==0 && (s.block_reasons & CHASSIS_SPIN_BLOCK_METER));
}
static void HealthyIdleCanEstablishDemand(void)
{
    ChassisPowerTransientOutput_s t={.meter_fresh=true,.feedback_fresh=true,.timing_valid=true,
        .mode=CHASSIS_POWER_TRANSIENT_IDLE};
    assert(ChassisSpinTransientAllowsIncrease(&t));
    t.brake_mask=1;assert(!ChassisSpinTransientAllowsIncrease(&t));
    t.brake_mask=0;t.meter_fresh=false;assert(!ChassisSpinTransientAllowsIncrease(&t));
}
int main(void)
{
    EntryRampsAndInvalidFeedbackStops();
    TrendHasLimitedRateAndCannotRenewOnOldSamples();
    HardLimitsAndModelLimitHaveDistinctReasons();
    HealthyIdleCanEstablishDemand();
    puts("PASS: spin entry ramp, bounded feedback trend and growth reasons");return 0;
}
