#include "chassis_power_shadow.h"
#include <string.h>
#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW
#include "chassis_power_internal.h"
#ifdef STM32F427xx
#include "bsp_dwt.h"
#endif

static PPModel model,pending_model;
static PPObserver observer;
static bool pending,published;
static ChassisPowerShadowSnapshot working,snapshot;

bool ChassisPowerShadowSetModel(const PPModel *m)
{
    if(!PPModelValid(m)) return false;
    POWER_LOCK();
    const uint32_t previous=pending ? pending_model.version : model.version;
    const uint32_t advance=m->version-previous;
    const bool accepted=advance>0 && advance<UINT32_C(0x80000000);
    if(accepted) {pending_model=*m;pending=true;}
    POWER_UNLOCK();return accepted;
}
bool ChassisPowerShadowGet(ChassisPowerShadowSnapshot *s)
{
    if(!s) return false;
    POWER_LOCK();*s=snapshot;const bool ready=published;POWER_UNLOCK();return ready;
}
static void cycle(const ChassisPowerCycleInput_s *i,const ChassisPower_Init_Config_s *c,
                  const ChassisPowerCycleOutput_s *legacy)
{
    if(!i || !c || !legacy) return;
    const uint32_t cycles=working.cycles+1,max_us=working.max_elapsed_us,overruns=working.overruns;
    working=(ChassisPowerShadowSnapshot){.tick_ms=i->now_ms,.dt_s=i->dt_s,.cycles=cycles,
        .max_elapsed_us=max_us,.overruns=overruns,.control_version=i->control.version,
        .config_version=i->control.config_version,.meter_sequence=i->meter.rx_sequence,
        .measured_w=i->meter.power_w,.reason=PP_INVALID_INPUT};
    POWER_LOCK();
    if(pending) {model=pending_model;pending=false;memset(&observer,0,sizeof observer);}
    POWER_UNLOCK();
    working.model_version=model.version;
    if(!legacy->valid || !i->control.control_enabled || i->control.zero_force ||
        !isfinite(i->dt_s) || i->dt_s<=0 || i->dt_s>.005f ||
        (uint32_t)(i->now_ms-i->control.tick_ms)>10 || !isfinite(i->control.target_power_w) ||
        i->control.target_power_w<0 || !isfinite(c->algorithm.max_current_command) ||
        c->algorithm.max_current_command<=0 || c->algorithm.max_current_command>32767) return;
    PPAllocationProblem problem={.scale=1000,.budget_w=i->control.target_power_w};
    ChassisPowerShadowEnergySnapshot energy;
    if(ChassisPowerShadowEnergyGet(&energy) && energy.plan.valid &&
        energy.config_version==i->control.config_version && (uint32_t)(i->now_ms-energy.plan.created_ms)<20) {
        working.energy_valid=true;working.energy_created_ms=energy.plan.created_ms;
        /* A shadow plan can tighten this diagnostic candidate, never increase
         * the live legacy allowance or manufacture a fresh strategy timestamp. */
        problem.budget_w=fminf(problem.budget_w,energy.plan.load_w[0]);
    }
    PPMeasurement measurement={.now_ms=i->now_ms,.model_version=model.version,.online_mask=i->online_mask};
    for(unsigned w=0;w<4;++w) {
        if(!isfinite(i->raw_current[w]) || !isfinite(i->actual_rpm[w]) || !isfinite(i->target_rpm[w]) ||
            !isfinite(i->feedback_current[w]) || (uint32_t)(i->now_ms-i->feedback_tick_ms[w])>5) return;
        const float raw=fminf(c->algorithm.max_current_command,fmaxf(-c->algorithm.max_current_command,i->raw_current[w]));
        const bool enabled=(i->online_mask&i->enabled_mask&(1U<<w))!=0;
        problem.low[w]=enabled ? fminf(0,raw) : 0;
        problem.high[w]=enabled ? fmaxf(0,raw) : 0;
        problem.q[w][w]=1;problem.linear[w]=-2*raw/problem.scale;
        measurement.current[w]=i->feedback_current[w];
        measurement.omega[w]=i->actual_rpm[w]*.10471975511965977f;
        measurement.rx_ms[w]=i->feedback_tick_ms[w];measurement.sequence[w]=i->feedback_sequence[w];
    }
    if(PPModelValid(&model) && PPObserverUpdate(&observer,&measurement)) {
        PPFinalActuation applied;
        observer.state.applied_valid=PPObserverStableCommand(&observer,i->now_ms,model.command_delay_ms,
            model.version,i->control.config_version,&applied);
        if(observer.state.applied_valid)
            for(unsigned w=0;w<4;++w) observer.state.applied_command[w]=applied.command[w];
        working.dynamic_model=PPPredict(&model,&observer.state,i->raw_current,model.horizon_s,&working.prediction);
    }
    if(working.dynamic_model) {
        problem.scale=model.command_scale;problem.power_c=working.prediction.c;
        for(unsigned w=0;w<4;++w) {
            problem.power_h[w]=working.prediction.h[w];problem.power_g[w]=working.prediction.g[w];
            /* Affine endpoint speed at frozen state. Baseline and successful
             * final command penalties keep migration behaviour explicit. */
            const float effective=fmaxf(0,model.horizon_s-model.command_delay_ms*.001f);
            const float gain=model.horizon_s*model.speed_gain[w]*(1-expf(-effective/model.current_tau[w]))*model.current_gain[w];
            const float raw=i->raw_current[w]/model.command_scale;
            const float offset=working.prediction.omega[w]-gain*raw;
            const float target=i->target_rpm[w]*.10471975511965977f;
            const float previous=observer.state.applied_valid ? observer.state.applied_command[w]/model.command_scale : 0;
            problem.q[w][w]=gain*gain+1.1f;
            problem.linear[w]=2*gain*(offset-target)-2*raw-.2f*previous;
        }
    } else {
        /* Reuse known static coefficients for comparison, never fabricate a
         * current response or claim this legacy envelope is calibrated dynamics. */
        working.baseline_envelope=true;
        for(unsigned w=0;w<4;++w) {
            const MotorPowerModelConfig_s *m=&c->models[w];
            if(!PowerModelConfigIsValid(m) || m->k4<0) return;
            const float n=fabsf(i->actual_rpm[w]),ratio=problem.scale/m->current_conversion;
            problem.power_h[w]=m->k4*ratio*ratio;
            /* Each legacy wheel clamps its polynomial at zero. Dropping its
             * negative terms gives one convex upper envelope without allowing
             * a negative wheel to subsidize another wheel's consumption. */
            problem.power_g[w]=copysignf(1,i->raw_current[w])*fmaxf(0,m->k1+m->k3*n)*ratio;
            problem.power_c+=fmaxf(0,m->k0+m->k2*n+m->k5*n*n);
        }
    }
    working.eligible=PPAllocate(&problem,&working.allocation);
    if(working.eligible && working.dynamic_model) {
        float quantized[4];PPPrediction checked;
        for(unsigned w=0;w<4;++w) quantized[w]=working.allocation.command[w];
        working.eligible=PPPredict(&model,&observer.state,quantized,model.horizon_s,&checked) &&
            checked.power_upper_w<=problem.budget_w;
        if(!working.eligible) working.allocation.feasible=false;
    }
    working.reason=working.eligible ? (working.dynamic_model ? PP_OK : PP_NO_MODEL) : PP_INFEASIBLE;
}
void ChassisPowerShadowCycle(const ChassisPowerCycleInput_s *i,const ChassisPower_Init_Config_s *c,
                            const ChassisPowerCycleOutput_s *legacy)
{
#ifdef STM32F427xx
    const uint32_t start=DWT_GetCycleCount();
#endif
    cycle(i,c,legacy);
#ifdef STM32F427xx
    const uint32_t hz=DWT_GetFrequencyHz();
    working.elapsed_us=hz ? (uint32_t)((uint64_t)(uint32_t)(DWT_GetCycleCount()-start)*1000000U/hz) : UINT32_MAX;
    if(working.elapsed_us>working.max_elapsed_us) working.max_elapsed_us=working.elapsed_us;
    if(working.elapsed_us>300) {++working.overruns;working.eligible=false;working.reason=PP_TIMEOUT;}
#endif
    working.allocation.elapsed_us=working.elapsed_us;
}
void ChassisPowerShadowRecord(const ChassisPowerFinalInput_s *f)
{
    if(!f || !working.cycles || f->control_version!=working.control_version) return;
    PPFinalActuation final={.tick_ms=f->now_ms,.control_version=f->control_version,
        .model_version=working.model_version,.config_version=working.config_version,
        .enabled_mask=f->enabled_mask,.submitted=f->can_submitted};
    memcpy(final.command,f->current_command,sizeof final.command);
    (void)PPObserverRecord(&observer,&final);working.final=final;
    POWER_LOCK();snapshot=working;published=true;POWER_UNLOCK();
}
#else
bool ChassisPowerShadowSetModel(const PPModel *m) {(void)m;return false;}
bool ChassisPowerShadowGet(ChassisPowerShadowSnapshot *s)
{if(s) memset(s,0,sizeof *s);return false;}
void ChassisPowerShadowCycle(const ChassisPowerCycleInput_s *i,const ChassisPower_Init_Config_s *c,
                            const ChassisPowerCycleOutput_s *o) {(void)i;(void)c;(void)o;}
void ChassisPowerShadowRecord(const ChassisPowerFinalInput_s *f) {(void)f;}
#endif
