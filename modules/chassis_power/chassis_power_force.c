#include "chassis_control_config.h"
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
#include "chassis_power_internal.h"
#include "chassis_power_force.h"
bool ChassisPowerForceBegin(const ChassisPowerControlSnapshot_s *control,const PowerMeterStatus_s *meter,
    uint32_t now,float dt,ChassisForceCycleInput_s *i)
{
    if(!chassis_power || !control || !meter || !i) return false;
    ChassisPowerInstance_s *p=chassis_power;
    POWER_LOCK();
    if(control->config_version==p->slow_version && p->config_version!=p->slow_version) {
        p->config=p->slow_config;p->config_version=p->slow_version;
        p->config_observation=p->slow_observation;
        ChassisPowerTransientReset(&p->transient);
    }
    POWER_UNLOCK();
    const ChassisPower_Init_Config_s *c=&p->config;
    if(!c->force_control || control->config_version!=p->config_version ||
        !isfinite(dt) || dt<=0 || dt>.005f || (uint32_t)(now-control->tick_ms)>10 ||
        !isfinite(control->nominal_budget_w) || control->nominal_budget_w<0 ||
        !isfinite(control->slow_correction_w)) return false;
    ChassisPowerTransientState_s *s=&p->transient;
    const bool fresh=c->meter_enabled && meter->online && isfinite(meter->power_w) &&
        meter->power_w>=-c->outer.error_deadband_w && (uint32_t)(now-meter->last_rx_tick_ms)<=20;
    const bool sample=fresh && (!s->meter_initialized || s->meter_tick_ms!=meter->last_rx_tick_ms);
    if(sample) {
        const float excess=meter->power_w-control->target_power_w;
        if(excess>c->outer.error_deadband_w) {
            s->fast_w=fminf(s->fast_w,-c->transient.fast_gain*excess);
            s->below_valid=false;s->below_samples=0;
        } else {
            if(!s->below_valid) {s->below_since_ms=now;s->below_valid=true;s->below_samples=0;}
            if(s->below_samples<255) ++s->below_samples;
            if(s->below_samples>=3 && (uint32_t)(now-s->below_since_ms)>=c->transient.release_confirm_ms)
                s->fast_w=fminf(0,s->fast_w+c->transient.release_w_per_s*dt);
        }
        s->meter_tick_ms=meter->last_rx_tick_ms;s->meter_initialized=true;
    }
    /* A stale meter never releases a positive allowance or resets an overload reduction. */
    if(!fresh) {s->below_valid=false;s->below_samples=0;}
    const float nominal=control->nominal_budget_w,eta=c->algorithm.safety_factor;
    s->fast_w=fmaxf(-nominal,fminf(0,s->fast_w));
    float budget=fmaxf(0,(nominal+control->slow_correction_w+s->fast_w)*eta);
    const float compensation=fresh && s->fast_w==0 ? eta*fmaxf(0,control->slow_correction_w) : 0;
    if(!fresh || s->fast_w<0) budget=fminf(budget,fmaxf(0,(nominal+s->fast_w)*eta));
    if(control->power_ceiling_w>0) budget=fminf(budget,control->power_ceiling_w+compensation);
    i->model_budget_w=budget;i->physical_target_w=control->target_power_w;
    i->meter_valid=fresh;i->meter_tick_ms=meter->last_rx_tick_ms;i->load_power_w=meter->power_w;
    i->policy_version=control->version;
    memcpy(i->models,c->models,sizeof i->models);
    p->control=*control;
    return isfinite(budget);
}
bool ChassisPowerForceRecord(const ChassisForceCycleInput_s *i,const ChassisForceCycleOutput_s *o,
    const uint32_t sequence[4])
{
    if(!chassis_power || !i || !o || !sequence) return false;
    ChassisPowerInstance_s *p=chassis_power;
    ChassisPowerSnapshot_s *s=&p->working_state;
    *s=(ChassisPowerSnapshot_s){.registered=true,.control_version=i->policy_version,
        .tick_ms=i->now_ms,.final_tick_ms=o->tick_ms,.config_version=i->config_version,
        .cycle_valid=o->valid,.final_valid=true,.can_submitted=o->can_submitted,
        .online_mask=i->online_mask,.meter_online=i->meter_valid,
        .measured_power_w=i->load_power_w,.meter_tick_ms=i->meter_tick_ms,
        .base_limited=o->limits!=0,.numeric_error=(o->faults&CHASSIS_FORCE_ERROR_NUMERIC)!=0,
        .power={.status=o->valid ? POWER_MODEL_STATUS_OK : POWER_MODEL_STATUS_NUMERIC_ERROR,
            .effective_limit_w=i->model_budget_w},
        .transient={.model_budget_w=i->model_budget_w,.effective_w=i->model_budget_w,
            .fast_w=p->transient.fast_w,.drive_mask=o->drive_mask,.brake_mask=o->brake_mask,
            .fault=o->faults!=0,.guard_scale=o->limits ? 0 : 1,.meter_fresh=i->meter_valid,
            .feedback_fresh=i->online_mask==15,.timing_valid=o->valid,
            .mode=o->faults ? CHASSIS_POWER_TRANSIENT_OFFLINE : CHASSIS_POWER_TRANSIENT_RUN}};
    for(unsigned w=0;w<4;++w) {
        s->raw_current[w]=o->raw_current[w];s->proposed_current[w]=o->allocated_current[w];
        s->final_current[w]=o->current[w];s->feedback_current[w]=i->feedback_current[w];
        s->feedback_sequence[w]=sequence[w];s->feedback_tick_ms[w]=i->feedback_tick_ms[w];
        s->power.predicted_unlimited_power_w[w]=o->raw_predicted_w[w];
        s->power.predicted_limited_power_w[w]=o->final_predicted_w[w];
        s->power.limited_current[w]=o->current[w];s->power.current_scale[w]=o->raw_current[w]!=0 ?
            fabsf(o->current[w]/o->raw_current[w]) : 0;
        s->telemetry.current_command[w]=o->current[w];s->telemetry.actual_speed_rpm[w]=i->actual_rpm[w];
        s->telemetry.predicted_power_w[w]=s->power.predicted_limited_power_w[w];
        s->telemetry.predicted_total_power_w+=s->telemetry.predicted_power_w[w];
    }
    s->telemetry.motor_enabled_mask=i->enabled_mask;
    s->cycle_control=p->control;s->control_tick_ms=p->control.tick_ms;
    s->cycle_config=p->config_observation;
    const bool driving=o->valid && !o->faults && o->can_submitted && o->drive_mask && !o->brake_mask &&
        !o->braking && i->meter_valid && i->load_power_w>=0 && i->online_mask==15 && i->enabled_mask==15;
    ChassisPowerTransientState_s *t=&p->transient;
    if(!driving) {t->run_valid=false;t->overpower_valid=false;}
    else {
        if(!t->run_valid) {t->run_since_ms=i->now_ms;t->run_valid=true;}
        const bool overpower=i->load_power_w>i->physical_target_w+p->config.outer.error_deadband_w;
        if(!overpower) t->overpower_valid=false;
        else if(!t->overpower_valid) {t->overpower_valid=true;t->overpower_since_ms=i->now_ms;}
    }
    const bool stable=driving && t->run_valid && (uint32_t)(i->now_ms-t->run_since_ms)>=p->config.transient.stable_run_ms;
    s->transient.positive_allowed=stable && t->fast_w==0 && !(o->limits&~CHASSIS_FORCE_LIMIT_ALLOCATION);
    s->transient.learning_allowed=s->transient.positive_allowed;
    s->transient.negative_learning_allowed=stable && t->overpower_valid &&
        (uint32_t)(i->now_ms-t->overpower_since_ms)>=p->config.transient.stable_run_ms;
    s->transient.negative_learning_threshold_w=i->physical_target_w+p->config.outer.error_deadband_w;
    /* Publish a whole completed cycle; no legacy speed-frame emission for force units. */
    POWER_LOCK();p->state=*s;POWER_UNLOCK();return true;
}
#endif
