#include "chassis_power_internal.h"
#include "chassis_power_shadow.h"

bool ChassisPowerLimit(const ChassisPowerCycleInput_s *i, ChassisPowerCycleOutput_s *o)
{
    if (o == NULL) return false;
    memset(o,0,sizeof *o);
    if (chassis_power == NULL || i == NULL) return false;
    ChassisPowerInstance_s *p=chassis_power;
    /* Each fast cycle takes one whole version. Slow and fast own separate copies. */
    POWER_LOCK();
    if (i->control.config_version == p->slow_version && p->config_version != p->slow_version) {
        p->config=p->slow_config;
        p->config_version=p->slow_version;
    }
    const bool config_matches=i->control.config_version==p->config_version;
    POWER_UNLOCK();
    ChassisPowerSnapshot_s *state=&p->working_state;
    *state=(ChassisPowerSnapshot_s){.registered=true,.tick_ms=i->now_ms,
        .control_version=i->control.version,.control_tick_ms=i->control.tick_ms,
        .config_version=i->control.config_version,.measured_power_w=i->meter.power_w,
        .meter_online=i->meter.online,.meter_tick_ms=i->meter.last_rx_tick_ms};
    p->control=i->control;
    p->input.referee_power_limit_w=i->control.legacy_budget_w;
    ChassisPowerTransientInput_s transient={.budget_w=i->control.nominal_budget_w,
        .target_w=i->control.target_power_w,.slow_w=i->control.slow_correction_w,
        .power_ceiling_w=i->control.power_ceiling_w,.power_deadband_w=i->control.power_deadband_w,
        .meter_w=i->meter.power_w,.meter_tick_ms=i->meter.last_rx_tick_ms,.now_ms=i->now_ms,
        .meter_enabled=p->config.meter_enabled,.meter_online=i->meter.online,
        .startup_enabled=p->config.stage!=1,.control_enabled=i->control.control_enabled,
        .online_mask=i->online_mask,.enabled_mask=i->enabled_mask};
    const bool inputs_valid=config_matches && isfinite(i->dt_s) && i->dt_s>0 && i->dt_s<=.005f &&
        (uint32_t)(i->now_ms-i->control.tick_ms)<=10;
    bool valid=inputs_valid;
    for(unsigned w=0;w<4;++w) {
        p->input.desired_current[w]=state->raw_current[w]=transient.raw_current[w]=i->raw_current[w];
        p->input.speed_rpm[w]=transient.actual_rpm[w]=i->actual_rpm[w];
        p->input.speed_error_rpm[w]=i->target_rpm[w]-i->actual_rpm[w];
        transient.target_rpm[w]=i->target_rpm[w];
        transient.feedback_tick_ms[w]=state->feedback_tick_ms[w]=i->feedback_tick_ms[w];
        transient.feedback_sequence[w]=state->feedback_sequence[w]=i->feedback_sequence[w];
        state->feedback_current[w]=i->feedback_current[w];
        state->numeric_error |= !isfinite(i->raw_current[w]) || !isfinite(i->actual_rpm[w]) ||
                               !isfinite(i->target_rpm[w]) || !isfinite(i->feedback_current[w]);
    }
    ChassisPowerTransientConfig_s tc=p->config.transient;
    tc.eta=p->config.algorithm.safety_factor;
    tc.max_current_command=p->config.algorithm.max_current_command;
    valid &= ChassisPowerTransientUpdate(&tc,&transient,i->dt_s,&p->transient,&state->transient);
    if(p->config.stage>=1) p->input.referee_power_limit_w=state->transient.model_budget_w;
    const bool model_valid=PowerModelApply(p->config.models,&p->config.algorithm,&p->input,&state->power);
    const float applied_budget=p->input.referee_power_limit_w;
    ChassisPowerOutput_s proposed;
    p->input.referee_power_limit_w=state->transient.model_budget_w;
    const bool proposed_valid=PowerModelApply(p->config.models,&p->config.algorithm,&p->input,&proposed);
    p->input.referee_power_limit_w=applied_budget;
    for(unsigned w=0;w<4;++w)
        if (!(i->online_mask & i->enabled_mask & (1U<<w)) || !i->control.control_enabled)
            proposed.limited_current[w]=0;
    if(proposed_valid && !state->transient.fault && i->control.control_enabled)
        (void)ChassisPowerTransientGuard(&tc,p->config.models,&transient,proposed.limited_current,
                                        &p->transient,&state->transient);
    valid &= model_valid && !state->numeric_error && !state->transient.fault;
    bool enabled[4];
    for(unsigned w=0;w<4;++w) {
        enabled[w]=valid && (i->online_mask & i->enabled_mask & (1U<<w)) && i->control.control_enabled;
        state->base_limited |= enabled[w] && (state->transient.drive_mask & (1U<<w)) &&
            fabsf(state->power.limited_current[w])<fabsf(state->raw_current[w]);
        float chosen=(p->config.stage>=2 || i->control.power_ceiling_w>0) ?
            state->transient.guarded_current[w] : state->power.limited_current[w];
        o->current[w]=enabled[w] ? chosen : 0;
        state->power.limited_current[w]=o->current[w];
        state->proposed_current[w]=proposed_valid && enabled[w] ? state->transient.guarded_current[w] : 0;
    }
    state->online_mask=i->online_mask;
    if (model_valid) (void)PowerModelRecomputeFinal(p->config.models,&p->input,state->power.limited_current,&state->power);
    (void)ChassisPowerTelemetryBuild(&p->input,&state->power,enabled,&state->telemetry);
    for(unsigned w=0;w<4;++w) {
        state->telemetry.target_speed_rpm[w]=i->target_rpm[w];
        state->telemetry.actual_speed_rpm[w]=i->actual_rpm[w];
    }
    state->cycle_valid=o->valid=valid;
    state->transient.fault |= !inputs_valid || state->numeric_error;
    o->control_version=i->control.version;o->model=state->power;o->transient=state->transient;
#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW
    ChassisPowerShadowCycle(i,&p->config,o);
#endif
    /* Only RecordFinal publishes a complete motor cycle to the slow task. */
    return valid;
}

