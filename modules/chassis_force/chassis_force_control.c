#include "chassis_force_control.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
static void clear_motion(ChassisForceController_s *s)
{
    s->command=(ChassisForceCommandState_s){0};s->spin=(ChassisForceSpinPowerState_s){0};
    s->history_valid=false;s->offer_pending=false;s->offer_timed_out=false;
    s->translation_overspeed=false;s->rotation_overspeed=false;s->wheel_overspeed=0;s->stall_active=0;
}
void ChassisForceControlCancel(ChassisForceController_s *s,uint32_t faults)
{
    if(!s) return;
    s->faults|=faults;clear_motion(s);s->recovery_samples=0;
    s->output.faults=s->faults;s->output.growth_permitted=false;
    s->output.drive_mask=0;s->output.brake_mask=0;s->output.valid=false;
    s->output.state=s->faults ? CHASSIS_FORCE_FAULT : CHASSIS_FORCE_DISABLED;
    memset(s->output.current,0,sizeof s->output.current);
}
bool ChassisForceControlInit(const ChassisForceConfig_s *c,ChassisForceController_s *s)
{
    if(!c || !s) return false;
    memset(s,0,sizeof *s);s->config=*c;
    s->calibrated=ChassisForceGeometryInit(c,&s->geometry);
    s->output.state=s->calibrated ? CHASSIS_FORCE_DISABLED : CHASSIS_FORCE_UNCALIBRATED;
    return s->calibrated;
}
static uint32_t check(const ChassisForceController_s *s,const ChassisForceCycleInput_s *i)
{
    if(!isfinite(i->dt_s) || i->dt_s<=0 || i->dt_s>.005f) return CHASSIS_FORCE_ERROR_TIMING;
    if(s->config_version && i->config_version!=s->config_version) return CHASSIS_FORCE_ERROR_CONFIG;
    const ChassisForceIntent_s *c=&i->intent;
    if(c->motion_mode>CHASSIS_FORCE_SPIN || c->input_permitted>1 || c->safety_reset>1 ||
        c->source_id>CHASSIS_FORCE_SOURCE_VTX || (c->spin_direction!=1 && c->spin_direction!=-1)) return CHASSIS_FORCE_ERROR_PROTOCOL;
    const uint32_t age=i->now_ms-i->command_received_ms,margin=i->transport_margin_ms;
    if(margin>10 || age>10-margin || c->source_age_ms>10-margin-age || c->yaw_age_ms>10-margin-age ||
        i->online_mask!=15 || i->enabled_mask!=15) return CHASSIS_FORCE_ERROR_AGE;
    if(!isfinite(c->input_x) || !isfinite(c->input_y) || fabsf(c->input_x)>1 || fabsf(c->input_y)>1 ||
        !isfinite(c->force_scale) || c->force_scale<0 || c->force_scale>1 ||
        !isfinite(c->relative_yaw_rad) || fabsf(c->relative_yaw_rad)>3.141593f ||
        !isfinite(i->model_budget_w) || i->model_budget_w<0 || !isfinite(i->physical_target_w) || i->physical_target_w<0)
        return CHASSIS_FORCE_ERROR_NUMERIC;
    for(unsigned w=0;w<4;++w) {
        if(!isfinite(i->actual_rpm[w]) || !isfinite(i->feedback_current[w])) return CHASSIS_FORCE_ERROR_NUMERIC;
        const MotorPowerModelConfig_s *m=&i->models[w];
        const float model[]={m->k0,m->k1,m->k2,m->k3,m->k4,m->k5,m->current_conversion};
        for(unsigned k=0;k<sizeof model/sizeof model[0];++k)
            if(!isfinite(model[k])) return CHASSIS_FORCE_ERROR_NUMERIC;
        if(m->current_conversion<=0) return CHASSIS_FORCE_ERROR_CONFIG;
        if((uint32_t)(i->now_ms-i->feedback_tick_ms[w])>5) return CHASSIS_FORCE_ERROR_AGE;
        if(i->temperature[w]>=s->config.temp_stop_c) return CHASSIS_FORCE_ERROR_THERMAL;
    }
    return 0;
}
static void hysteresis(bool *active,float value,float limit,float ratio)
{
    if(value>=limit) *active=true;else if(value<=limit*ratio) *active=false;
}
static bool emit(ChassisForceController_s *s,ChassisForceCycleOutput_s *o,bool valid)
{
    s->output.valid=valid;s->pending_record=true;*o=s->output;return valid;
}
static uint32_t stamp(const ChassisForceCycleInput_s *i)
{
    return i->clock_us ? i->clock_us(i->clock_context) : 0;
}
static void observe(ChassisForceController_s *s,const ChassisForceCycleInput_s *i)
{
    bool fresh=i->online_mask==15;
    for(unsigned w=0;w<4;++w) {
        s->output.actual_rpm[w]=i->actual_rpm[w];s->output.feedback_current[w]=i->feedback_current[w];
        s->output.temperature[w]=i->temperature[w];s->output.feedback_age_ms[w]=i->now_ms-i->feedback_tick_ms[w];
        fresh &= isfinite(i->actual_rpm[w]) && isfinite(i->feedback_current[w]) && s->output.feedback_age_ms[w]<=5;
    }
    if(!s->calibrated || !fresh) return;
    ChassisForceEstimate(&s->geometry,&s->config,i->actual_rpm,s->output.velocity);
    for(unsigned w=0;w<4;++w) {
        float model=0;for(unsigned k=0;k<3;++k) model+=s->geometry.h[w][k]*s->output.velocity[k];
        const float actual=i->actual_rpm[w]*(6.28318530718f/60)*s->config.wheel[w].radius_m/s->config.wheel[w].reduction;
        s->output.residual+=(actual-model)*(actual-model);
    }
    s->output.residual=sqrtf(s->output.residual);
    s->output.feedback_valid=isfinite(s->output.velocity[0]) && isfinite(s->output.velocity[1]) &&
        isfinite(s->output.velocity[2]) && isfinite(s->output.residual);
}
static void hard_constraints(ChassisForceController_s *s,const ChassisForceCycleInput_s *i)
{
    ChassisForceAllocationInput_s *a=&s->hard_constraints;const ChassisForceConfig_s *c=&s->config;
    memset(a,0,sizeof *a);a->budget_w=i->model_budget_w;
    const float speed=hypotf(s->output.velocity[0],s->output.velocity[1]),omega=s->output.velocity[2];
    hysteresis(&s->translation_overspeed,speed,c->velocity_limit_mps,c->resume_ratio);
    hysteresis(&s->rotation_overspeed,fabsf(omega),c->omega_limit_rad_s,c->resume_ratio);
    if(s->translation_overspeed) {
        const unsigned k=a->linear_count++;
        for(unsigned w=0;w<4;++w) a->linear[k][w]=s->geometry.wrench_map[0][w]*s->output.velocity[0]+
            s->geometry.wrench_map[1][w]*s->output.velocity[1];
    }
    if(s->rotation_overspeed) {
        const unsigned k=a->linear_count++;
        for(unsigned w=0;w<4;++w) a->linear[k][w]=s->geometry.wrench_map[2][w]*omega;
    }
    if(a->linear_count) s->output.limits|=CHASSIS_FORCE_LIMIT_SPEED;
    for(unsigned w=0;w<4;++w) {
        const ChassisForceWheel_s *wheel=&c->wheel[w];
        float maximum=fminf(wheel->current_limit,wheel->force_limit_n*wheel->radius_m/wheel->k_out_nm_a*(16384.0f/20));
        if(i->temperature[w]>c->temp_derate_c) {
            maximum*=(c->temp_stop_c-i->temperature[w])/(c->temp_stop_c-c->temp_derate_c);
            s->output.limits|=CHASSIS_FORCE_LIMIT_THERMAL;
        }
        if(fabsf(i->actual_rpm[w])<=c->stall_rpm) maximum=fminf(maximum,c->startup_current_limit);
        a->min_current[w]=-maximum;a->max_current[w]=maximum;
        if(fabsf(i->actual_rpm[w])>=c->wheel_limit_rpm) s->wheel_overspeed|=(uint8_t)(1U<<w);
        else if(fabsf(i->actual_rpm[w])<=c->wheel_limit_rpm*c->resume_ratio) s->wheel_overspeed&=(uint8_t)~(1U<<w);
        if(s->wheel_overspeed & (1U<<w)) {
            if(i->actual_rpm[w]>0) a->max_current[w]=0;else a->min_current[w]=0;
            s->output.limits|=CHASSIS_FORCE_LIMIT_SPEED;
        }
        a->models[w]=i->models[w];
        /* One fixed envelope for every candidate, covering current speed and calibrated growth margin. */
        a->rpm_bound[w]=fabsf(i->actual_rpm[w])+c->speed_prediction_rpm;
        if(s->history_valid && fabsf(s->sent_current[w])>=c->stall_current_command && fabsf(i->actual_rpm[w])<c->stall_rpm) {
            if(!(s->stall_active & (1U<<w))) {s->stall_since_ms[w]=i->now_ms;s->stall_active|=(uint8_t)(1U<<w);}
            else if((uint32_t)(i->now_ms-s->stall_since_ms[w])>=c->stall_timeout_ms) s->faults|=CHASSIS_FORCE_ERROR_STALL;
        } else s->stall_active&=(uint8_t)~(1U<<w);
    }
}
bool ChassisForceControlStep(ChassisForceController_s *s,const ChassisForceCycleInput_s *i,ChassisForceCycleOutput_s *o)
{
    if(!s || !i || !o) return false;
    const ChassisForceCycleOutput_s previous=s->output;
    s->output=(ChassisForceCycleOutput_s){.command_seq=i->intent.command_seq,.cycle_seq=++s->cycle_seq,
        .config_version=i->config_version,.safety_epoch=i->safety_epoch,.tick_ms=i->now_ms,
        .policy_version=i->policy_version,.dt_s=i->dt_s,.physical_target_w=i->physical_target_w,
        .calibration_checksum=s->config.checksum,.calibrated=s->calibrated,
        .load_power_w=i->load_power_w,.command_age_ms=i->now_ms-i->command_received_ms};
    uint32_t phase_start=stamp(i);
    observe(s,i);s->output.estimate_us=stamp(i)-phase_start;
    if(!s->calibrated) {s->output.state=CHASSIS_FORCE_UNCALIBRATED;return emit(s,o,true);}
    if(i->external_faults) {ChassisForceControlCancel(s,i->external_faults);return emit(s,o,false);}
    if(!i->control_enabled || !i->intent.input_permitted || i->intent.motion_mode==CHASSIS_FORCE_ZERO) {
        clear_motion(s);s->recovery_samples=0;s->output.state=s->faults ? CHASSIS_FORCE_FAULT : CHASSIS_FORCE_DISABLED;
        s->output.faults=s->faults;return emit(s,o,true);
    }
    if(!s->clock_started) {
        s->clock_started=true;s->safety_epoch=i->safety_epoch;s->recovery_samples=0;clear_motion(s);
        s->output.state=CHASSIS_FORCE_READY;return emit(s,o,true);
    }
    const uint32_t fault=check(s,i);
    if(fault) {
        s->faults|=fault;clear_motion(s);s->recovery_samples=0;
        s->output.state=CHASSIS_FORCE_FAULT;s->output.faults=s->faults;return emit(s,o,false);
    }
    s->config_version=i->config_version;
    if(s->safety_epoch!=i->safety_epoch) {
        s->clock_started=true;s->safety_epoch=i->safety_epoch;s->recovery_samples=0;clear_motion(s);
        s->output.state=CHASSIS_FORCE_READY;return emit(s,o,true);
    }
    if(s->faults) {
        if(i->intent.safety_reset && hypotf(i->intent.input_x,i->intent.input_y)<=s->config.input_deadzone &&
            i->intent.motion_mode!=CHASSIS_FORCE_SPIN) ++s->recovery_samples;else s->recovery_samples=0;
        if(s->recovery_samples<3) {
            s->output.state=CHASSIS_FORCE_FAULT;s->output.faults=s->faults;return emit(s,o,false);
        }
        s->faults=0;clear_motion(s);s->output.state=CHASSIS_FORCE_READY;return emit(s,o,true);
    }
    if(s->recovery_samples<3) {++s->recovery_samples;s->output.state=CHASSIS_FORCE_READY;return emit(s,o,true);}
    if(!s->output.feedback_valid) {
        ChassisForceControlCancel(s,CHASSIS_FORCE_ERROR_NUMERIC);return emit(s,o,false);
    }
    s->output.source_age_ms=i->intent.source_age_ms+s->output.command_age_ms+i->transport_margin_ms;
    s->output.yaw_age_ms=i->intent.yaw_age_ms+s->output.command_age_ms+i->transport_margin_ms;
    phase_start=stamp(i);
    hard_constraints(s,i);
    if(s->faults) {s->output.state=CHASSIS_FORCE_FAULT;s->output.faults=s->faults;clear_motion(s);return emit(s,o,false);}
    const bool spinning=i->intent.motion_mode==CHASSIS_FORCE_SPIN;
    ChassisForceConfig_s shaping=s->config;
#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE
    const float previous_power_torque=s->spin.torque_nm;
    const bool direction_change=s->spin_direction!=0 && s->spin_direction!=i->intent.spin_direction;
    if(direction_change) s->spin=(ChassisForceSpinPowerState_s){0};
    const bool history=s->history_valid && s->last_submission_ok && (uint32_t)(i->now_ms-s->last_sent_ms)<=5;
    ChassisForceSpinPowerInput_s power={.enabled=spinning,.growth_allowed=i->growth_allowed &&
        !s->output.limits && history && previous.alpha>=.9999f && previous.beta>=.9999f &&
        !previous.brake_mask && !previous.braking && !direction_change,
        .meter_valid=i->meter_valid,.last_submission_ok=history,.target_w=i->physical_target_w,
        .load_w=i->load_power_w,.dt_s=i->dt_s,.now_ms=i->now_ms,.meter_tick_ms=i->meter_tick_ms};
    (void)ChassisForceSpinPowerUpdate(&s->config,&power,&s->spin,&shaping.spin_torque_nm);
    /* READY zero records establish safe startup permission without a fictitious speed reference. */
    if(spinning && history && previous.state==CHASSIS_FORCE_READY && !s->output.limits)
        shaping.spin_torque_nm=fminf(s->config.spin_start_torque_nm,s->config.torque_rise_nm_s*i->dt_s);
    s->output.growth_permitted=s->spin.growth_permitted;
#endif
    s->spin_direction=i->intent.spin_direction;
    ChassisForceCommandOutput_s request;
    const bool exiting=s->command.previous_mode==CHASSIS_FORCE_SPIN && !spinning;
    if(!ChassisForceCommand(&shaping,&i->intent,s->output.velocity[2],i->now_ms,i->dt_s,&s->command,&request)) {
        s->faults|=CHASSIS_FORCE_ERROR_NUMERIC;s->output.state=CHASSIS_FORCE_FAULT;
        s->output.faults=s->faults;clear_motion(s);return emit(s,o,false);
    }
    s->output.braking=request.braking;s->output.brake_timed_out=request.brake_timed_out;
    ChassisForceToCurrent(&s->geometry,request.high,s->hard_constraints.high);
    ChassisForceToCurrent(&s->geometry,request.low,s->hard_constraints.low);
#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE
    bool candidate_braking=request.braking;
    for(unsigned w=0;w<4;++w) candidate_braking |= fabsf(i->actual_rpm[w])>s->config.stall_rpm &&
        (s->hard_constraints.high[w]+s->hard_constraints.low[w])*i->actual_rpm[w]<0;
    if(candidate_braking) {
        s->spin.growth_permitted=false;s->output.growth_permitted=false;
        s->spin.torque_nm=fminf(s->spin.torque_nm,previous_power_torque);
        if(spinning) {
            s->command.torque_nm=copysignf(fminf(fabsf(s->command.torque_nm),previous_power_torque),s->command.torque_nm);
            request.low[2]=s->command.torque_nm;
            ChassisForceToCurrent(&s->geometry,request.low,s->hard_constraints.low);
        }
    }
#endif
    memcpy(s->output.force_g,request.force_g,sizeof request.force_g);
    for(unsigned k=0;k<3;++k) s->output.wrench_request[k]=request.high[k]+request.low[k];
    s->output.torque_nm=request.low[2];
    for(unsigned w=0;w<4;++w) s->output.raw_current[w]=s->hard_constraints.high[w]+s->hard_constraints.low[w];
    s->output.operating_power_w=ChassisForcePredict(&s->hard_constraints,s->output.raw_current);
    if(!isfinite(s->output.operating_power_w)) {
        ChassisForceControlCancel(s,CHASSIS_FORCE_ERROR_NUMERIC);return emit(s,o,false);
    }
    ChassisForceAllocationInput_s a=s->hard_constraints;
    const bool soft=s->history_valid && !exiting && (uint32_t)(i->now_ms-s->last_sent_ms)<=5 && i->model_budget_w>=s->last_budget_w;
    s->last_budget_w=i->model_budget_w;
    if(soft) for(unsigned w=0;w<4;++w) {
        const float step=s->config.wheel[w].current_slew*i->dt_s;
        a.min_current[w]=fmaxf(a.min_current[w],s->sent_current[w]-step);
        a.max_current[w]=fminf(a.max_current[w],s->sent_current[w]+step);
    }
    ChassisForceAllocationOutput_s allocation;
    s->output.command_us=stamp(i)-phase_start;phase_start=stamp(i);
    bool allocated=ChassisForceAllocate(&a,&allocation);
    if(!allocated && soft) {s->output.limits|=CHASSIS_FORCE_LIMIT_SLEW;allocated=ChassisForceAllocate(&s->hard_constraints,&allocation);}
    s->output.allocation_us=stamp(i)-phase_start;phase_start=stamp(i);
    if(allocated) {
        s->output.alpha=allocation.alpha;s->output.beta=allocation.beta;
        memcpy(s->output.allocated_current,allocation.current,sizeof allocation.current);
        if(allocation.alpha<.9999f || allocation.beta<.9999f) s->output.limits|=CHASSIS_FORCE_LIMIT_ALLOCATION;
        float quantized[4];bool valid=false;
        for(unsigned attempt=0;attempt<8;++attempt) {
            for(unsigned w=0;w<4;++w) quantized[w]=(int16_t)(s->output.alpha*a.high[w]+s->output.beta*a.low[w]);
            if(ChassisForceCandidateValid(&s->hard_constraints,quantized)) {valid=true;break;}
            s->output.limits|=CHASSIS_FORCE_LIMIT_INTEGER;
            if(s->output.beta>0) s->output.beta=fmaxf(0,s->output.beta-.0001f);
            else s->output.alpha=fmaxf(0,s->output.alpha-.0001f);
        }
        if(valid) for(unsigned w=0;w<4;++w) s->output.current[w]=(int16_t)quantized[w];
        else {allocated=false;s->output.limits|=CHASSIS_FORCE_LIMIT_INTEGER;}
    }
    if(!allocated) {
        s->output.limits|=CHASSIS_FORCE_LIMIT_ALLOCATION;
        memset(s->output.current,0,sizeof s->output.current);s->output.alpha=0;s->output.beta=0;
    }
    float final[4];for(unsigned w=0;w<4;++w) final[w]=s->output.current[w];
    s->output.predicted_w=ChassisForcePredict(&s->hard_constraints,final);
    s->output.zero_over_budget=!allocated && s->output.predicted_w>s->hard_constraints.budget_w;
    if(s->output.zero_over_budget) s->output.limits|=CHASSIS_FORCE_LIMIT_ZERO_POWER;
    s->output.guard_us=stamp(i)-phase_start;
    for(unsigned w=0;w<4;++w) {
        if(final[w]!=0) {
            if(fabsf(i->actual_rpm[w])<=s->config.stall_rpm || final[w]*i->actual_rpm[w]>=0) s->output.drive_mask|=(uint8_t)(1U<<w);
            else s->output.brake_mask|=(uint8_t)(1U<<w);
        }
    }
#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE
    if(s->output.brake_mask) {
        s->output.growth_permitted=false;s->spin.growth_permitted=false;
        s->spin.torque_nm=fminf(s->spin.torque_nm,previous_power_torque);
    }
#endif
    if(spinning && i->offer_allowed && s->output.growth_permitted && !s->output.limits &&
        fabsf(i->physical_target_w-s->spin.filtered_w)<=s->config.power_deadband_w*2) {
        if(s->offer_pending && i->policy_version!=s->offer_policy_version && i->physical_target_w>previous.physical_target_w+.1f)
            s->offer_pending=false;
        if(s->offer_pending && (uint32_t)(i->now_ms-s->offer_since_ms)>s->config.grant_timeout_ms) s->offer_timed_out=true;
        if(!s->offer_timed_out) {
            const float offered=fminf(s->config.spin_torque_max_nm,fabsf(request.low[2])+s->config.torque_probe_step_nm);
            float wrench[3]={request.high[0],request.high[1],i->intent.spin_direction*offered},candidate[4];
            ChassisForceToCurrent(&s->geometry,wrench,candidate);
            ChassisForceAllocationInput_s physical=s->hard_constraints;physical.budget_w=FLT_MAX/16;
            if(offered>fabsf(request.low[2]) && ChassisForceCandidateValid(&physical,candidate)) {
                s->output.offered_torque_nm=offered;
                s->output.offered_power_w=fminf(ChassisForcePredict(&physical,candidate),i->physical_target_w+s->config.power_request_step_w);
                if(!s->offer_pending) {s->offer_since_ms=i->now_ms;s->offer_policy_version=i->policy_version;s->offer_pending=true;}
            }
        }
    } else if(!spinning || !i->offer_allowed) {s->offer_pending=false;s->offer_timed_out=false;}
    s->output.state=request.braking ? CHASSIS_FORCE_EXIT_BRAKING :
        (s->output.limits ? CHASSIS_FORCE_LIMITED : CHASSIS_FORCE_RUN);
    return emit(s,o,true);
}
bool ChassisForceControlValidateFinal(const ChassisForceController_s *s,const int16_t current[4])
{
    if(!s || !current) return false;
    bool zero=true;float c[4];for(unsigned w=0;w<4;++w) {c[w]=current[w];zero &= current[w]==0;}
    if(zero) return true; /* Zero is always the no-drive fallback, even if modeled idle power exceeds budget. */
    return s->pending_record && s->output.valid &&
        (s->output.state==CHASSIS_FORCE_RUN || s->output.state==CHASSIS_FORCE_LIMITED ||
         s->output.state==CHASSIS_FORCE_EXIT_BRAKING) && s->calibrated &&
        ChassisForceCandidateValid(&s->hard_constraints,c);
}
bool ChassisForceControlRecord(ChassisForceController_s *s,uint32_t cycle,const int16_t current[4],bool submitted,uint32_t now)
{
    if(!s || !current || !s->pending_record || cycle!=s->output.cycle_seq) return false;
    s->pending_record=false;s->last_submission_ok=submitted;s->output.can_submitted=submitted;
    float final[4];for(unsigned w=0;w<4;++w) {final[w]=current[w];s->output.current[w]=current[w];}
    ChassisForceFromCurrent(&s->geometry,final,s->output.wrench_sent_est);
    s->output.predicted_w=0;
    for(unsigned w=0;w<4;++w) {
        s->output.raw_predicted_w[w]=s->calibrated ? ChassisForcePredictWheel(&s->hard_constraints.models[w],
            s->output.raw_current[w],s->hard_constraints.rpm_bound[w]) : 0;
        s->output.final_predicted_w[w]=s->calibrated ? ChassisForcePredictWheel(&s->hard_constraints.models[w],
            final[w],s->hard_constraints.rpm_bound[w]) : 0;
        if(!isfinite(s->output.raw_predicted_w[w]) || !isfinite(s->output.final_predicted_w[w])) {
            s->output.raw_predicted_w[w]=0;s->output.final_predicted_w[w]=0;
        }
        s->output.predicted_w+=s->output.final_predicted_w[w];
    }
    if(!submitted) {
        s->faults|=CHASSIS_FORCE_ERROR_CAN;s->output.faults=s->faults;s->output.state=CHASSIS_FORCE_FAULT;
        clear_motion(s);return false;
    }
    memcpy(s->sent_current,final,sizeof final);s->history_valid=true;s->last_sent_ms=now;
    if(s->command.previous_mode==CHASSIS_FORCE_SPIN && !s->output.braking) {
        const float torque=fmaxf(0,s->spin_direction*s->output.wrench_sent_est[2]);
        /* Ordinary shaping and integer truncation retain their bounded residual.
         * Only a constrained execution projects the power integrator downward. */
        ChassisForceSpinPowerProject(&s->spin,torque,s->output.limits!=0);
        if(s->output.limits) s->command.torque_nm=s->spin_direction*fminf(fabsf(s->command.torque_nm),torque);
    }
    return true;
}
