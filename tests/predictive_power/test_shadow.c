#include "chassis_power_shadow.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_wheel_shadow(void)
{
    PPWheelKFConfig k={.version=1,.max_gap_us=20000,.max_age_us=20000,.max_horizon_us=20000,
        .q=1,.r=.25f,.initial_p00=1,.initial_p11=100,.gate_squared=25,.max_rejections=3};
    assert(ChassisPowerShadowSetWheelKFConfig(&k));
    PPWheelKFConfig changed=k;changed.q=2;
    assert(!ChassisPowerShadowSetWheelKFConfig(&changed));
    changed.version=UINT32_MAX;
    assert(!ChassisPowerShadowSetWheelKFConfig(&changed));
    uint32_t mv,kv;assert(!ChassisPowerShadowVersions(&mv,&kv));
    ChassisPowerCycleInput_s i={.observation_now_us=100000,.raw_time_valid_mask=15};
    ChassisPower_Init_Config_s c={0};
    ChassisPowerCycleOutput_s legacy={.current={12,34,56,78}},before=legacy;
    for(unsigned w=0;w<4;++w) i.raw_speed[w]=(PPWheelSample){.rx_time_us=99000,
        .sequence=1,.source_epoch=1,.omega_rad_s=10+(float)w,.online=true};
    ChassisPowerShadowCycle(&i,&c,&legacy);
    ChassisPowerFinalInput_s f={.current_command={12,34,56,78}};
    ChassisPowerShadowRecord(&f);
    ChassisPowerShadowSnapshot s;
    assert(ChassisPowerShadowGet(&s));
    assert(s.wheel_kf.configured && !s.dynamic_model);
    assert(ChassisPowerShadowVersions(&mv,&kv) && mv==0 && kv==1);
    assert(s.wheel_kf.current[0].valid && s.wheel_kf.corrected_count[0]==1);
    assert(s.wheel_kf.future_valid_mask[2]==15);
    assert(memcmp(&legacy,&before,sizeof legacy)==0);
    i.observation_now_us+=1000;
    ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));
    assert(s.wheel_kf.corrected_count[0]==1);
    PPWheelKF state[4];PPWheelKFConfig installed;
    assert(ChassisPowerShadowGetWheelKFCheckpoint(state,&installed));
    assert(state[0].corrected.corrected_time_us==99000 && installed.version==1);
    ChassisPowerShadowRecordElapsed(301);
    ChassisPowerShadowRecordElapsed(300);
    ChassisPowerShadowRecordElapsed(301);
    ChassisPowerShadowRecordElapsed(301);
    ChassisPowerShadowRecordElapsed(301);
    ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));assert(s.wheel_kf.paused);
    ChassisPowerShadowResume();
    ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));assert(!s.wheel_kf.paused);
}
int main(void)
{
    test_wheel_shadow();
    ChassisPower_Init_Config_s c={0};
    c.algorithm.max_current_command=15000;
    for(unsigned w=0;w<4;++w) c.models[w]=(MotorPowerModelConfig_s){.current_conversion=1000,.k4=1};
    ChassisPowerCycleInput_s i={.now_ms=100,.dt_s=.002f,.online_mask=15,.enabled_mask=15,
        .control={.version=1,.tick_ms=100,.control_enabled=true,.target_power_w=4}};
    ChassisPowerCycleOutput_s legacy={.valid=true,.current={1000,1000,1000,1000}},before=legacy;
    for(unsigned w=0;w<4;++w) {i.raw_current[w]=2000;i.feedback_tick_ms[w]=100;i.feedback_sequence[w]=1;}
    ChassisPowerShadowCycle(&i,&c,&legacy);
    assert(memcmp(&legacy,&before,sizeof legacy)==0);
    ChassisPowerFinalInput_s f={.now_ms=100,.control_version=1,.can_submitted=false,.enabled_mask=15,
        .current_command={1000,1000,1000,1000}};
    ChassisPowerShadowRecord(&f);
    ChassisPowerShadowSnapshot s;assert(ChassisPowerShadowGet(&s));
    assert(s.baseline_envelope && !s.dynamic_model && !s.final.submitted);
    assert(s.allocation.feasible && s.allocation.power_w<=4);
    for(unsigned w=0;w<4;++w) assert(s.final.command[w]==1000);
    c.models[0].k0=-100;
    ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));
    float actual=0;
    for(unsigned w=0;w<4;++w) actual+=fmaxf(0,PowerModelPredict(&c.models[w],s.allocation.command[w],0));
    assert(actual<=4.0001f); /* A negative wheel polynomial must not credit other wheels. */
    i.raw_current[0]=NAN;i.now_ms=102;ChassisPowerShadowCycle(&i,&c,&legacy);
    f.now_ms=102;ChassisPowerShadowRecord(&f);assert(ChassisPowerShadowGet(&s));
    assert(!s.allocation.feasible && s.reason==PP_INVALID_INPUT);
    assert(memcmp(&legacy,&before,sizeof legacy)==0);
    PPModel invalid={0};assert(!ChassisPowerShadowSetModel(&invalid));
    PPModel model={.version=1,.schema=1,.command_scale=1000,.horizon_s=.01f,
        .max_dt_s=.02f,.max_speed=1000,.max_current=30,.max_command=15000,
        .resistance=1,.power_margin_w=.1f,.speed_error=2,.current_error=.01f,
        .acceleration_bound=100};
    for(unsigned w=0;w<4;++w) {
        model.current_tau[w]=.01f;model.current_gain[w]=1;model.speed_gain[w]=10;model.speed_drag[w]=1;
        i.raw_current[w]=2000;i.feedback_tick_ms[w]=104;i.feedback_sequence[w]=2;
    }
    model.checksum=PPModelChecksum(&model);assert(ChassisPowerShadowSetModel(&model));
    assert(!ChassisPowerShadowSetModel(&model));
    i.now_ms=i.control.tick_ms=104;f.now_ms=104;
    ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));assert(s.dynamic_model && s.allocation.feasible);
    assert(memcmp(&legacy,&before,sizeof legacy)==0);
    i.control.control_enabled=false;
    ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));assert(!s.eligible && !s.allocation.feasible);
    c.buffer_enabled=true;c.budget.buffer=(ChassisBufferBudgetConfig_s){.max_energy_j=60,
        .hard_floor_j=5,.energy_margin_j=2,.recovery_enter_j=10,.max_boost_w=60};
    ChassisPowerPolicyInput_s slow={.now_ms=200,.previous_valid=true,
        .referee={.valid=true,.chassis_output_enabled=true,.chassis_power_limit_w=50},
        .meter={.online=true,.last_rx_tick_ms=200,.power_w=45},
        .previous={.cycle_valid=true,.final_valid=true,.can_submitted=true,.final_tick_ms=200}};
    for(unsigned w=0;w<4;++w) slow.previous.power.predicted_unlimited_power_w[w]=20;
    ChassisPowerPolicyOutput_s policy={.valid=true,.buffer_estimate_valid=true,
        .control={.control_enabled=true},.budget={.buffer_lower_j=40}};
    ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    ChassisPowerShadowEnergySnapshot energy;
    assert(ChassisPowerShadowEnergyGet(&energy));assert(energy.plan.valid && energy.trace.feasible);
    assert(energy.plan.created_ms==200 && energy.plan.load_w[0]>50);
    slow.now_ms=205;ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(ChassisPowerShadowEnergyGet(&energy));assert(energy.plan.created_ms==200);
    slow.zero_force=true;ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(ChassisPowerShadowEnergyGet(&energy));assert(!energy.plan.valid);
    slow.zero_force=false;slow.now_ms=210;c.use_supercap=true;
    ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(ChassisPowerShadowEnergyGet(&energy));assert(!energy.plan.valid);
    puts("PASS: shadow computes without changing legacy output; actual failed bytes retained");return 0;
}