bool ChassisPowerRecordFinal(const ChassisPowerFinalInput_s *i)
{
    if(chassis_power==NULL || i==NULL || !chassis_power->working_state.registered ||
        chassis_power->working_state.final_valid ||
        i->control_version!=chassis_power->control.version) return false;
    ChassisPowerInstance_s *p=chassis_power;
    ChassisPowerSnapshot_s *state=&p->working_state;
    float current[4];bool enabled[4];
    float target_rpm[4],actual_rpm[4];
    memcpy(target_rpm,state->telemetry.target_speed_rpm,sizeof target_rpm);
    memcpy(actual_rpm,state->telemetry.actual_speed_rpm,sizeof actual_rpm);
    for(unsigned w=0;w<4;++w) {
        current[w]=state->final_current[w]=i->current_command[w];
        enabled[w]=(i->enabled_mask & (1U<<w))!=0;
    }
    (void)PowerModelRecomputeFinal(p->config.models,&p->input,current,&state->power);
    (void)ChassisPowerTelemetryBuild(&p->input,&state->power,enabled,&state->telemetry);
    for(unsigned w=0;w<4;++w) {
        state->telemetry.target_speed_rpm[w]=target_rpm[w];
        state->telemetry.actual_speed_rpm[w]=actual_rpm[w];
    }
    state->final_valid=true;state->final_tick_ms=i->now_ms;state->can_submitted=i->can_submitted;
    /* Failed attempts remain in diagnostics, not in the growth-limit baseline.
     * A successful submission is still not an acknowledgement of execution. */
    if (i->can_submitted) ChassisPowerTransientRecordSent(&p->transient,current);
    POWER_LOCK();p->state=*state;POWER_UNLOCK();
#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW
    ChassisPowerShadowRecord(i);
#endif
    ChassisPowerTelemetryFinal(p);
    return true;
}
