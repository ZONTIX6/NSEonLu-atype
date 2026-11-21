#include "dji_tuning_adapter.h"
#include "dji_power_adapter.h"
#include "bsp_dwt.h"
#include "power_meter.h"
#include "vehicle_safety.h"
#include "chassis_power_shadow.h"
#include "../../predictive_power/speed_observation_time.h"
#include "chassis_power_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include <string.h>
#include "../../../config/30_velocity_observation/chassis_traction_config.h"
static DJIMotorOutputGroup_s adapter_group;
#if CHASSIS_TRACTION_MODE > 0
#include "../../../config/02_chassis/chassis_config.h"
static ChassisTractionState_s traction_state;
static ChassisTractionInput_s traction_input;
static DJITractionSnapshot_s traction_working, traction_published;
static void ApplyTraction(const DJIMotorCycle_s *cycle, ChassisPowerCycleInput_s *input)
{
    const ChassisVelocityConfig_s geometry={.omni_x=CHASSIS_DRIVE_TYPE==CHASSIS_DRIVE_OMNI_X,
        .wheel_radius_m=RADIUS_WHEEL*.001f,.reduction_ratio=REDUCTION_RATIO_WHEEL,
        .wheel_base_m=WHEEL_BASE*.001f,.track_width_m=TRACK_WIDTH*.001f,
        .direction={OMNI_X_LF_DIRECTION,OMNI_X_RF_DIRECTION,OMNI_X_LB_DIRECTION,OMNI_X_RB_DIRECTION}};
    const ChassisTractionConfig_s config=ChassisTractionDefaultConfig(&geometry);
    ChassisTractionInput_s *i=&traction_input;
    *i=(ChassisTractionInput_s){.reference=input->control.traction,
        .now_ms=input->now_ms,.dt_s=input->dt_s,
        .online_mask=input->online_mask,.enabled_mask=input->enabled_mask,
        .drive_enabled=input->control.control_enabled && !input->control.zero_force &&
            !input->control.autotune_permitted && VehicleSafetyAllowsDrive(input->now_ms) &&
            (uint32_t)(input->now_ms-input->control.tick_ms)<=10U,
        .targets_match=true};
    for(unsigned w=0;w<4;++w) {
        const unsigned n=adapter_group.motor_indices[w];
        i->raw_current[w]=input->raw_current[w];
        i->actual_rpm[w]=(float)cycle->measure[n].raw_rpm;
        i->max_current[w]=fminf(adapter_group.max_current_command,cycle->motors[n]->motor_controller.speed_PID.MaxOut);
        i->feedback_tick_ms[w]=input->feedback_tick_ms[w];
        i->feedback_sequence[w]=input->feedback_sequence[w];
        i->targets_match &= isfinite(input->target_rpm[w]) &&
            fabsf(input->target_rpm[w]-input->control.target_rpm[w])<=.001f;
        traction_working.raw_current[w]=input->raw_current[w];
        traction_working.target_rpm[w]=input->target_rpm[w];
        traction_working.feedback_current[w]=input->feedback_current[w];
    }
    (void)ChassisTractionUpdate(&config,i,&traction_state,&traction_working.traction);
    traction_working.reference=i->reference;
    for(unsigned w=0;w<4;++w) {
        input->raw_current[w]=traction_working.traction.current[w];
        if(traction_working.traction.intervened_mask & (1U<<w)) {
            PIDInstance *pid=&cycle->motors[adapter_group.motor_indices[w]]->motor_controller.speed_PID;
            pid->Iout=pid->ITerm=pid->Last_ITerm=0;
        }
    }
}
#endif

