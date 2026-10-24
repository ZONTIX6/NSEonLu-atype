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
    return (ChassisPowerBudgetInput_s){.referee_limit_w=80,.buffer_control_enabled=true,
        .buffer_data_valid=true,.buffer_energy_j=60,.buffer_tick_ms=100,.input_power_valid=true,
        .measured_input_power_w=watts,.load_power_valid=true,.demand_valid=true,
        .requested_load_power_w=140,.spin_power_priority=true,.spin_feedback_valid=true,
        .spin_entry_allowed=true,.now_ms=100,.input_power_tick_ms=100};
}
static ChassisPowerBudgetOutput_s Step(ChassisPowerBudgetState_s *s,
                                      ChassisPowerBudgetInput_s *i, bool new_meter)
{
    const ChassisPowerBudgetConfig_s c=Config();
    i->now_ms+=5;
    if(new_meter) i->input_power_tick_ms=i->now_ms;
    if(i->now_ms%20==0) i->buffer_tick_ms=i->now_ms;
    i->buffer_age_s=(i->now_ms-i->buffer_tick_ms)*.001f;
    ChassisPowerBudgetOutput_s o;
    assert(ChassisPowerBudgetUpdate(&c,i,.005f,s,&o));
    return o;
}
static void Near(float a,float b) {assert(isfinite(b) && fabsf(a-b)<.02f);}
static void BaseMustBeReachedBeforeBoost(void)
{
    ChassisPowerBudgetState_s s={0};
    ChassisPowerBudgetInput_s i=Input(20);
    ChassisPowerBudgetOutput_s o={0};
    for(unsigned n=0;n<100;++n) o=Step(&s,&i,true);
    Near(80,o.applied_budget_w); /* High energy and high request must not bypass the base phase. */
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    Near(0,o.spin_buffer_grant_w);
    i.measured_input_power_w=78.4f;
    i.demand_valid=false;i.requested_load_power_w=0; /* No pre-existing excess demand required. */
    for(unsigned n=0;n<200;++n) o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST);
    assert(o.spin_buffer_request_w>0 && o.spin_buffer_grant_w>0);
    Near(78.4f,o.spin_base_power_w);
    Near(78.4f+o.spin_buffer_grant_w,o.input_target_w);
    Near(80+o.spin_buffer_grant_w,o.spin_power_ceiling_w);
    /* Budget conversion must not attenuate the physical boost a second time. */
    Near(o.input_target_w,.98f*o.applied_budget_w);
    i.measured_input_power_w=95;
    for(unsigned n=0;n<10;++n) o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST); /* Leaving base band is expected. */
}
static void RepeatedAndStaleSamplesCannotQualify(void)
{
    ChassisPowerBudgetState_s s={0};
    ChassisPowerBudgetInput_s i=Input(78.4f);
    ChassisPowerBudgetOutput_s o;
    for(unsigned n=0;n<100;++n) {
        o=Step(&s,&i,false);
        Near(0,o.spin_buffer_grant_w);
    }
    assert(o.spin_phase==CHASSIS_SPIN_POWER_DEGRADED);
    Near(0,o.spin_base_confirm_s);
}
static void DropAndFaultRevokeQualification(void)
{
    ChassisPowerBudgetState_s s={0};
    ChassisPowerBudgetInput_s i=Input(78.4f);
    ChassisPowerBudgetOutput_s o;
    for(unsigned n=0;n<60;++n) o=Step(&s,&i,true);
    assert(o.spin_buffer_grant_w>0);
    i.spin_braking=true;
    o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_DEGRADED);
    Near(0,o.spin_buffer_grant_w);
    i.spin_braking=false;
    o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    for(unsigned n=0;n<60;++n) o=Step(&s,&i,true);
    assert(o.spin_buffer_grant_w>0);
    i.referee_limit_w=40;i.measured_input_power_w=39.2f;
    o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    Near(0,o.spin_buffer_grant_w);Near(40,o.applied_budget_w);
    for(unsigned n=0;n<60;++n) o=Step(&s,&i,true);
    assert(o.spin_buffer_grant_w>0);
    i.spin_feedback_valid=false;
    o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_DEGRADED);
    Near(0,o.spin_buffer_grant_w);Near(40,o.applied_budget_w);
    i.spin_feedback_valid=true;
    o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    Near(0,o.spin_buffer_grant_w);
    i.spin_power_priority=false;i.demand_valid=false;
    o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_OFF);
    Near(0,o.spin_base_confirm_s);
}
static void LowEnergyAlwaysOverridesBasePhase(void)
{
    ChassisPowerBudgetState_s s={0};
    ChassisPowerBudgetInput_s i=Input(20);
    i.buffer_energy_j=10;
    ChassisPowerBudgetOutput_s o=Step(&s,&i,true);
    assert(o.recovering && o.spin_phase==CHASSIS_SPIN_POWER_RECOVER);
    assert(o.input_target_w<78.4f);Near(0,o.spin_buffer_grant_w);
    s=(ChassisPowerBudgetState_s){0};i=Input(150);i.buffer_energy_j=38;
    o=Step(&s,&i,true);
    assert(o.buffer_lower_j>10 && o.buffer_predicted_j<=5);
    assert(o.recovering); /* Prediction overrides even a high current estimate. */
}
static void EnergyTargetAndCapabilityBoundBoost(void)
{
    ChassisPowerBudgetState_s s={0};
    ChassisPowerBudgetInput_s i=Input(78.4f);
    ChassisPowerBudgetOutput_s o;
    for(unsigned n=0;n<60;++n) o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST);
    /* A controlled observed lower bound checks the target independently of observer correction. */
    s.buffer_lower_j=25;i.buffer_tick_ms=s.buffer_tick_ms;i.buffer_age_s=s.last_buffer_age_s;
    i.now_ms=s.buffer_tick_ms+(uint32_t)lroundf(s.last_buffer_age_s*1000);
    o=Step(&s,&i,true);
    assert(o.spin_buffer_request_w>19 && o.spin_buffer_request_w<21);
    i.spin_capability_limited=true;
    for(unsigned n=0;n<10;++n) o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST);
    for(unsigned n=0;n<20;++n) o=Step(&s,&i,true);
    assert(o.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    Near(0,o.spin_buffer_grant_w);
}
int main(void)
{
    BaseMustBeReachedBeforeBoost();
    RepeatedAndStaleSamplesCannotQualify();
    DropAndFaultRevokeQualification();
    LowEnergyAlwaysOverridesBasePhase();
    EnergyTargetAndCapabilityBoundBoost();
    puts("PASS: limit-first spin budget, samples, energy and revocation");
    return 0;
}
