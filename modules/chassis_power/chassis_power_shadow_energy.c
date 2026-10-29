#include "chassis_power_shadow.h"
#include <string.h>
#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW
#include "chassis_power_internal.h"
#ifdef STM32F427xx
#include "bsp_dwt.h"
#endif
static ChassisPowerShadowEnergySnapshot energy;
static uint32_t last_attempt,version;
static bool attempted;
bool ChassisPowerShadowEnergyGet(ChassisPowerShadowEnergySnapshot *s)
{
    if(!s) return false;
    POWER_LOCK();*s=energy;POWER_UNLOCK();return true;
}
void ChassisPowerShadowEnergyUpdate(const ChassisPowerPolicyInput_s *i,const ChassisPower_Init_Config_s *c,
                                    const ChassisPowerPolicyOutput_s *policy)
{
    if(!i || !c || !policy) return;
    ChassisPowerShadowEnergySnapshot result={.config_version=policy->control.config_version,.reason=PP_NO_PERMISSION};
    const bool permitted=policy->valid && policy->control.control_enabled && !i->zero_force &&
        i->referee.valid && i->referee.chassis_output_enabled;
    const bool fresh=i->meter.online && isfinite(i->meter.power_w) && i->meter.power_w>=0 &&
        (uint32_t)(i->now_ms-i->meter.last_rx_tick_ms)<=20 && i->previous_valid &&
        i->previous.cycle_valid && i->previous.final_valid && i->previous.can_submitted &&
        (uint32_t)(i->now_ms-i->previous.final_tick_ms)<=5;
    /* Legacy CAN carries no calibrated joules or strong command confirmation.
     * A capacitor plan is deliberately unavailable until those inputs exist. */
    if(!permitted || !fresh || c->use_supercap || !c->buffer_enabled || !policy->buffer_estimate_valid) {
        result.reason=permitted ? PP_NO_MODEL : PP_NO_PERMISSION;
        POWER_LOCK();energy=result;POWER_UNLOCK();attempted=false;return;
    }
    if(attempted && (uint32_t)(i->now_ms-last_attempt)<10 &&
        policy->control.config_version==energy.config_version) return;
    attempted=true;last_attempt=i->now_ms;
#ifdef STM32F427xx
    const uint32_t start=DWT_GetCycleCount();
#endif
    const ChassisBufferBudgetConfig_s *b=&c->budget.buffer;
    const PPEnergyConfig config={.buffer_max_j=b->max_energy_j,
        .buffer_floor_j=b->hard_floor_j+b->energy_margin_j,
        .buffer_terminal_j=fmaxf(b->recovery_enter_j,b->hard_floor_j+b->energy_margin_j),
        .input_max_w=i->referee.chassis_power_limit_w+b->max_boost_w,
        .load_max_w=i->referee.chassis_power_limit_w+b->max_boost_w};
    PPEnergyInput input={.now_ms=i->now_ms,.strategy_version=++version,.model_version=1,.capability_version=1,
        .permitted=true,.buffer_valid=true,.buffer_low_j=policy->budget.buffer_lower_j,
        .buffer_high_j=b->max_energy_j,.referee_w=i->referee.chassis_power_limit_w};
    for(unsigned w=0;w<4;++w) {
        const float demand=i->previous.power.predicted_unlimited_power_w[w];
        if(!isfinite(demand) || demand<0) {input.buffer_valid=false;break;}
        input.demand_w+=demand;
    }
    input.demand_w=fmaxf(input.demand_w,i->meter.power_w);
    result.plan.valid=PPEnergySchedule(&config,&input,&result.plan) &&
        PPEnergyCheck(&config,&input,&result.plan,&result.trace);
    result.reason=result.plan.valid ? PP_OK : PP_INFEASIBLE;
#ifdef STM32F427xx
    const uint32_t hz=DWT_GetFrequencyHz();
    result.elapsed_us=hz ? (uint32_t)((uint64_t)(uint32_t)(DWT_GetCycleCount()-start)*1000000U/hz) : UINT32_MAX;
    if(result.elapsed_us>2000) {result.plan.valid=false;result.reason=PP_TIMEOUT;}
#endif
    POWER_LOCK();energy=result;POWER_UNLOCK();
}
#else
bool ChassisPowerShadowEnergyGet(ChassisPowerShadowEnergySnapshot *s)
{if(s) memset(s,0,sizeof *s);return false;}
void ChassisPowerShadowEnergyUpdate(const ChassisPowerPolicyInput_s *i,const ChassisPower_Init_Config_s *c,
                                    const ChassisPowerPolicyOutput_s *p) {(void)i;(void)c;(void)p;}
#endif