static DJIMotorInstance *adapter_motors[4];
static ChassisPowerControlSnapshot_s adapter_control, adapter_cycle_control;
static uint32_t adapter_dwt_count;
static ChassisPowerCycleInput_s adapter_input;
static ChassisPowerCycleOutput_s adapter_output;
static DJIVelocityFeedbackEvidence_s velocity_feedback_evidence;
static uint32_t last_corrected_version;
static bool have_corrected_version;
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
static PPObservationClock observation_clock;
#endif
#if CHASSIS_SPEED_CAPTURE_ENABLE
static CSCCapture *speed_capture;
static CSCCycleRecord pending_cycle;
static bool cycle_pending;
static uint32_t observation_cycle,limit_end_cycles;
static SuperCapSnapshot_s observed_input_power;
static ChassisPowerShadowSnapshot capture_shadow;
static PPWheelKF checkpoint[4];
static PPWheelKFConfig checkpoint_config;
static void CaptureFinal(const DJIMotorCycle_s *,const ChassisPowerFinalInput_s *);
#endif

static void AdapterReadControl(void *context)
{
    (void)context;
    adapter_cycle_control=adapter_control;
}

static void AdapterLimit(void *context,const DJIMotorCycle_s *cycle)
{
    (void)context;
    ChassisPowerCycleInput_s *input=&adapter_input;
    *input=(ChassisPowerCycleInput_s){.control=adapter_cycle_control,
        .dt_s=DWT_GetDeltaT(&adapter_dwt_count)};
#if POWER_METER_ENABLE
    (void)PowerMeterGetStatus(&input->meter);
#endif
    input->now_ms=HAL_GetTick();
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
    bool time_valid=PPObservationClockUpdate(&observation_clock,DWT_GetCycleCount(),input->now_ms,DWT_GetFrequencyHz());
    input->observation_now_us=observation_clock.now_us;
#endif
    for(unsigned w=0;w<4;++w) {
        const unsigned index=adapter_group.motor_indices[w];
        input->raw_current[w]=cycle->current[index];
        input->actual_rpm[w]=cycle->actual_aps[index]/6.0f;
        input->target_rpm[w]=cycle->target_aps[index]/6.0f;
        input->feedback_current[w]=cycle->measure[index].real_current;
        input->feedback_tick_ms[w]=cycle->measure[index].rx_tick_ms;
        input->feedback_sequence[w]=cycle->measure[index].rx_sequence;
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
        const DJI_Motor_Measure_s *m=&cycle->measure[index];
        input->raw_speed[w]=(PPWheelSample){.sequence=m->rx_sequence,.source_epoch=observation_clock.source_epoch,
            .omega_rad_s=m->raw_rpm*.10471975511965977f,.online=cycle->online[index]};
        if(time_valid && PPObservationReceiveTime(&observation_clock,m->rx_cycles,m->rx_tick_ms,20000,
                                                  &input->raw_speed[w].rx_time_us))
            input->raw_time_valid_mask|=(uint8_t)(1U<<w);
#endif
        if(cycle->online[index]) input->online_mask |= (uint8_t)(1U<<w);
        if(cycle->stop[index]!=MOTOR_STOP) input->enabled_mask |= (uint8_t)(1U<<w);
    }
#if CHASSIS_TRACTION_MODE > 0
    ApplyTraction(cycle,input);
#endif
    (void)ChassisPowerLimit(input,&adapter_output);
#if CHASSIS_POWER_CONTROL_ENABLE
    for(unsigned w=0;w<4;++w) cycle->current[adapter_group.motor_indices[w]]=adapter_output.current[w];
#else
    /* Keep power diagnostics, but only motion/data validity may gate PID output. */
    bool valid=input->control.control_enabled && !input->control.zero_force &&
        (uint32_t)(input->now_ms-input->control.tick_ms)<=10U &&
        isfinite(input->dt_s) && input->dt_s>0 && input->dt_s<=.005f;
    for(unsigned w=0;w<4;++w)
        valid &= isfinite(input->raw_current[w]) && isfinite(input->actual_rpm[w]) &&
            isfinite(input->target_rpm[w]) && isfinite(input->feedback_current[w]);
    for(unsigned w=0;w<4;++w) {
        const bool enabled=valid && (input->online_mask & input->enabled_mask & (1U<<w)) &&
            input->feedback_sequence[w]!=0 && (uint32_t)(input->now_ms-input->feedback_tick_ms[w])<=20U;
        cycle->current[adapter_group.motor_indices[w]]=enabled ?
            fmaxf(-adapter_group.max_current_command,fminf(adapter_group.max_current_command,input->raw_current[w])) : 0;
    }
#endif
#if CHASSIS_SPEED_CAPTURE_ENABLE
    limit_end_cycles=DWT_GetCycleCount();
#endif
}

