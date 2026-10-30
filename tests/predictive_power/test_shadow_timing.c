#include "chassis_power_shadow.h"
#include <assert.h>
#include <stdio.h>

static uint32_t clock_values[4],clock_count,clock_index,frequency=1000000;
uint32_t DWT_GetFrequencyHz(void) {return frequency;}
uint32_t DWT_GetCycleCount(void)
{
    assert(clock_index<clock_count);
    return clock_values[clock_index++];
}
static void clock_fast(uint32_t elapsed)
{
    clock_values[0]=0;clock_values[1]=0;clock_values[2]=10;clock_values[3]=elapsed;
    clock_index=0;clock_count=4;
}
static void clock_slow(uint32_t elapsed)
{
    clock_values[0]=0;clock_values[1]=elapsed;clock_index=0;clock_count=2;
}
int main(void)
{
    PPWheelKFConfig k={.version=1,.max_gap_us=20000,.max_age_us=20000,.max_horizon_us=20000,
        .q=1,.r=.25f,.initial_p00=1,.initial_p11=100,.gate_squared=25,.max_rejections=3};
    assert(ChassisPowerShadowSetWheelKFConfig(&k));
    ChassisPowerCycleInput_s i={.observation_now_us=100000,.raw_time_valid_mask=15};
    ChassisPower_Init_Config_s c={0};ChassisPowerCycleOutput_s legacy={0};
    ChassisPowerFinalInput_s f={0};ChassisPowerShadowSnapshot s;
    for(unsigned w=0;w<4;++w)i.raw_speed[w]=(PPWheelSample){.rx_time_us=100000,
        .sequence=1,.source_epoch=1,.omega_rad_s=10,.online=true};
    const uint32_t elapsed[]={299,300,301,301,301};
    for(unsigned n=0;n<5;++n) {
        clock_fast(elapsed[n]);ChassisPowerShadowCycle(&i,&c,&legacy);
        ChassisPowerShadowRecord(&f);assert(ChassisPowerShadowGet(&s));
        assert(s.elapsed_us==elapsed[n]);
        assert(s.wheel_kf.consecutive_overruns==(n<2 ? 0:n-1));
        assert(s.wheel_kf.paused==(n==4));
    }
    clock_fast(1);ChassisPowerShadowCycle(&i,&c,&legacy);ChassisPowerShadowRecord(&f);
    assert(ChassisPowerShadowGet(&s));assert(s.wheel_kf.paused && clock_index==1);
    assert(s.wheel_kf.corrected_count[0]==1 && s.wheel_kf.accepted_age_us[0]==UINT32_MAX);
    ChassisPowerShadowResume();clock_fast(299);ChassisPowerShadowCycle(&i,&c,&legacy);
    ChassisPowerShadowRecord(&f);assert(ChassisPowerShadowGet(&s));assert(!s.wheel_kf.paused);

    c.buffer_enabled=true;c.budget.buffer=(ChassisBufferBudgetConfig_s){.max_energy_j=60,
        .hard_floor_j=5,.energy_margin_j=2,.recovery_enter_j=10,.max_boost_w=60};
    ChassisPowerPolicyInput_s slow={.now_ms=200,.previous_valid=true,
        .referee={.valid=true,.chassis_output_enabled=true,.chassis_power_limit_w=50},
        .meter={.online=true,.last_rx_tick_ms=200,.power_w=45},
        .previous={.cycle_valid=true,.final_valid=true,.can_submitted=true,.final_tick_ms=200}};
    for(unsigned w=0;w<4;++w)slow.previous.power.predicted_unlimited_power_w[w]=20;
    ChassisPowerPolicyOutput_s policy={.valid=true,.buffer_estimate_valid=true,
        .control={.control_enabled=true},.budget={.buffer_lower_j=40}};
    ChassisPowerShadowEnergySnapshot energy;
    clock_slow(2000);ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(ChassisPowerShadowEnergyGet(&energy));assert(energy.plan.valid && energy.elapsed_us==2000);
    slow.now_ms=slow.meter.last_rx_tick_ms=slow.previous.final_tick_ms=210;
    clock_slow(2001);ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(ChassisPowerShadowEnergyGet(&energy));assert(!energy.plan.valid && energy.reason==PP_TIMEOUT);
    slow.now_ms=slow.meter.last_rx_tick_ms=slow.previous.final_tick_ms=220;
    clock_slow(20);ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(clock_index==0); /* Paused planning does not execute the solver. */
    ++k.version;assert(ChassisPowerShadowSetWheelKFConfig(&k));
    clock_fast(299);ChassisPowerShadowCycle(&i,&c,&legacy);
    clock_slow(20);ChassisPowerShadowEnergyUpdate(&slow,&c,&policy);
    assert(ChassisPowerShadowEnergyGet(&energy));assert(energy.plan.valid && energy.elapsed_us==20);
    puts("PASS: injected DWT validates fast/slow timeout boundaries and explicit recovery");
    return 0;
}
