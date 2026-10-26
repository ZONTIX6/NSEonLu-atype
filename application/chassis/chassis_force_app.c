#include "board/board_chassis_force.h"
#include "chassis_force_intent.h"
#include "chassis.h"
#include "dji_force_adapter.h"
#include "board/board_force_calibration.h"
#if CHASSIS_FORCE_CALIBRATION_ENABLE
#include "runtime/force_calibration_app.h"
#endif
#include "board_config.h"
#include "vehicle_safety.h"
#include "message_center.h"
#include "heat_control.h"
#include "super_cap.h"
#include "power_meter.h"
#include "referee_task.h"
#include "referee_UI.h"
#include "bsp_dwt.h"
#include "bsp_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include <string.h>
#ifdef CHASSIS_BOARD
#include "can_comm.h"
static CANCommInstance *force_can;
#else
static Publisher_t *feedback_pub;
static Subscriber_t *force_sub;
#endif
static Publisher_t *heat_pub;
static DJIMotorInstance *force_motors[4];
static ChassisForceIntentReceiver_s command;
static Chassis_Upload_Data_s feedback;
static ChassisVelocityFeedback_s velocity;
const ChassisVelocityFeedback_s *const volatile chassis_velocity=&velocity;
static bool ready,calibrated;
static uint32_t budget_dwt,heat_tick;
static uint32_t command_faults;
static ChassisPowerPolicyInput_s policy_input;
static ChassisForceCycleOutput_s final;
static Referee_Interactive_info_t ui;