static bool AdapterValidateCurrent(void *context,const DJIMotorCycle_s *cycle,
    const int16_t commands[DJI_MOTOR_CNT],const bool enabled[DJI_MOTOR_CNT],uint32_t now_ms)
{
    (void)context;
    if(!VehicleSafetyAllowsDrive(now_ms) || !adapter_control.control_enabled || adapter_control.zero_force ||
        adapter_control.version!=adapter_cycle_control.version ||
        adapter_control.config_version!=adapter_cycle_control.config_version ||
        (uint32_t)(now_ms-adapter_control.tick_ms)>10U) return false;
    for(unsigned w=0;w<4;++w) {
        const unsigned n=adapter_group.motor_indices[w];
        const DJIMotorInstance *motor=cycle->motors[n];
        if(commands[n]!=0 && (!enabled[n] || motor->stop_flag==MOTOR_STOP || !DaemonIsOnline(motor->daemon) ||
            !motor->measure.rx_sequence || (uint32_t)(now_ms-motor->measure.rx_tick_ms)>20U ||
            (uint32_t)(now_ms-cycle->measure[n].rx_tick_ms)>20U)) return false;
    }
    return true;
}

static void AdapterRecordFinal(void *context,const DJIMotorCycle_s *cycle,
    const int16_t commands[DJI_MOTOR_CNT],const bool enabled[DJI_MOTOR_CNT],
    uint8_t submitted_groups,uint8_t successful_groups)
{
    (void)context;
    ChassisPowerFinalInput_s final={.now_ms=HAL_GetTick(),.control_version=adapter_cycle_control.version,
        .can_submitted=true,.submitted_groups=submitted_groups,.successful_groups=successful_groups};
    for(unsigned w=0;w<4;++w) {
        const unsigned index=adapter_group.motor_indices[w];
        const uint8_t group=(uint8_t)(1U<<cycle->motors[index]->sender_group);
        final.current_command[w]=commands[index];
        final.can_submitted &= (submitted_groups & successful_groups & group)!=0;
        if(enabled[index]) final.enabled_mask |= (uint8_t)(1U<<w);
    }
    bool corrected=adapter_cycle_control.velocity_feedback_corrected;
    for(unsigned w=0;w<4;++w) {
        const float target=cycle->target_aps[adapter_group.motor_indices[w]];
        corrected &= isfinite(target) &&
            fabsf(target-adapter_cycle_control.target_rpm[w]*6.0f)<=.001f;
    }
    const bool drive=adapter_cycle_control.control_enabled && !adapter_cycle_control.zero_force;
    taskENTER_CRITICAL();
    velocity_feedback_evidence.tick_ms=final.now_ms;
    velocity_feedback_evidence.control_version=final.control_version;
    velocity_feedback_evidence.flags=DJI_FEEDBACK_AVAILABLE |
        (corrected ? DJI_FEEDBACK_CORRECTED_TARGETS : 0U) |
        (final.can_submitted ? DJI_FEEDBACK_CAN_ACCEPTED : 0U) |
        (drive ? DJI_FEEDBACK_DRIVE_ENABLED : 0U) | ((uint16_t)final.enabled_mask<<4);
    memcpy(velocity_feedback_evidence.final_current,final.current_command,sizeof final.current_command);
    if(corrected && drive && final.enabled_mask==15U && final.can_submitted &&
        (!have_corrected_version || last_corrected_version!=final.control_version)) {
        ++velocity_feedback_evidence.submitted_correction_versions;
        last_corrected_version=final.control_version;
        have_corrected_version=true;
    }
    taskEXIT_CRITICAL();
#if CHASSIS_TRACTION_MODE > 0
    traction_working.available=true;
    traction_working.tick_ms=final.now_ms;
    traction_working.control_version=final.control_version;
    traction_working.enabled_mask=final.enabled_mask;
    traction_working.can_submitted=final.can_submitted;
    ++traction_working.cycles;
    const bool intervention=traction_working.traction.intervened_mask!=0;
    traction_working.intervention_cycles+=intervention;
    traction_working.spin_cycles+=traction_working.traction.spin_mask!=0;
    traction_working.lock_cycles+=traction_working.traction.lock_mask!=0;
    traction_working.submitted_intervention_cycles+=intervention && final.enabled_mask==15U && final.can_submitted;
    memcpy(traction_working.final_current,final.current_command,sizeof final.current_command);
    taskENTER_CRITICAL();traction_published=traction_working;taskEXIT_CRITICAL();
#endif
    (void)ChassisPowerRecordFinal(&final);
#if CHASSIS_SPEED_CAPTURE_ENABLE
    CaptureFinal(cycle,&final);
#endif
}

