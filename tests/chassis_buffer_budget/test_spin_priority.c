#include "chassis_power_budget.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static ChassisPowerBudgetConfig_s Config(void)
{
    return (ChassisPowerBudgetConfig_s){.empty_energy_ratio=.1f,.full_energy_ratio=.3f,
        .max_boost_w=100,.rise_rate_w_per_s=200,.demand_margin_w=5,
        .buffer={.max_energy_j=60,.recovery_enter_j=10,.recovery_exit_j=20,.hard_floor_j=5,
            .prediction_window_s=.25f,.guard_window_s=.3f,.settlement_delay_s=.1f,
            .energy_margin_j=3,.power_margin_w=5,.max_boost_w=60,.recovery_power_w=20,
            .recovery_target_time_s=1,.observer_max_step_s=.02f,.recovery_hold_s=.1f,
            .rise_rate_w_per_s=1200,.safety_factor=.98f,.correction_rise_j_per_s=20}};
}
static ChassisPowerBudgetInput_s Input(float watts)
{
    return (ChassisPowerBudgetInput_s){.referee_limit_w=50,.buffer_control_enabled=true,
        .buffer_data_valid=true,.buffer_energy_j=60,.buffer_tick_ms=100,.input_power_valid=true,
        .measured_input_power_w=watts,.load_power_valid=true,.demand_valid=true,
        .requested_load_power_w=60,.spin_power_priority=true,.spin_feedback_valid=true,
        .spin_entry_allowed=true,.now_ms=100,.input_power_tick_ms=100};
}
static ChassisPowerBudgetOutput_s Step(ChassisPowerBudgetState_s *s,
                                      ChassisPowerBudgetInput_s *i, bool new_meter)
{
    const ChassisPowerBudgetConfig_s c=Config();
    i->now_ms+=5;
    if(new_meter) i->input_power_tick_ms=i->now_ms;
    if(new_meter) for(unsigned w=0;w<4;++w) ++i->spin_feedback_sequence[w];
    if(i->now_ms%20==0) i->buffer_tick_ms=i->now_ms;
    i->buffer_age_s=(i->now_ms-i->buffer_tick_ms)*.001f;
    ChassisPowerBudgetOutput_s o;
    assert(ChassisPowerBudgetUpdate(&c,i,.005f,s,&o));
    return o;
}
static void Near(float a,float b) {assert(isfinite(b) && fabsf(a-b)<.02f);}
static void DemandBoostDoesNotWaitForMeasuredBase(void)
{
    ChassisPowerBudgetState_s s={0};ChassisPowerBudgetInput_s i=Input(42);
    ChassisPowerBudgetOutput_s o=Step(&s,&i,true);o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST && o.spin_buffer_grant_w>0);
    Near(0,o.spin_base_confirm_s);
    for(unsigned n=0;n<60;++n) o=Step(&s,&i,true);
    Near(65,o.input_target_w);Near(65/.98f,o.applied_budget_w);
    Near(16,o.spin_buffer_request_w);Near(16,o.spin_buffer_grant_w);
    Near(o.applied_budget_w,o.spin_power_ceiling_w);
    i.requested_load_power_w=40;o=Step(&s,&i,true);
    Near(50,o.applied_budget_w);Near(0,o.spin_buffer_grant_w);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    i.requested_load_power_w=60;i.demand_valid=false;o=Step(&s,&i,true);
    Near(50,o.applied_budget_w);Near(0,o.spin_buffer_grant_w);
}
static void SpinAndOrdinaryBudgetUseTheSameDemandFormula(void)
{
    const float energy[]={12,25,60},demand[]={40,49,50,60,140},meter[]={20,42,55,100};
    for(unsigned e=0;e<3;++e) for(unsigned d=0;d<5;++d) for(unsigned m=0;m<4;++m) {
        ChassisPowerBudgetState_s spin={0},ordinary={0};
        ChassisPowerBudgetInput_s a=Input(meter[m]);a.buffer_energy_j=energy[e];
        a.requested_load_power_w=demand[d];a.spin_entry_allowed=false;
        a.spin_capability_limited=true; /* Motor limits remain downstream, as on C. */
        ChassisPowerBudgetInput_s b=a;b.spin_power_priority=false;
        for(unsigned n=0;n<60;++n) {
            ChassisPowerBudgetOutput_s x=Step(&spin,&a,true),y=Step(&ordinary,&b,true);
            Near(y.applied_budget_w,x.applied_budget_w);Near(y.input_target_w,x.input_target_w);
            Near(y.buffer_lower_j,x.buffer_lower_j);assert(x.recovering==y.recovering);
            assert(!x.spin_capability_latched && x.spin_capability_release_s==0);
        }
    }
}
static void OldCapabilityAndBaseStateCannotBlockDemand(void)
{
    ChassisPowerBudgetState_s s={.spin_capability_latched=true,.spin_base_time_s=.1f};
    ChassisPowerBudgetInput_s i=Input(42);i.spin_entry_allowed=false;i.spin_capability_limited=true;
    ChassisPowerBudgetOutput_s o=Step(&s,&i,true);o=Step(&s,&i,true);
    assert(o.spin_buffer_grant_w>0 && !o.spin_capability_latched);
    Near(0,o.spin_base_confirm_s);Near(0,o.spin_capability_release_s);
}
static void FaultsRevokeAndHealthyDemandResumesWithoutBaseWindow(void)
{
    for(unsigned fault=0;fault<7;++fault) {
        ChassisPowerBudgetState_s s={0};ChassisPowerBudgetInput_s i=Input(42);
        ChassisPowerBudgetOutput_s o={0};
        for(unsigned n=0;n<10;++n) o=Step(&s,&i,true);
        assert(o.spin_buffer_grant_w>0);
        if(fault==0) i.spin_braking=true;
        if(fault==1) i.spin_feedback_valid=false;
        if(fault==2) i.measured_input_power_w=-1;
        if(fault==3) i.input_power_valid=false;
        if(fault==4) i.buffer_data_valid=false;
        if(fault==5) i.demand_valid=false;
        if(fault==6) i.input_power_tick_ms=i.now_ms-30;
        o=Step(&s,&i,fault!=6);
        Near(50,o.applied_budget_w);Near(0,o.spin_buffer_grant_w);
        i.spin_braking=false;i.spin_feedback_valid=true;i.measured_input_power_w=42;
        i.input_power_valid=i.buffer_data_valid=i.demand_valid=true;
        for(unsigned n=0;n<2;++n) o=Step(&s,&i,true);
        assert(o.spin_buffer_grant_w>0 && o.spin_base_confirm_s==0);
    }
}
static void StaleMeterCannotKeepBoost(void)
{
    ChassisPowerBudgetState_s s={0};ChassisPowerBudgetInput_s i=Input(42);
    ChassisPowerBudgetOutput_s o={0};
    for(unsigned n=0;n<10;++n) o=Step(&s,&i,true);
    for(unsigned n=0;n<10;++n) o=Step(&s,&i,false);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_DEGRADED);
    Near(0,o.spin_buffer_grant_w);Near(0,o.spin_base_confirm_s);
}
static void LowEnergyAndPredictionStillOverrideDemand(void)
{
    ChassisPowerBudgetState_s s={0};ChassisPowerBudgetInput_s i=Input(20);
    i.buffer_energy_j=10;
    ChassisPowerBudgetOutput_s o=Step(&s,&i,true);
    assert(o.recovering && o.spin_phase==CHASSIS_SPIN_POWER_RECOVER);
    assert(o.input_target_w<49);Near(0,o.spin_buffer_grant_w);
    s=(ChassisPowerBudgetState_s){0};i=Input(120);i.buffer_energy_j=35;
    o=Step(&s,&i,true);
    assert(o.buffer_lower_j>10 && o.buffer_predicted_j<=5 && o.recovering);
}
int main(void)
{
    DemandBoostDoesNotWaitForMeasuredBase();
    SpinAndOrdinaryBudgetUseTheSameDemandFormula();
    OldCapabilityAndBaseStateCannotBlockDemand();
    FaultsRevokeAndHealthyDemandResumesWithoutBaseWindow();
    StaleMeterCannotKeepBoost();
    LowEnergyAndPredictionStillOverrideDemand();
    puts("PASS: demand-driven spin buffer, C-policy equivalence and protection");
    return 0;
}
