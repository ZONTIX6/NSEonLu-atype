#include "chassis_power_internal.h"
#include "chassis_power_policy.h"

bool ChassisPowerUpdatePolicy(const ChassisPowerPolicyInput_s *i, ChassisPowerPolicyOutput_s *o)
{
    if (o==NULL) return false;
    memset(o,0,sizeof *o);
    if (chassis_power==NULL || i==NULL) return false;
    ChassisPowerInstance_s *p=chassis_power;
    POWER_LOCK();
    const bool changed=p->config_pending;
    if (changed) {
        p->slow_config=p->pending_config;
        p->slow_version=p->pending_version;
        p->config_pending=false;
    }
    POWER_UNLOCK();
    if(changed) {
        ChassisPowerOuterLoopReset(&p->outer_state);
        p->budget_state.spin_sample_valid=false;
    }
    const ChassisPower_Init_Config_s *c=&p->slow_config;
    const ChassisPowerSnapshot_s *s=&i->previous;
    const ChassisPowerCommandInput_s command_input={.referee=i->referee,
        .bench_enabled=i->bench_enabled,.bench_limit_w=i->bench_limit_w,.chassis_zero_force=i->zero_force};
    ChassisPowerCommandOutput_s command;
    (void)ChassisPowerBuildCommand(&command_input,&command);
    SuperCapStatus_s cap=c->use_supercap ? i->cap.status : (SuperCapStatus_s){0};
    SuperCapInputPower_s cap_input=c->use_supercap ? i->cap.input_power : (SuperCapInputPower_s){0};
    if(c->buffer_enabled) cap.online &= (uint32_t)(i->now_ms-cap.last_rx_tick_ms)<=20;
    const bool load_valid=c->meter_enabled && i->meter.online &&
        (uint32_t)(i->now_ms-i->meter.last_rx_tick_ms)<=20;
    bool demand_valid=i->previous_valid && s->registered && s->power.status==POWER_MODEL_STATUS_OK &&
        !s->numeric_error && !s->transient.fault && s->online_mask==15 &&
        s->telemetry.motor_enabled_mask==15 && s->transient.drive_mask && !s->transient.brake_mask &&
        (uint32_t)(i->now_ms-s->tick_ms)<=5 && !i->zero_force;
    bool spin_feedback_valid=i->previous_valid && s->registered &&
        s->power.status==POWER_MODEL_STATUS_OK && !s->numeric_error && !s->transient.fault &&
        s->online_mask==15 && s->telemetry.motor_enabled_mask==15 &&
        (uint32_t)(i->now_ms-s->tick_ms)<=5 && !i->zero_force;
    float requested=0, predicted=0, motoring=0;
    for(unsigned w=0;w<4;++w) {
        spin_feedback_valid &= (uint32_t)(i->now_ms-s->feedback_tick_ms[w])<=5 &&
            isfinite(s->telemetry.actual_speed_rpm[w]) && isfinite(s->telemetry.target_speed_rpm[w]) &&
            isfinite(s->raw_current[w]);
        demand_valid &= isfinite(s->power.predicted_unlimited_power_w[w]) &&
            (uint32_t)(i->now_ms-s->feedback_tick_ms[w])<=5;
        requested+=s->power.predicted_unlimited_power_w[w];
        if(i->previous_valid && s->registered && s->power.status==POWER_MODEL_STATUS_OK) {
            predicted+=s->power.predicted_unlimited_power_w[w];
            motoring+=s->telemetry.current_command[w]*s->telemetry.actual_speed_rpm[w];
        }
    }
    if(load_valid && isfinite(i->meter.power_w)) requested=fmaxf(requested,i->meter.power_w);
    const ChassisPowerBudgetInput_s budget_input={.referee_limit_w=command.base_power_limit_w,
        .cap_online=cap.online,.cap_output_enabled=cap.output_enabled,.cap_error_code=cap.error_code,
        .cap_energy_ratio=cap.energy_ratio,.cap_reported_limit_w=cap.available_power_limit_w,
        .buffer_control_enabled=c->buffer_enabled,.use_supercap=c->use_supercap,
        .buffer_data_valid=i->referee.valid && i->referee.chassis_output_enabled,
        .buffer_energy_j=i->referee.buffer_energy_j,.buffer_tick_ms=i->referee.buffer_tick_ms,
        .buffer_age_s=(uint32_t)(i->now_ms-i->referee.buffer_tick_ms)*.001f,
        .input_power_valid=c->use_supercap ? cap_input.online : load_valid,
        .measured_input_power_w=c->use_supercap ? cap_input.input_power_w : i->meter.power_w,
        .cap_input_control_ready=cap_input.online && cap_input.input_control_active,
        .load_power_valid=load_valid,.demand_valid=demand_valid,.requested_load_power_w=requested,
        .confirmed_input_target_w=cap_input.input_power_target_w,
        .spin_power_priority=i->adaptive_spin && !i->zero_force,
        .spin_feedback_valid=spin_feedback_valid,
        .spin_entry_allowed=spin_feedback_valid && !s->transient.brake_mask &&
            s->transient.guard_scale==1 && !s->transient.growth_fallback &&
            !s->transient.guard_infeasible && !s->transient.guard_search_exhausted &&
            s->transient.fast_w==0 && (s->transient.mode==CHASSIS_POWER_TRANSIENT_RUN ||
                                      s->transient.mode==CHASSIS_POWER_TRANSIENT_IDLE),
        .spin_capability_limited=i->spin_capability_limited,
        .spin_braking=s->transient.brake_mask!=0,
        .now_ms=i->now_ms,.input_power_tick_ms=i->meter.last_rx_tick_ms};
    bool valid=isfinite(i->dt_s) && i->dt_s>0 && i->dt_s<=.1f &&
        ChassisPowerBudgetUpdate(&c->budget,&budget_input,i->dt_s,&p->budget_state,&o->budget);
    if(!valid) {
        o->budget.applied_budget_w=0;
        p->budget_state.spin_sample_valid=false;
        p->budget_state.spin_base_time_s=0;
        p->budget_state.spin_phase=CHASSIS_SPIN_POWER_DEGRADED;
    }
    o->buffer_estimate_valid=p->budget_state.buffer_estimate_valid;
    if(i->referee.chassis_output_valid && !i->referee.chassis_output_enabled) valid=false;
    o->cap_command=command.command;
    o->cap_command.input_control_enabled=c->buffer_enabled && i->referee.valid && command.command.enable_dcdc;
    o->cap_command.input_power_target_w=(uint16_t)fminf(65535,fmaxf(0,o->budget.input_target_w));
    o->cap_command_valid=c->use_supercap;
    const float nominal=o->budget.applied_budget_w;
    const float target=nominal*c->algorithm.safety_factor;
    PowerMeterStatus_s meter=i->meter;
    bool meter_valid=c->meter_enabled && meter.online;
    if(c->stage>=1) {
        meter.power_w=s->measured_power_w;meter.last_rx_tick_ms=s->meter_tick_ms;
        meter_valid=c->meter_enabled && i->previous_valid && s->meter_online &&
            (uint32_t)(i->now_ms-meter.last_rx_tick_ms)<=20;
    }
    const ChassisPowerOuterLoopInput_s outer_input={.nominal_budget_w=nominal,.target_power_w=target,
        .measured_power_w=meter.power_w,.predicted_unlimited_power_w=predicted,.meter_online=meter_valid,
        .allow_positive_correction=c->stage==0 ? meter_valid && meter.power_w>=0 && motoring>=0 :
            meter_valid && s->transient.positive_allowed && meter.power_w<=nominal,
        .hold_positive_learning=i->hold_positive_learning && !s->base_limited,
        .learning_allowed=meter_valid && s->transient.learning_allowed && meter.power_w<=nominal &&
            (uint32_t)(i->now_ms-s->tick_ms)<=5,
        .negative_learning_allowed=c->stage==3 && meter_valid && s->transient.negative_learning_allowed &&
            s->transient.negative_learning_threshold_w>=target+c->outer.error_deadband_w &&
            (uint32_t)(i->now_ms-s->tick_ms)<=5,
        .complete_slow_control=c->stage==3,.base_limited=s->base_limited,
        .demand_sufficient=s->transient.drive_mask && predicted>=target-c->outer.activation_margin_w,
        .braking=s->transient.brake_mask!=0,.meter_tick_ms=meter.last_rx_tick_ms,.control_enabled=!i->zero_force};
    if(c->meter_enabled) {
        const bool outer_valid=c->stage==0 ?
            ChassisPowerOuterLoopUpdate(&c->outer,&outer_input,i->dt_s,&p->outer_state,&o->outer) :
            ChassisPowerOuterLoopUpdateIsolated(&c->outer,&outer_input,i->dt_s,&p->outer_state,&o->outer);
        if(!outer_valid) {o->outer.corrected_budget_w=0;valid=false;}
    } else o->outer.corrected_budget_w=nominal;
    o->control=(ChassisPowerControlSnapshot_s){.version=++p->policy_version,.tick_ms=i->now_ms,.config_version=p->slow_version,
        .nominal_budget_w=nominal,.target_power_w=target,.power_deadband_w=c->outer.error_deadband_w,
        .legacy_budget_w=o->outer.corrected_budget_w,.slow_correction_w=o->outer.correction_w,
        .power_ceiling_w=c->buffer_enabled ? nominal : 0,.control_enabled=valid && !i->zero_force,
        .zero_force=i->zero_force,.telemetry={.nominal_budget_w=nominal,.target_power_w=target,
            .corrected_budget_w=o->outer.corrected_budget_w,.filtered_power_w=o->outer.filtered_power_w,
            .error_w=o->outer.error_w,.correction_w=o->outer.correction_w,.outer_mode=(uint8_t)o->outer.mode}};
    if (o->budget.spin_phase!=CHASSIS_SPIN_POWER_OFF)
        o->control.power_ceiling_w=o->budget.spin_power_ceiling_w;
    if(c->buffer_enabled && c->use_supercap && cap.online && cap.output_enabled && !cap.error_code)
        o->control.power_ceiling_w=fminf(o->control.power_ceiling_w,cap.available_power_limit_w);
    o->valid=valid;
    POWER_LOCK();p->policy=*o;p->policy_tick_ms=i->now_ms;POWER_UNLOCK();
    return valid;
}