bool DJIPowerAdapterInit(const DJIPowerAdapterConfig_s *config)
{
    if(config==NULL || config->count!=4 || adapter_group.registered) return false;
    DJIMotorOutputGroup_s group={.max_current_command=config->max_current_command};
    ChassisPowerSnapshot_s power;
    if(!ChassisPowerGetSnapshot(&power)) return false;
    for(unsigned w=0;w<4;++w) {
        if(config->motors[w]==NULL || config->motors[w]->motor_type!=M3508 ||
            !DJIMotorFindIndex(config->motors[w],&group.motor_indices[w])) return false;
        for(unsigned j=0;j<w;++j) if(config->motors[w]==config->motors[j]) return false;
    }
    adapter_group=group;
    const DJIMotorOutputHooks_s hooks={.policy=DJITuningAdapterPolicy(),.read_control=AdapterReadControl,.limit=AdapterLimit,
        .validate_current=AdapterValidateCurrent,.record_final=AdapterRecordFinal};
    if(!DJIMotorRegisterOutputGroup(&adapter_group,&hooks)) return false;
    memcpy(adapter_motors,config->motors,sizeof adapter_motors);
    (void)DWT_GetDeltaT(&adapter_dwt_count);
    RTTTelemetryWriteChassisPowerHeader();
    return true;
}

bool DJIPowerAdapterPublish(const ChassisPowerControlSnapshot_s *control)
{
    if(control==NULL || !adapter_group.registered) return false;
    taskENTER_CRITICAL();
    adapter_control=*control;
    adapter_group.control=(DJIMotorGroupControl_s){.version=control->version,.tick_ms=control->tick_ms,
        .control_enabled=control->control_enabled,.autotune_permitted=control->autotune_permitted,
        .zero_force=control->zero_force};
    memcpy(adapter_group.control.target_rpm,control->target_rpm,sizeof control->target_rpm);
    for(unsigned w=0;w<4;++w) {
        adapter_motors[w]->motor_controller.pid_ref=control->target_rpm[w]*6.0f;
        adapter_motors[w]->stop_flag=control->control_enabled ? MOTOR_ENALBED : MOTOR_STOP;
    }
    taskEXIT_CRITICAL();
    return true;
}


bool DJIPowerAdapterDriveStopped(uint32_t now_ms)
{
    taskENTER_CRITICAL();
    bool stopped=!adapter_control.control_enabled || adapter_control.zero_force ||
        !VehicleSafetyAllowsDrive(now_ms) || (uint32_t)(now_ms-adapter_control.tick_ms)>10;
    if(!stopped && adapter_group.registered) {
        stopped=true;
        for(unsigned w=0;w<4;++w) stopped &= adapter_motors[w]->stop_flag==MOTOR_STOP;
    }
    taskEXIT_CRITICAL();return stopped;
}