bool ChassisGetVelocitySnapshot(ChassisVelocityFeedback_s *s)
{
    if(!s) return false;
    taskENTER_CRITICAL();*s=velocity;taskEXIT_CRITICAL();
    if((uint32_t)(HAL_GetTick()-s->actual.tick_ms)>5) s->actual.valid=false;
    return s->actual.valid;
}
bool ChassisGetTelemetrySnapshot(ChassisTelemetrySnapshot_s *s)
{
    if(s) memset(s,0,sizeof *s);
    return false; /* Force-v1 has a separate schema and no target-speed fields. */
}
void ChassisInit(void)
{
    Motor_Init_Config_s m={.can_init_config.can_handle=&hcan1,.motor_type=M3508};
    const uint8_t ids[4]={1,4,2,3};
    for(unsigned w=0;w<4;++w) {m.can_init_config.tx_id=ids[w];force_motors[w]=DJIMotorInit(&m);}
    const ChassisForceConfig_s force=BoardChassisForceConfig();
    calibrated=ChassisForceConfigValid(&force);
    const ChassisPower_Init_Config_s power=BoardChassisForcePowerConfig();
    ready=ChassisPowerInit(&power) && DJIForceAdapterInit(force_motors,&force);
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    ready=ready && ForceCalibrationAppInit(force_motors);
#endif
    (void)ChassisPowerFeedbackInit();
    for(unsigned w=0;w<4;++w) if(force_motors[w]) DJIMotorStop(force_motors[w]);
    heat_pub=PubRegister(HEAT_CONTROL_DATA_TOPIC,sizeof(HeatControlData_s));
#if REFEREE_ENABLE
    (void)UITaskInit(BOARD_REFEREE_UART,&ui);
#else
    (void)ui;
#endif
#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP
    SuperCap_Init_Config_s cap={.can_config={.can_handle=BOARD_SUPERCAP_CAN,.tx_id=SUPERCAP_COMMAND_CAN_ID,
        .rx_id=SUPERCAP_STATUS_CAN_ID},.offline_reload_count=20,.input_power_feedback_enabled=true};
    (void)SuperCapInit(&cap);
#endif
#if POWER_METER_ENABLE
    PowerMeter_Init_Config_s meter={.can_config={.can_handle=BOARD_POWER_METER_CAN,.tx_id=POWER_METER_STATUS_CAN_ID,
        .rx_id=POWER_METER_STATUS_CAN_ID},.offline_reload_count=10};
    (void)PowerMeterInit(&meter);
#endif
#ifdef CHASSIS_BOARD
    CANComm_Init_Config_s comm={.can_config={.can_handle=&hcan2,.tx_id=0x311,.rx_id=0x312},
        .recv_data_len=44,.send_data_len=sizeof(Chassis_Upload_Data_s),.daemon_count=10};
    force_can=CANCommInit(&comm);
#else
    force_sub=SubRegister("chassis_force_cmd",sizeof(ChassisForceIntentMessage_s));
    feedback_pub=PubRegister("chassis_feed",sizeof(Chassis_Upload_Data_s));
#endif
    (void)DWT_GetDeltaT(&budget_dwt);
    LOGINFO("backend=force-v1 spin=%s exit=%u calibrated=%u schema=1 checksum=%08lx FOLLOW=translation",
        CHASSIS_FORCE_SPIN_MODE ? "power_adaptive_torque" : "fixed_torque",CHASSIS_FORCE_EXIT_MODE,
        calibrated,(unsigned long)force.checksum);
}
void ChassisTask(void)
{
    uint32_t now=HAL_GetTick(),received=now;ChassisForceIntent_s next={0};bool available=false;
#ifdef CHASSIS_BOARD
    uint8_t wire[44];
    if(CANCommCopyFreshAt(force_can,wire,44,&received,10)) available=ChassisForceIntentDecode(wire,44,&next);
#else
    ChassisForceIntentMessage_s message;
    available=SubGetMessage(force_sub,&message)!=0;
    if(available) {next=message.intent;received=message.published_ms;}
#endif
    if(available) {
        if(!ChassisForceIntentValid(&next)) command_faults=next.source_age_ms>10 || next.yaw_age_ms>10 ?
            CHASSIS_FORCE_ERROR_AGE : CHASSIS_FORCE_ERROR_PROTOCOL;
        else if(ChassisForceIntentAccept(&command,&next,received)) command_faults=0;
    }
    now=HAL_GetTick();
#ifdef CHASSIS_BOARD
    const uint32_t margin=1; /* Conservative measured transport bound must be verified on paired boards. */
#else
    const uint32_t margin=0;
#endif
    const bool fresh=ChassisForceIntentFresh(&command,now,margin);
    if(command.valid && !fresh && command.intent.input_permitted && command.intent.motion_mode!=CHASSIS_FORCE_ZERO)
        command_faults|=CHASSIS_FORCE_ERROR_AGE;
#ifdef CHASSIS_BOARD
    const uint32_t age=now-command.received_ms;
    const bool aux=fresh && command.intent.aux_stopped && age<=20 && command.intent.aux_age_ms<=20-age;
    const VehicleSafetyInput_s safety={.input_online=fresh && command.intent.input_permitted,
        .emergency=!fresh || !command.intent.input_permitted,.reset=fresh && command.intent.safety_reset,
        .bench=ATYPE_BENCH_MODE && command.intent.bench_permitted && aux,.aux_stopped=aux};
    VehicleSafetyPublish(&safety,now);
#endif
    const bool has_final=DJIForceAdapterGetSnapshot(&final);
    policy_input=(ChassisPowerPolicyInput_s){.now_ms=now,.dt_s=DWT_GetDeltaT(&budget_dwt),.force_backend=true,
        .zero_force=!fresh || !calibrated || !ready || command.intent.motion_mode==CHASSIS_FORCE_ZERO ||
            !VehicleSafetyAllowsDrive(now) || (has_final && final.state==CHASSIS_FORCE_FAULT),
        .bench_enabled=CHASSIS_POWER_BENCH_TEST!=0,.bench_limit_w=CHASSIS_POWER_BENCH_LIMIT_W,
        .adaptive_spin=command.intent.motion_mode==CHASSIS_FORCE_SPIN && CHASSIS_FORCE_SPIN_MODE==1,
        .hold_positive_learning=command.intent.motion_mode==CHASSIS_FORCE_SPIN && CHASSIS_FORCE_SPIN_MODE==1,
        .spin_capability_limited=has_final && (final.limits || final.faults),
        .force_operating_valid=has_final && final.valid && !final.braking,
        .force_offer_valid=has_final && final.offered_power_w>0 && final.growth_permitted,
        .force_operating_demand_w=has_final ? final.operating_power_w : 0,
        .force_offered_demand_w=has_final ? final.offered_power_w : 0};
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    policy_input.zero_force=!ready || !ForceCalibrationAppPermit(&command,now);
    policy_input.adaptive_spin=false;policy_input.hold_positive_learning=true;
    policy_input.force_operating_valid=false;policy_input.force_offer_valid=false;
    policy_input.force_operating_demand_w=0;policy_input.force_offered_demand_w=0;
#endif
    (void)RefereeGetPowerSnapshot(&policy_input.referee);
#if POWER_METER_ENABLE
    (void)PowerMeterGetStatus(&policy_input.meter);
#endif
#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP
    (void)SuperCapGetSnapshot(now,&policy_input.cap);
#endif
    policy_input.previous_valid=ChassisPowerGetSnapshot(&policy_input.previous);
    policy_input.now_ms=HAL_GetTick();
    ChassisPowerPolicyOutput_s policy;
    (void)ChassisPowerUpdatePolicy(&policy_input,&policy);
    if(policy.cap_command_valid) {
        const bool submitted=SuperCapSendCommand(&policy.cap_command)!=0;
        ChassisPowerCapCommandResult(&policy.cap_command,submitted,HAL_GetTick(),policy_input.cap.input_received_frame_count,&policy);
    }
    /* Fault recovery may observe neutral fresh commands with zero energy allowance.
     * It never bypasses the independent VehicleSafety permission. */
#if !CHASSIS_FORCE_CALIBRATION_ENABLE
    ChassisForceActuationSnapshot_s snapshot={.intent=command.intent,.power=policy.control,
        .command_received_ms=command.received_ms,.config_version=1,.safety_epoch=VehicleSafetyEpoch(),
        .transport_margin_ms=margin,.growth_allowed=policy.force_growth_allowed,
        .command_faults=command_faults,
        .offer_allowed=policy.force_growth_allowed && !policy.budget.recovering};
    snapshot.power.control_enabled=ready && fresh && VehicleSafetyAllowsDrive(now) &&
        command.intent.motion_mode!=CHASSIS_FORCE_ZERO;
    if(!policy.valid) snapshot.power.control_enabled=false;
#endif
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    /* Only approved base watts; no boost, offered demand or calibrated flag. */
    ChassisPowerControlSnapshot_s calibration_power=policy.control;
    float base=CHASSIS_POWER_BENCH_LIMIT_W;
    if(policy_input.referee.valid) base=fminf(base,policy_input.referee.chassis_power_limit_w);
    calibration_power.power_ceiling_w=fminf(base,policy.control.power_ceiling_w);
    calibration_power.control_enabled=ready && policy.valid && !policy_input.zero_force && policy.input_confirmed;
    ForceCalibrationAppPublish(&command,&calibration_power,now);
#else
    (void)DJIForceAdapterPublish(&snapshot);
#endif
    ChassisVelocityFeedback_s v={0};
    if(has_final && (uint32_t)(now-final.tick_ms)<=5 && final.feedback_valid && calibrated) {
        v.actual=(ChassisVelocity_s){.valid=true,.tick_ms=final.tick_ms,.vx_mps=final.velocity[0],
            .vy_mps=final.velocity[1],.wz_rpm=final.velocity[2]*(60/6.28318530718f)};
    }
    taskENTER_CRITICAL();velocity=v;taskEXIT_CRITICAL();
    if((uint32_t)(now-heat_tick)>=100) {
        (void)RefereeGetHeatSnapshot(&feedback.heat);
        HeatControlData_s heat={.valid=feedback.heat.valid,.current_heat=feedback.heat.current_heat,
            .heat_limit=feedback.heat.heat_limit,.cooling_rate=feedback.heat.cooling_rate,
            .robot_state_tick_ms=feedback.heat.robot_state_tick_ms,.power_heat_tick_ms=feedback.heat.power_heat_tick_ms};
        PubPushMessage(heat_pub,&heat);heat_tick=now;
    }
    ChassisPowerFeedbackUpdate(policy_input.previous_valid,&policy_input.previous.power);
    (void)ChassisPowerFeedbackGet(&feedback.power_control);
#ifdef CHASSIS_BOARD
    feedback.heat_robot_age_ms=now-feedback.heat.robot_state_tick_ms;feedback.heat_power_age_ms=now-feedback.heat.power_heat_tick_ms;
    CANCommSend(force_can,(uint8_t *)&feedback);
#else
    PubPushMessage(feedback_pub,&feedback);
#endif
}
