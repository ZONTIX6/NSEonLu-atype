#include "dji_force_adapter.h"
#include "bsp_dwt.h"
#include "power_meter.h"
#include "chassis_power_config.h"
#include "vehicle_safety.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <math.h>

static DJIMotorOutputGroup_s force_group;
static DJIMotorInstance *force_motors[4];
static ChassisForceActuationSnapshot_s published,cycle_snapshot;
static ChassisForceController_s controller;
static ChassisForceCycleInput_s force_input;
static ChassisForceCycleOutput_s completed;
static uint32_t feedback_sequence[4],dwt_count;
static uint32_t can_started_us;
static bool has_completed;
static uint32_t clock_us(void *context) {(void)context;return (uint32_t)DWT_GetTimeline_us();}

static void read_control(void *context) {(void)context;cycle_snapshot=published;}
static void prepare(void *context,const DJIMotorCycle_s *cycle)
{
    (void)context;
    const uint32_t input_start=clock_us(NULL);
    PowerMeterStatus_s meter={0};
#if POWER_METER_ENABLE
    (void)PowerMeterGetStatus(&meter);
#endif
    const uint32_t now=HAL_GetTick();
    force_input=(ChassisForceCycleInput_s){.intent=cycle_snapshot.intent,.now_ms=now,
        .dt_s=DWT_GetDeltaT(&dwt_count),.command_received_ms=cycle_snapshot.command_received_ms,
        .config_version=cycle_snapshot.config_version,.safety_epoch=cycle_snapshot.safety_epoch,
        .external_faults=cycle_snapshot.command_faults,
        .transport_margin_ms=cycle_snapshot.transport_margin_ms,
        .clock_us=clock_us,
        .control_enabled=cycle_snapshot.power.control_enabled && VehicleSafetyAllowsDrive(now),
        .growth_allowed=cycle_snapshot.growth_allowed,.offer_allowed=cycle_snapshot.offer_allowed};
    for(unsigned w=0;w<4;++w) {
        const unsigned n=force_group.motor_indices[w];
        force_input.actual_rpm[w]=cycle->measure[n].speed_aps/6;
        force_input.feedback_current[w]=cycle->measure[n].real_current;
        force_input.feedback_tick_ms[w]=cycle->measure[n].rx_tick_ms;
        force_input.temperature[w]=cycle->measure[n].temperature;
        feedback_sequence[w]=cycle->measure[n].rx_sequence;
        if(cycle->online[n] && feedback_sequence[w]) force_input.online_mask|=(uint8_t)(1U<<w);
        if(cycle->stop[n]!=MOTOR_STOP) force_input.enabled_mask|=(uint8_t)(1U<<w);
    }
    if(controller.clock_started && !ChassisPowerForceBegin(&cycle_snapshot.power,&meter,now,force_input.dt_s,&force_input)) {
        force_input.control_enabled=false;
        if(cycle_snapshot.power.control_enabled) force_input.external_faults|=
            !isfinite(force_input.dt_s) || force_input.dt_s<=0 || force_input.dt_s>.005f ?
                CHASSIS_FORCE_ERROR_TIMING : CHASSIS_FORCE_ERROR_AGE;
    }
    ChassisForceCycleOutput_s result;
    const uint32_t input_elapsed=clock_us(NULL)-input_start;
    (void)ChassisForceControlStep(&controller,&force_input,&result);
    controller.output.input_us=input_elapsed;
    can_started_us=clock_us(NULL);
    for(unsigned w=0;w<4;++w) cycle->current[force_group.motor_indices[w]]=result.current[w];
}
static void limit(void *context,const DJIMotorCycle_s *cycle) {(void)context;(void)cycle;}
static bool validate(void *context,const DJIMotorCycle_s *cycle,const int16_t commands[DJI_MOTOR_CNT],
    const bool enabled[DJI_MOTOR_CNT],uint32_t now)
{
    (void)context;
    int16_t current[4];bool valid=true,zero=true;uint32_t fault=0;
    taskENTER_CRITICAL();
    valid=VehicleSafetyAllowsDrive(now) && VehicleSafetyEpoch()==cycle_snapshot.safety_epoch &&
        published.safety_epoch==cycle_snapshot.safety_epoch && published.config_version==cycle_snapshot.config_version &&
        published.power.control_enabled && !published.power.zero_force &&
        published.intent.input_permitted && published.intent.motion_mode!=CHASSIS_FORCE_ZERO &&
        (uint32_t)(now-cycle_snapshot.power.tick_ms)<=10;
    if(published.config_version!=cycle_snapshot.config_version) fault|=CHASSIS_FORCE_ERROR_CONFIG;
    if((uint32_t)(now-cycle_snapshot.power.tick_ms)>10) fault|=CHASSIS_FORCE_ERROR_AGE;
    fault|=published.command_faults;
    const uint32_t age=now-cycle_snapshot.command_received_ms,margin=cycle_snapshot.transport_margin_ms;
    const bool command_fresh=margin<=10 && age<=10-margin &&
        cycle_snapshot.intent.source_age_ms<=10-margin-age && cycle_snapshot.intent.yaw_age_ms<=10-margin-age;
    valid &= command_fresh;
    if(!command_fresh) fault|=CHASSIS_FORCE_ERROR_AGE;
    for(unsigned w=0;w<4;++w) {
        const unsigned n=force_group.motor_indices[w];current[w]=commands[n];
        zero &= commands[n]==0;
        valid &= enabled[n] && cycle->stop[n]!=MOTOR_STOP && force_motors[w]->stop_flag!=MOTOR_STOP &&
            DaemonIsOnline(force_motors[w]->daemon) && force_motors[w]->measure.rx_sequence!=0 &&
            (uint32_t)(now-force_motors[w]->measure.rx_tick_ms)<=5 &&
            (uint32_t)(now-cycle->measure[n].rx_tick_ms)<=5;
        if(!DaemonIsOnline(force_motors[w]->daemon) || !force_motors[w]->measure.rx_sequence ||
            (uint32_t)(now-force_motors[w]->measure.rx_tick_ms)>5 || (uint32_t)(now-cycle->measure[n].rx_tick_ms)>5)
            fault|=CHASSIS_FORCE_ERROR_AGE;
        if(!isfinite(force_motors[w]->measure.speed_aps)) fault|=CHASSIS_FORCE_ERROR_NUMERIC;
        if(controller.calibrated && force_motors[w]->measure.temperature>=controller.config.temp_stop_c)
            fault|=CHASSIS_FORCE_ERROR_THERMAL;
    }
    taskEXIT_CRITICAL();
    if(fault) {ChassisForceControlCancel(&controller,fault);return false;}
    if(!valid && zero) return true; /* Preserve READY/fault-recovery progress on safe zero frames. */
    valid &= ChassisForceControlValidateFinal(&controller,current);
    if(!valid) ChassisForceControlCancel(&controller,0);
    return valid;
}
static void record(void *context,const DJIMotorCycle_s *cycle,const int16_t commands[DJI_MOTOR_CNT],
    const bool enabled[DJI_MOTOR_CNT],uint8_t submitted,uint8_t successful)
{
    (void)context;
    int16_t current[4];bool sent=true;
    force_input.enabled_mask=0;
    for(unsigned w=0;w<4;++w) {
        const unsigned n=force_group.motor_indices[w];const uint8_t mask=(uint8_t)(1U<<cycle->motors[n]->sender_group);
        current[w]=commands[n];sent &= (submitted&successful&mask)!=0;
        if(enabled[n]) force_input.enabled_mask|=(uint8_t)(1U<<w);
    }
    const uint32_t now=HAL_GetTick();
    controller.output.can_us=clock_us(NULL)-can_started_us;
    (void)ChassisForceControlRecord(&controller,controller.output.cycle_seq,current,sent,now);
    controller.output.tick_ms=now;
    (void)ChassisPowerForceRecord(&force_input,&controller.output,feedback_sequence);
    taskENTER_CRITICAL();completed=controller.output;has_completed=true;taskEXIT_CRITICAL();
}
bool DJIForceAdapterInit(DJIMotorInstance *const motors[4],const ChassisForceConfig_s *config)
{
    if(!motors || !config || force_group.registered) return false;
    DJIMotorOutputGroup_s group={.max_current_command=16384,.owns_raw_current=true};
    for(unsigned w=0;w<4;++w) {
        if(!motors[w] || motors[w]->motor_type!=M3508 || !DJIMotorFindIndex(motors[w],&group.motor_indices[w]) ||
            motors[w]->sender_group!=motors[0]->sender_group) return false;
        for(unsigned j=0;j<w;++j) if(motors[j]==motors[w]) return false;
    }
    /* Invalid calibration is a supported locked state, never a fallback to wheel PID. */
    (void)ChassisForceControlInit(config,&controller);
    force_group=group;memcpy(force_motors,motors,sizeof force_motors);
    const DJIMotorOutputHooks_s hooks={.read_control=read_control,.prepare_current=prepare,.limit=limit,
        .validate_current=validate,.record_final=record};
    if(!DJIMotorRegisterOutputGroup(&force_group,&hooks)) return false;
    (void)DWT_GetDeltaT(&dwt_count);return true;
}
bool DJIForceAdapterPublish(const ChassisForceActuationSnapshot_s *s)
{
    if(!s || !force_group.registered) return false;
    taskENTER_CRITICAL();published=*s;
    force_group.control=(DJIMotorGroupControl_s){.version=s->intent.command_seq,.tick_ms=s->power.tick_ms,
        .control_enabled=s->power.control_enabled,.zero_force=s->power.zero_force,.autotune_permitted=false};
    for(unsigned w=0;w<4;++w) force_motors[w]->stop_flag=s->power.control_enabled ? MOTOR_ENALBED : MOTOR_STOP;
    taskEXIT_CRITICAL();return true;
}
bool DJIForceAdapterGetSnapshot(ChassisForceCycleOutput_s *s)
{
    if(!s) return false;
    taskENTER_CRITICAL();*s=completed;const bool valid=has_completed;taskEXIT_CRITICAL();return valid;
}