bool DJIPowerAdapterGetVelocityFeedbackEvidence(DJIVelocityFeedbackEvidence_s *out)
{
    if(!out) return false;
    taskENTER_CRITICAL();
    *out=velocity_feedback_evidence;
    taskEXIT_CRITICAL();
    return (out->flags & DJI_FEEDBACK_AVAILABLE)!=0U;
}

bool DJIPowerAdapterGetTractionSnapshot(DJITractionSnapshot_s *out)
{
    if(!out) return false;
#if CHASSIS_TRACTION_MODE > 0
    taskENTER_CRITICAL();*out=traction_published;taskEXIT_CRITICAL();
    return out->available;
#else
    *out=(DJITractionSnapshot_s){0};return false;
#endif
}

bool DJIPowerAdapterTractionLimited(uint32_t now_ms)
{
#if CHASSIS_TRACTION_MODE >= 2
    taskENTER_CRITICAL();
    const bool limited=traction_published.available && traction_published.enabled_mask==15U &&
        traction_published.traction.intervened_mask!=0 &&
        (uint32_t)(now_ms-traction_published.tick_ms)<=20U;
    taskEXIT_CRITICAL();
    return limited;
#else
    (void)now_ms;return false;
#endif
}
#if CHASSIS_SPEED_CAPTURE_ENABLE
static void RawFeedback(void *context,unsigned index,const DJI_Motor_Measure_s *m)
{
    (void)context;
    if(!speed_capture || !m) return;
    unsigned w;
    for(w=0;w<4 && adapter_group.motor_indices[w]!=index;++w) {}
    if(w==4) return;
    bool source_changed=false;
    if((speed_capture->state==CSC_ARMED || speed_capture->state==CSC_RECORDING) &&
       (CSCReadU32(speed_capture->metadata.bytes+44)&1U)) {
        const uint8_t *meta=speed_capture->metadata.bytes;
        PPObservationClock clock={.now_us=CSCReadU64(meta+48),.cycles=CSCReadU32(meta+36),
            .tick_ms=CSCReadU32(meta+40),.hz=CSCReadU32(meta+8),.fraction=CSCReadU32(meta+56),
            .source_epoch=CSCReadU32(meta+32),.initialized=true};
        /* Raw IRQs can resume before AdapterLimit notices a stopped/rebased
         * clock. Never place those events under the prior metadata epoch. */
        source_changed=!PPObservationClockUpdate(&clock,m->rx_cycles,m->rx_tick_ms,DWT_GetFrequencyHz());
    }
    CSCRawRecord r={{0}};uint8_t *b=r.bytes;
    b[0]=(uint8_t)w;b[1]=m->temperature;
    CSCWriteU32(b+4,m->rx_sequence);CSCWriteU32(b+8,m->rx_tick_ms);CSCWriteU32(b+12,m->rx_cycles);
    CSCWriteU16(b+16,(uint16_t)m->raw_rpm);CSCWriteU16(b+18,(uint16_t)m->raw_current);
    CSCWriteF32(b+20,m->speed_aps/6);CSCWriteU16(b+24,(uint16_t)m->real_current);
    /* Driver callback is an ISR; PRIMASK preserves nesting and excludes all
     * interrupt writers while publishing this one bounded record. */
    uint32_t irq=__get_PRIMASK();__disable_irq();
    if(source_changed)CSCFreezeReason(speed_capture,CSC_FREEZE_VERSION);
    CSCWriteU32(b+36,speed_capture->raw_count);
    (void)CSCAppendRaw(speed_capture,&r);__set_PRIMASK(irq);
}
bool DJIPowerAdapterSetSpeedCapture(CSCCapture *capture)
{
    if(!capture || !adapter_group.registered) return false;
    speed_capture=capture;
    return DJIMotorSetSpeedFeedbackObserver(RawFeedback,NULL);
}
void DJIPowerAdapterPublishInputPower(const SuperCapSnapshot_s *input)
{if(input) {taskENTER_CRITICAL();observed_input_power=*input;taskEXIT_CRITICAL();}}
bool DJIPowerAdapterTakeSpeedCycle(CSCCycleRecord *out)
{if(!out || !cycle_pending) return false;*out=pending_cycle;cycle_pending=false;return true;}
static void WriteConfig(uint8_t *b,const PPWheelKFConfig *c)
{
    CSCWriteU32(b,c->version);CSCWriteU32(b+4,c->max_gap_us);CSCWriteU32(b+8,c->max_age_us);
    CSCWriteU32(b+12,c->max_horizon_us);CSCWriteF32(b+16,c->q);CSCWriteF32(b+20,c->r);
    CSCWriteF32(b+24,c->initial_p00);CSCWriteF32(b+28,c->initial_p11);CSCWriteF32(b+32,c->gate_squared);
    b[36]=c->max_rejections;
}
bool DJIPowerAdapterDescribeSpeedCapture(CSCMetadata *meta)
{
    if(!meta || !adapter_group.registered || !observation_clock.initialized) return false;
    uint8_t *b=meta->bytes;
    CSCWriteU32(b+32,observation_clock.source_epoch);CSCWriteU32(b+36,observation_clock.cycles);
    CSCWriteU32(b+40,observation_clock.tick_ms);CSCWriteU64(b+48,observation_clock.now_us);
    CSCWriteU32(b+56,observation_clock.fraction);
    uint32_t flags=observation_clock.initialized ? 1U:0U;
    if(ChassisPowerShadowGetWheelKFCheckpoint(checkpoint,&checkpoint_config)) {
        flags|=6U;WriteConfig(b+64,&checkpoint_config);
        for(unsigned w=0;w<4;++w) {
            uint8_t *d=b+104+80*w;const PPWheelKF *k=&checkpoint[w];const PPWheelEstimate *e=&k->corrected;
            const float f[7]={e->omega,e->alpha,e->p00,e->p01,e->p11,e->innovation,e->innovation_variance};
            for(unsigned j=0;j<7;++j) CSCWriteF32(d+4*j,f[j]);
            CSCWriteU64(d+28,e->corrected_time_us);CSCWriteU64(d+36,e->output_time_us);
            CSCWriteU32(d+44,e->sequence);CSCWriteU32(d+48,e->source_epoch);CSCWriteU32(d+52,e->config_version);
            d[56]=e->valid;d[57]=(uint8_t)e->reason;d[58]=k->initialized;d[59]=k->seen;
            CSCWriteU64(d+60,k->seen_time_us);CSCWriteU32(d+68,k->seen_sequence);CSCWriteU32(d+72,k->seen_source_epoch);
            d[76]=k->rejected_consecutive;
        }
    }
    CSCWriteU32(b+44,flags);
    taskENTER_CRITICAL();
    for(unsigned w=0;w<4;++w) {
        const PIDInstance *pid=&adapter_motors[w]->motor_controller.speed_PID;
        const float f[7]={pid->Kp,pid->Ki,pid->Kd,pid->MaxOut,pid->IntegralLimit,pid->Output_LPF_RC,pid->Derivative_LPF_RC};
        for(unsigned j=0;j<7;++j) CSCWriteF32(b+424+w*32+j*4,f[j]);
        CSCWriteU32(b+424+w*32+28,(uint32_t)pid->Improve);
        b[24+w]=adapter_motors[w]->motor_settings.motor_reverse_flag==MOTOR_DIRECTION_REVERSE ? (uint8_t)-1:1;
    }
    CSCWriteU32(b+624,adapter_control.config_version);taskEXIT_CRITICAL();
    CSCWriteU16(b+554,1);
    if(ChassisPowerShadowGet(&capture_shadow))CSCWriteU32(b+556,capture_shadow.model_checksum);
    return true;
}
static void CaptureFinal(const DJIMotorCycle_s *cycle,const ChassisPowerFinalInput_s *final)
{
    if(!speed_capture) return;
    memset(&pending_cycle,0,sizeof pending_cycle);uint8_t *b=pending_cycle.bytes;
    const bool shadow_available=ChassisPowerShadowGet(&capture_shadow);
    if(!shadow_available) {
        capture_shadow.elapsed_us=UINT32_MAX;capture_shadow.allocation.elapsed_us=UINT32_MAX;
        capture_shadow.wheel_kf.kf_elapsed_us=UINT32_MAX;
        for(unsigned w=0;w<4;++w)capture_shadow.wheel_kf.accepted_age_us[w]=UINT32_MAX;
    }
    CSCWriteU32(b,adapter_input.now_ms);CSCWriteU32(b+20,adapter_cycle_control.config_version);
    CSCWriteU32(b+24,final->control_version);CSCWriteU32(b+28,capture_shadow.model_version);
    uint32_t model_version,kf_version;
    (void)ChassisPowerShadowVersions(&model_version,&kf_version);
    CSCWriteU32(b+36,kf_version);
    CSCWriteU32(b+40,observation_clock.source_epoch);CSCWriteU32(b+44,observation_cycle++);
    CSCWriteU64(b+48,adapter_input.observation_now_us);
    uint32_t flags=(adapter_input.raw_time_valid_mask ? 2U:0U)|
        (capture_shadow.wheel_kf.configured ? 4U:0U)|(capture_shadow.wheel_kf.paused ? 8U:0U);
    CSCWriteU32(b+64,flags);b[72]=final->enabled_mask;b[73]=adapter_input.online_mask;
    b[74]=final->submitted_groups;b[75]=final->successful_groups;
    CSCWriteU32(b+76,speed_capture->rejected_records);
    uint32_t skipped=0,first=0,last=0;bool submitted=false;
    for(unsigned w=0;w<4;++w) {
        const unsigned n=adapter_group.motor_indices[w],g=cycle->motors[n]->sender_group;
        const DJI_Motor_Measure_s *m=&cycle->measure[n];uint8_t *d=b+80+80*w;
        const PPWheelEstimate *k=&capture_shadow.wheel_kf.corrected[w];
        CSCWriteU32(d,m->rx_sequence);CSCWriteU32(d+4,m->rx_tick_ms);CSCWriteU32(d+8,m->rx_cycles);
        CSCWriteU16(d+12,(uint16_t)m->raw_rpm);CSCWriteU16(d+14,(uint16_t)m->raw_current);
        CSCWriteF32(d+16,m->speed_aps/6);CSCWriteU16(d+20,(uint16_t)m->real_current);
        CSCWriteU16(d+22,(uint16_t)final->current_command[w]);
        CSCWriteF32(d+24,adapter_input.target_rpm[w]);CSCWriteF32(d+28,adapter_input.raw_current[w]);
        CSCWriteF32(d+32,adapter_output.current[w]);d[36]=m->temperature;d[37]=(uint8_t)g;d[38]=(uint8_t)k->reason;
        d[39]=(adapter_input.raw_time_valid_mask&(1U<<w) ? 1:0)|
            (capture_shadow.wheel_kf.current[w].valid && !capture_shadow.wheel_kf.paused ? 2:0)|
            (final->submitted_groups&(1U<<g) ? 8:0)|(final->successful_groups&(1U<<g) ? 16:0);
        const float f[7]={k->omega,k->alpha,k->p00,k->p01,k->p11,k->innovation,k->innovation_variance};
        for(unsigned j=0;j<7;++j) CSCWriteF32(d+40+j*4,f[j]);
        CSCWriteU32(d+68,capture_shadow.wheel_kf.accepted_age_us[w]);
        CSCWriteU32(d+72,capture_shadow.wheel_kf.corrected_count[w]);
        CSCWriteU32(d+76,capture_shadow.wheel_kf.skipped_events[w]);skipped+=capture_shadow.wheel_kf.skipped_events[w];
    }
    if(cycle->submission)for(unsigned g=0;g<DJI_SENDER_GROUP_COUNT;++g) {
        bool member=false;for(unsigned w=0;w<4;++w)member|=cycle->motors[adapter_group.motor_indices[w]]->sender_group==g;
        if(member && (final->submitted_groups&(1U<<g))) {
            if(!submitted)first=cycle->submission->begin_cycles[g];
            last=cycle->submission->end_cycles[g];submitted=true;
        }
    }
    CSCWriteU32(b+56,first);CSCWriteU32(b+60,last);CSCWriteU32(b+68,skipped);
    uint8_t *d=b+400;
    taskENTER_CRITICAL();
    const uint32_t input_flags=observed_input_power.input_received_frame_count &&
        isfinite(observed_input_power.input_power.input_power_w) ?
        1U|((observed_input_power.input_power.online &&
             (uint32_t)(final->now_ms-observed_input_power.input_power.last_rx_tick_ms)<=20) ? 2U:0U):0;
    CSCWriteF32(d,(input_flags&3U)==3U ? observed_input_power.input_power.input_power_w:0);
    CSCWriteU32(d+4,observed_input_power.input_received_frame_count);
    CSCWriteU32(d+8,observed_input_power.input_power.last_rx_tick_ms);
    CSCWriteU32(d+12,input_flags);
    taskEXIT_CRITICAL();
    const uint32_t load_flags=adapter_input.meter.rx_sequence && isfinite(adapter_input.meter.power_w) ?
        1U|((adapter_input.meter.online && (uint32_t)(final->now_ms-adapter_input.meter.last_rx_tick_ms)<=20) ? 2U:0U):0;
    CSCWriteF32(d+16,(load_flags&3U)==3U ? adapter_input.meter.power_w:0);CSCWriteU32(d+20,adapter_input.meter.rx_sequence);
    CSCWriteU32(d+24,adapter_input.meter.last_rx_tick_ms);
    CSCWriteU32(d+28,load_flags);
    for(unsigned w=0;w<4;++w) CSCWriteF32(d+32+4*w,adapter_output.transient.guard_rpm[w]);
    CSCWriteU32(d+48,capture_shadow.wheel_kf.kf_elapsed_us);
    CSCWriteU32(d+52,capture_shadow.allocation.elapsed_us);CSCWriteU32(d+56,capture_shadow.elapsed_us);
    uint32_t slow_us,strategy_version;ChassisPowerShadowEnergyTiming(&slow_us,&strategy_version);
    CSCWriteU32(d+60,slow_us);CSCWriteU32(b+32,strategy_version);
    for(unsigned off=64;off<=92;off+=4) CSCWriteU32(d+off,UINT32_MAX);
    CSCWriteU32(d+64,observation_clock.cycles);
    uint32_t hz=DWT_GetFrequencyHz();
    if(submitted && hz)CSCWriteU32(d+72,(uint32_t)((uint64_t)(uint32_t)(last-limit_end_cycles)*1000000U/hz));
    CSCWriteF32(d+96,adapter_output.transient.candidate_guard_w);CSCWriteF32(d+100,adapter_output.transient.guard_power_w);
    CSCWriteF32(d+104,adapter_cycle_control.target_power_w);CSCWriteU32(d+108,(uint32_t)capture_shadow.reason);
    cycle_pending=true;
}
#else
bool DJIPowerAdapterSetSpeedCapture(CSCCapture *c) {(void)c;return false;}
void DJIPowerAdapterPublishInputPower(const SuperCapSnapshot_s *c) {(void)c;}
bool DJIPowerAdapterTakeSpeedCycle(CSCCycleRecord *c) {(void)c;return false;}
bool DJIPowerAdapterDescribeSpeedCapture(CSCMetadata *c) {(void)c;return false;}
#endif
