/**
 * @file chassis.c
 * @author NeoZeng neozng1@hnu.edu.cn
 * @brief 底盘应用,负责接收robot_cmd的控制命令并根据命令进行运动学解算,得到输出
 *        注意底盘采取右手系,对于平面视图,底盘纵向运动的正前方为x正方向;横向运动的右侧为y正方向
 *
 * @version 0.1
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "chassis_internal.h"
#include "chassis_motion.h"
#include "chassis_kinematics.h"
#include "chassis_power_budget.h"
#include "chassis_energy_config.h"
#include "chassis_power_outer_loop.h"
#include "chassis_power.h"
#include "dji_power_adapter.h"
#include "chassis_power_config.h"
#include "chassis_power_model_config.h"
#include "chassis_spin_config.h"
#include "chassis_spin.h"
#include "runtime_tuning.h"
#include "remote_control.h"
#include "robot_def.h"
#include "board_config.h"
#include "user_lib.h"
#include "heat_control.h"
#include "vehicle_safety.h"
#if POWER_METER_ENABLE
#include "power_meter.h"
#endif
#include "dji_motor.h"
#include "super_cap.h"
#include "message_center.h"
#include "referee_task.h"

#include "general_def.h"
#include "bsp_dwt.h"
#include "referee_UI.h"
#include "arm_math.h"
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"

/* 根据 config/02_chassis/chassis_config.h 中的宏自动计算的参数。 */
#define HALF_WHEEL_BASE (WHEEL_BASE / 2.0f)     // 半轴距
#define HALF_TRACK_WIDTH (TRACK_WIDTH / 2.0f)   // 半轮距
#define PERIMETER_WHEEL (RADIUS_WHEEL * 2 * PI) // 轮子周长

/* Control-cycle state. Device registration and tuning live in sibling private implementations. */
Chassis_Ctrl_Cmd_s chassis_cmd_recv;         // 底盘接收到的控制命令
static Chassis_Upload_Data_s chassis_feedback_data; // 底盘回传的反馈数据


static ChassisPowerBudgetOutput_s chassis_budget_output;// 底盘功率预算输出
uint32_t chassis_budget_dwt_count;
static ChassisPowerControlSnapshot_s pending_power_control;
static ChassisTelemetrySnapshot_s chassis_telemetry;
static ChassisVelocityFeedback_s chassis_velocity_data;
const ChassisVelocityFeedback_s *const volatile chassis_velocity = &chassis_velocity_data;
static uint32_t heat_publish_tick;

bool ChassisGetVelocitySnapshot(ChassisVelocityFeedback_s *snapshot)
{
    if (snapshot == NULL) return false;
    taskENTER_CRITICAL();
    *snapshot = *chassis_velocity;
    taskEXIT_CRITICAL();
    const uint32_t now=HAL_GetTick();
    if ((uint32_t)(now-snapshot->actual.tick_ms)>CHASSIS_VELOCITY_MAX_AGE_MS)
        snapshot->actual=(ChassisVelocity_s){0};
    if ((uint32_t)(now-snapshot->target.tick_ms)>CHASSIS_VELOCITY_MAX_AGE_MS)
        snapshot->target=(ChassisVelocity_s){0};
    return snapshot->actual.valid;
}

bool ChassisGetTelemetrySnapshot(ChassisTelemetrySnapshot_s *snapshot)
{
    if (snapshot == NULL) return false;
    taskENTER_CRITICAL();
    *snapshot = chassis_telemetry;
    taskEXIT_CRITICAL();
    return snapshot->valid;
}

#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED
ChassisSpinState_s chassis_spin_state;
static ChassisSpinInput_s chassis_spin_input;
static uint32_t chassis_spin_dwt_count;
#endif
/* 用于自旋变速策略的时间变量 */
// static float t;

/* 私有函数计算的中介变量,设为静态避免参数传递的开销 */
static float chassis_vx, chassis_vy;                      // 将云台系的速度投影到底盘
static float vt_lf, vt_rf, vt_lb, vt_rb;                  // 底盘速度解算后的临时输出,待进行限幅

#define LF_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH + CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE - CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)
#define RF_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH - CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE - CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)
#define LB_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH + CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE + CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)
#define RB_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH - CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE + CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)
/**
 * @brief 计算每个轮毂电机的输出,正运动学解算
 *        用宏进行预替换减小开销,运动解算具体过程参考教程
 */
static ChassisWheelOutput_s WheelSpeedFor(float vx, float vy, float wz)
{
    const ChassisKinematicsInput_s input = {
        .vx = vx,
        .vy = vy,
        .wz = wz,
        .rotation_lever_sum = {
            LF_ROTATION_LEVER_SUM,
            RF_ROTATION_LEVER_SUM,
            LB_ROTATION_LEVER_SUM,
            RB_ROTATION_LEVER_SUM,
        },
    };
    ChassisWheelOutput_s output = {0};

#if CHASSIS_DRIVE_TYPE == CHASSIS_DRIVE_OMNI_X
    const float wheel_direction[CHASSIS_WHEEL_COUNT] = {
        OMNI_X_LF_DIRECTION,
        OMNI_X_RF_DIRECTION,
        OMNI_X_LB_DIRECTION,
        OMNI_X_RB_DIRECTION,
    };
    (void)ChassisOmniXCalculate(&input, wheel_direction, &output);
#else
    (void)ChassisMecanumCalculate(&input, &output);
#endif

    return output;
}

static void CalculateWheelSpeed(void)
{
    const ChassisWheelOutput_s output=WheelSpeedFor(chassis_vx,chassis_vy,chassis_cmd_recv.wz);
    vt_lf = output.speed_aps[CHASSIS_WHEEL_LEFT_FRONT];
    vt_rf = output.speed_aps[CHASSIS_WHEEL_RIGHT_FRONT];
    vt_lb = output.speed_aps[CHASSIS_WHEEL_LEFT_BACK];
    vt_rb = output.speed_aps[CHASSIS_WHEEL_RIGHT_BACK];
}

/**
 * @brief 根据裁判系统和电容剩余容量对输出进行限制并设置电机参考值
 *
 */
static void LimitChassisOutput()
{
    // 底盘解算输出沿用 rpm；统一 DJI 驱动的速度环使用度/秒。
    pending_power_control.target_rpm[0] = vt_lf / 6.0f;
    pending_power_control.target_rpm[1] = vt_rf / 6.0f;
    pending_power_control.target_rpm[2] = vt_lb / 6.0f;
    pending_power_control.target_rpm[3] = vt_rb / 6.0f;
    pending_power_control.tick_ms = HAL_GetTick();
    DJIPowerAdapterPublish(&pending_power_control);
}

/**
 * @brief 根据每个轮子的速度反馈,计算底盘的实际运动速度,逆运动解算
 *        对于双板的情况,考虑增加来自底盘板IMU的数据
 *
 */
static const ChassisVelocityConfig_s velocity_config={
        .omni_x=CHASSIS_DRIVE_TYPE==CHASSIS_DRIVE_OMNI_X,
        .wheel_radius_m=RADIUS_WHEEL*.001f, .reduction_ratio=REDUCTION_RATIO_WHEEL,
        .wheel_base_m=WHEEL_BASE*.001f, .track_width_m=TRACK_WIDTH*.001f,
        .direction={OMNI_X_LF_DIRECTION,OMNI_X_RF_DIRECTION,OMNI_X_LB_DIRECTION,OMNI_X_RB_DIRECTION}};

#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
static void PrepareSpinLimits(void)
{
    ChassisSpinInput_s *i=&chassis_spin_input;
    i->extension_path=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_DIRECT;
    if(!i->extension_path || !i->spin_enabled) return;
    ChassisVelocityInput_s velocity={.now_ms=i->now_ms,.online_mask=15};
    ChassisVelocity_s body={0};
    for(unsigned w=0;w<4;++w) {
        velocity.rpm[w]=i->actual_rpm[w];velocity.feedback_tick_ms[w]=i->feedback_tick_ms[w];
    }
    i->velocity_valid=ChassisEstimateVelocity(&velocity_config,&velocity,&body);
    /* Spin calibration and protection keep their existing revolutions/s units. */
    i->actual_body_wz_rps=body.wz_rpm/60.0f;i->velocity_tick_ms=body.tick_ms;
    ChassisWheelOutput_s translation=WheelSpeedFor(chassis_vx,chassis_vy,0);
    const ChassisWheelOutput_s rotation=WheelSpeedFor(0,0,1);
    float scale=1;
    for(unsigned w=0;w<4;++w) {
        velocity.rpm[w]=translation.speed_aps[w]/6;
        velocity.feedback_tick_ms[w]=i->now_ms;
        if(chassis_spin_config.wheel_hard_rpm>0 && fabsf(velocity.rpm[w])>chassis_spin_config.wheel_hard_rpm)
            scale=fminf(scale,chassis_spin_config.wheel_hard_rpm/fabsf(velocity.rpm[w]));
    }
    i->velocity_valid &= ChassisEstimateVelocity(&velocity_config,&velocity,&body);
    i->translation_mps=hypotf(body.vx_mps,body.vy_mps);
    if(chassis_spin_config.body_hard_rps>0 && fabsf(body.wz_rpm)/60.0f>chassis_spin_config.body_hard_rps)
        scale=fminf(scale,chassis_spin_config.body_hard_rps/(fabsf(body.wz_rpm)/60.0f));
    /* If translation already consumes the wheel allowance, bound translation
     * first. This leaves a feasible Wz=0 and never uses spin to evade the cap. */
    chassis_vx*=scale;chassis_vy*=scale;
    i->body_wz_offset=body.wz_rpm/60.0f*scale;
    for(unsigned w=0;w<4;++w) {
        i->wheel_rpm_offset[w]=translation.speed_aps[w]/6*scale;
        i->wheel_rpm_per_wz[w]=rotation.speed_aps[w]/6;
        velocity.rpm[w]=i->wheel_rpm_per_wz[w];
    }
    i->velocity_valid &= ChassisEstimateVelocity(&velocity_config,&velocity,&body);
    i->body_wz_per_wz=body.wz_rpm/60.0f;
}
#endif

static void EstimateSpeed(void)
{
    DJIMotorInstance *const motors[4]={motor_lf,motor_rf,motor_lb,motor_rb};
    ChassisVelocityInput_s input={0};
    /* Raw CAN feedback remains meaningful while coasting in ZERO_FORCE.
     * Copy all wheels under the same lock as the motor task's feedback snapshot. */
    taskENTER_CRITICAL();
    input.now_ms=HAL_GetTick();
    for (unsigned i=0;i<4;++i) {
        if (motors[i]==NULL) continue;
        input.rpm[i]=motors[i]->measure.speed_aps/6.0f;
        input.feedback_tick_ms[i]=motors[i]->measure.rx_tick_ms;
        if (motors[i]->measure.rx_sequence!=0 && DaemonIsOnline(motors[i]->daemon))
            input.online_mask|=(uint8_t)(1U<<i);
    }
    taskEXIT_CRITICAL();
    ChassisVelocityFeedback_s velocity={0};
    (void)ChassisEstimateVelocity(&velocity_config,&input,&velocity.actual);
    const bool drive_enabled=pending_power_control.control_enabled &&
        !pending_power_control.zero_force && VehicleSafetyAllowsDrive(input.now_ms);
    for (unsigned i=0;i<4;++i) {
        input.rpm[i]=drive_enabled ? pending_power_control.target_rpm[i] : 0;
        input.feedback_tick_ms[i]=input.now_ms;
    }
    input.online_mask=15U;
    (void)ChassisEstimateVelocity(&velocity_config,&input,&velocity.target);
    taskENTER_CRITICAL();
    chassis_velocity_data=velocity;
    taskEXIT_CRITICAL();
}

static void UpdateChassisPowerBudget(void)
{
    /* Only the robot task owns these snapshots. Keep full diagnostic copies
     * off its stack, including headroom for floating-point context saves. */
    static ChassisPowerPolicyInput_s input;
    static ChassisPowerPolicyOutput_s output;
    input=(ChassisPowerPolicyInput_s){.now_ms=HAL_GetTick(),
        .dt_s=DWT_GetDeltaT(&chassis_budget_dwt_count),
        .zero_force=chassis_cmd_recv.chassis_mode==CHASSIS_ZERO_FORCE,
        .bench_enabled=CHASSIS_POWER_BENCH_TEST!=0,.bench_limit_w=CHASSIS_POWER_BENCH_LIMIT_W};
    (void)RefereeGetPowerSnapshot(&input.referee);
#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP
    (void)SuperCapGetSnapshot(input.now_ms,&input.cap);
#endif
#if POWER_METER_ENABLE
    (void)PowerMeterGetStatus(&input.meter);
#endif
    input.previous_valid=ChassisPowerGetSnapshot(&input.previous);
    input.now_ms=HAL_GetTick(); /* Evaluate ages after acquiring all asynchronous snapshots. */
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
    input.hold_positive_learning=chassis_cmd_recv.chassis_mode==CHASSIS_ROTATE;
    input.adaptive_spin=chassis_cmd_recv.chassis_mode==CHASSIS_ROTATE;
    input.spin_capability_limited=chassis_spin_state.limit_state==CHASSIS_SPIN_LIMIT_HARD ||
        (!chassis_spin_input.extension_path && chassis_spin_state.applied_wz>=chassis_spin_config.max_wz) ||
        ((chassis_spin_state.block_reasons & CHASSIS_SPIN_BLOCK_TRACKING) &&
         (chassis_spin_state.tracking_hold_s>=.100f || chassis_spin_state.trend_no_improvement_s>=.100f));
    input.spin_hard_margin=chassis_spin_state.hard_margin;
    DJIMotorInstance *capability_motors[POWER_MODEL_MOTOR_COUNT]={motor_lf,motor_rf,motor_lb,motor_rb};
    for(unsigned w=0;w<POWER_MODEL_MOTOR_COUNT;++w) {
        const float maximum=capability_motors[w]->motor_controller.speed_PID.MaxOut;
        input.spin_capability_limited |= !isfinite(maximum) || maximum<=0 ||
            fabsf(input.previous.raw_current[w])>=maximum;
    }
#endif
    (void)ChassisPowerUpdatePolicy(&input,&output);
#if !CHASSIS_POWER_CONTROL_ENABLE
    /* Power calculations are observational; retain timing and referee stop permission.
     * Apply this before the supercap submission gate so a failed command still stops drive. */
    output.control.control_enabled=!input.zero_force &&
        isfinite(input.dt_s) && input.dt_s>0 && input.dt_s<=.1f &&
        (!input.referee.chassis_output_valid || input.referee.chassis_output_enabled);
#endif
    if(output.cap_command_valid) {
        const bool submitted=SuperCapSendCommand(&output.cap_command);
        ChassisPowerCapCommandResult(&output.cap_command,submitted,HAL_GetTick(),
            input.cap.input_received_frame_count,&output);
    }
    chassis_budget_output=output.budget;
    pending_power_control=output.control;
    pending_power_control.control_enabled &= chassis_power_ready;
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED && POWER_METER_ENABLE
    static ChassisPowerSnapshot_s power_state;
    power_state=input.previous;
    const bool power_state_valid=input.previous_valid;
    PowerMeterStatus_s meter_status=input.meter;
    bool meter_valid=meter_status.online;
#if CHASSIS_POWER_CONTROL_STAGE >= 1
    meter_status.power_w=power_state.measured_power_w;
    meter_status.last_rx_tick_ms=power_state.meter_tick_ms;
    meter_valid=power_state_valid && power_state.meter_online &&
        (uint32_t)(input.now_ms-meter_status.last_rx_tick_ms)<=20;
#endif
    chassis_spin_input=(ChassisSpinInput_s){.spin_enabled=chassis_cmd_recv.chassis_mode==CHASSIS_ROTATE};
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED
    const uint32_t spin_now_ms = HAL_GetTick();
    bool feedback_valid = power_state_valid && power_state.registered &&
        power_state.power.status == POWER_MODEL_STATUS_OK && !power_state.numeric_error &&
        !power_state.transient.fault && power_state.online_mask == 0x0f &&
        power_state.telemetry.motor_enabled_mask == 0x0f &&
        (uint32_t)(spin_now_ms - power_state.tick_ms) <= 5;
    bool tracking = true;
    bool current_limited = power_state.base_limited;
    bool guard_limited = false;
    bool current_saturated = false;
#if CHASSIS_POWER_CONTROL_STAGE >= 2
    guard_limited = !ChassisSpinGuardAllowsIncrease(&power_state.transient);
    current_limited |= guard_limited;
#endif
    DJIMotorInstance *spin_motors[POWER_MODEL_MOTOR_COUNT] = {
        motor_lf, motor_rf, motor_lb, motor_rb,
    };
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        const float actual_rpm = power_state.telemetry.actual_speed_rpm[i];
        const float target_rpm = power_state.telemetry.target_speed_rpm[i];
        feedback_valid &= (uint32_t)(spin_now_ms - power_state.feedback_tick_ms[i]) <= 5 &&
            isfinite(actual_rpm) && isfinite(target_rpm) && isfinite(power_state.raw_current[i]);
        tracking &= fabsf(target_rpm - actual_rpm) <= CHASSIS_SPIN_TRACKING_ERROR_RPM;
        const float max_current = spin_motors[i]->motor_controller.speed_PID.MaxOut;
        current_saturated |= !isfinite(max_current) || max_current <= 0 ||
            fabsf(power_state.raw_current[i]) >= max_current;
        chassis_spin_input.actual_rpm[i]=actual_rpm;
        chassis_spin_input.target_rpm[i]=target_rpm;
        chassis_spin_input.feedback_tick_ms[i]=power_state.feedback_tick_ms[i];
        chassis_spin_input.feedback_sequence[i]=power_state.feedback_sequence[i];
    }
    current_limited |= current_saturated;
    chassis_spin_input.feedback_valid = feedback_valid && meter_valid &&
        (uint32_t)(spin_now_ms - meter_status.last_rx_tick_ms) <= 20;
    chassis_spin_input.measured_power_w = meter_status.power_w;
    chassis_spin_input.power_limited = current_limited;
    chassis_spin_input.increase_allowed = tracking && !current_limited &&
        ChassisSpinTransientAllowsIncrease(&power_state.transient);
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
    chassis_spin_input.evaluate_tracking=true;
    chassis_spin_input.now_ms=spin_now_ms;
    chassis_spin_input.power_tick_ms=meter_status.last_rx_tick_ms;
    chassis_spin_input.model_limited=power_state.base_limited;
    chassis_spin_input.current_saturated=current_saturated;
    chassis_spin_input.guard_limited=guard_limited;
    chassis_spin_input.braking=power_state.transient.brake_mask!=0;
    chassis_spin_input.meter_stale=!meter_valid ||
        (uint32_t)(spin_now_ms-meter_status.last_rx_tick_ms)>20;
    chassis_spin_input.increase_allowed=ChassisSpinTransientAllowsIncrease(&power_state.transient);
    if(chassis_budget_output.spin_phase==CHASSIS_SPIN_POWER_DEGRADED)
        chassis_spin_input.increase_allowed=false;
#endif
#endif
#endif
#ifdef CHASSIS_BOARD
    pending_power_control.autotune_permitted=ATYPE_BENCH_MODE &&
        VehicleSafetyAllowsAutotune(HAL_GetTick()) && VehicleSafetyAuxStopped(HAL_GetTick()) &&
        pending_power_control.control_enabled && chassis_cmd_recv.chassis_mode==CHASSIS_NO_FOLLOW;
#else
    RemoteControlSnapshot_s bench_remote={0};
    pending_power_control.autotune_permitted = ATYPE_BENCH_MODE && VehicleSafetyAllowsAutotune(HAL_GetTick()) && pending_power_control.control_enabled &&
        chassis_cmd_recv.chassis_mode==CHASSIS_NO_FOLLOW && RemoteControlGetSnapshot(&bench_remote) &&
        bench_remote.online && bench_remote.has_sample &&
        (uint32_t)(HAL_GetTick()-bench_remote.last_rx_tick_ms)<=100 &&
        bench_remote.control.rc.switch_left==2 && bench_remote.control.rc.switch_right==1 &&
        abs(bench_remote.control.rc.rocker_l_)<=30 && abs(bench_remote.control.rc.rocker_l1)<=30 &&
        abs(bench_remote.control.rc.rocker_r_)<=30 && abs(bench_remote.control.rc.rocker_r1)<=30 &&
        abs(bench_remote.control.rc.dial)<=30 && !bench_remote.control.key[KEY_PRESS].keys &&
        !bench_remote.control.mouse.press_l && !bench_remote.control.mouse.press_r;
#endif
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED
    chassis_spin_input.target_power_w = pending_power_control.target_power_w;
    chassis_spin_input.control_enabled = pending_power_control.control_enabled;
    chassis_spin_input.energy_recovering = chassis_budget_output.recovering;
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
    chassis_spin_input.power_ceiling_w=pending_power_control.power_ceiling_w>0 ?
        pending_power_control.power_ceiling_w : pending_power_control.nominal_budget_w;
    PrepareSpinLimits();
#endif
#endif
}

/* 机器人底盘控制核心任务 */
void ChassisTask()
{
    const uint32_t now = HAL_GetTick();
    if ((uint32_t)(now-heat_publish_tick)>=100U) {
        RefereeHeatSnapshot_s heat = {0};
        (void)RefereeGetHeatSnapshot(&heat);
        chassis_feedback_data.heat=heat;
        HeatControlData_s data = {.current_heat=heat.current_heat,
            .heat_limit=heat.heat_limit, .cooling_rate=heat.cooling_rate,
            .valid=heat.valid, .robot_state_tick_ms=heat.robot_state_tick_ms,
            .power_heat_tick_ms=heat.power_heat_tick_ms};
        PubPushMessage(heat_pub, &data);
        heat_publish_tick=now;
    }
    RuntimeTuningProcess(TUNING_POWER, ChassisTuningApplyPower);
    RuntimeTuningProcess(TUNING_SPIN, ChassisTuningApplySpin);
    // 后续增加没收到消息的处理(双板的情况)
    // 获取新的控制信息
#ifdef ONE_BOARD
    SubGetMessage(chassis_sub, &chassis_cmd_recv);
#endif
#ifdef CHASSIS_BOARD
    uint32_t command_rx_tick=0;
    const bool command_fresh=CANCommCopyFreshAt(chasiss_can_comm,&chassis_cmd_recv,sizeof chassis_cmd_recv,&command_rx_tick,10U);
    if(!command_fresh) chassis_cmd_recv=(Chassis_Ctrl_Cmd_s){.chassis_mode=CHASSIS_ZERO_FORCE};
    const uint32_t command_age=HAL_GetTick()-command_rx_tick;
    const bool aux_proof=command_fresh && chassis_cmd_recv.aux_stopped &&
        chassis_cmd_recv.aux_feedback_age_ms<=20U && command_age<=10U &&
        command_age+chassis_cmd_recv.aux_feedback_age_ms<=20U;
    VehicleSafetyInput_s safety={.input_online=command_fresh && chassis_cmd_recv.input_permitted,
        .emergency=!chassis_cmd_recv.input_permitted,
        .reset=command_fresh && chassis_cmd_recv.safety_reset,
        .bench=ATYPE_BENCH_MODE && chassis_cmd_recv.bench_permitted && aux_proof,
        .aux_stopped=aux_proof};
    VehicleSafetyPublish(&safety,HAL_GetTick());
#endif // CHASSIS_BOARD

    // Resolve current translation before evaluating candidate wheel constraints.
    ChassisResolveMotion(&chassis_cmd_recv, 0, &chassis_vx, &chassis_vy);
    UpdateChassisPowerBudget();

#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED
    float spin_target_wz = 0;
    (void)ChassisSpinUpdate(&chassis_spin_config, &chassis_spin_input,
        DWT_GetDeltaT(&chassis_spin_dwt_count), &chassis_spin_state, &spin_target_wz);
#else
    const float spin_target_wz = chassis_spin_config.fixed_wz;
#endif

    // 选择旋转策略，并按云台实测相对角将平移命令转换到底盘坐标系。
    if(chassis_cmd_recv.chassis_mode==CHASSIS_ROTATE) chassis_cmd_recv.wz=spin_target_wz;

    // 根据控制模式进行正运动学解算,计算底盘输出
    CalculateWheelSpeed();

    // 根据裁判系统的反馈数据和电容数据对输出限幅并设定闭环参考值
    LimitChassisOutput();

    // 从四轮 CAN 反馈估算底盘几何中心速度。
    EstimateSpeed();
#if CHASSIS_VELOCITY_FUSION_ENABLE
    ChassisObserveFusedVelocity(&velocity_config);
#endif

    // // 获取裁判系统数据   建议将裁判系统与底盘分离，所以此处数据应使用消息中心发送
    // // 我方颜色id小于7是红色,大于7是蓝色,注意这里发送的是对方的颜色, 0:blue , 1:red
    // chassis_feedback_data.enemy_color = referee_data->GameRobotState.robot_id > 7 ? 1 : 0;
    // // 当前只做了17mm热量的数据获取,后续根据robot_def中的宏切换双枪管和英雄42mm的情况
    // chassis_feedback_data.bullet_speed = referee_data->GameRobotState.shooter_id1_17mm_speed_limit;
    // chassis_feedback_data.rest_heat = referee_data->PowerHeatData.shooter_heat0;

    ChassisPowerSnapshot_s power_state = {0};
    const bool power_state_valid = ChassisPowerGetSnapshot(&power_state);
    ChassisPowerFeedbackUpdate(power_state_valid && power_state.registered,
                               power_state_valid ? &power_state.power : NULL);
    (void)ChassisPowerFeedbackGet(&chassis_feedback_data.power_control);

    const ChassisTelemetrySnapshot_s telemetry = {
        .valid = true,
        .tick_ms = HAL_GetTick(),
        .mode = (uint8_t)chassis_cmd_recv.chassis_mode,
        .vx_cmd = chassis_vx, .vy_cmd = chassis_vy, .wz_cmd = chassis_cmd_recv.wz,
        .offset_angle = chassis_cmd_recv.offset_angle,
        .velocity = *chassis_velocity,
        .nominal_budget_w = pending_power_control.nominal_budget_w,
        .target_power_w = pending_power_control.target_power_w,
        .buffer_control_enabled = CHASSIS_BUFFER_POWER_ENABLE != 0,
        .buffer_estimate_valid = power_state.policy.buffer_estimate_valid,
        .buffer_recovering = chassis_budget_output.recovering,
        .buffer_lower_j = chassis_budget_output.buffer_lower_j,
        .buffer_predicted_j = chassis_budget_output.buffer_predicted_j,
        .spin_phase=chassis_budget_output.spin_phase,
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED
        .spin_block_reasons=chassis_spin_state.block_reasons,
#endif
        .spin_base_power_w=chassis_budget_output.spin_base_power_w,
        .spin_buffer_request_w=chassis_budget_output.spin_buffer_request_w,
        .spin_buffer_grant_w=chassis_budget_output.spin_buffer_grant_w,
        .spin_base_confirm_s=chassis_budget_output.spin_base_confirm_s,
        .spin_filtered_power_w=chassis_budget_output.spin_filtered_power_w,
        .spin_power_ceiling_w=pending_power_control.power_ceiling_w,
        .slow_correction_w=pending_power_control.slow_correction_w,
        .spin_gain=chassis_spin_config.gain_wz_per_w_s,
        .spin_rise=chassis_spin_config.rise_wz_per_s,
        .spin_fall=chassis_spin_config.fall_wz_per_s,
        .spin_lpf_s=chassis_spin_config.power_lpf_tau_s,
        .spin_settle_s=chassis_spin_config.settle_time_s,
        .spin_soft_wz=chassis_spin_config.max_wz,
        .spin_hard_wz=chassis_spin_config.hard_wz>0 ? chassis_spin_config.hard_wz : chassis_spin_config.max_wz,
        .spin_body_hard_rps=chassis_spin_config.body_hard_rps,
        .spin_wheel_hard_rpm=chassis_spin_config.wheel_hard_rpm,
        .spin_extension_enabled=chassis_spin_config.extension_enabled,
        .spin_capability_latched=chassis_budget_output.spin_capability_latched,
        .spin_capability_release_s=chassis_budget_output.spin_capability_release_s,
#if CHASSIS_POWER_CONTROL_ENABLE && CHASSIS_SPIN_DYNAMIC_ENABLED
        .spin_effective_wz=chassis_spin_state.effective_max_wz,
        .spin_limit_state=chassis_spin_state.limit_state,
        .spin_extension_qualified=chassis_spin_state.extension_qualified,
        .spin_extension_confirm_s=chassis_spin_state.extension_confirm_s,
        .spin_power_gap_w=chassis_spin_state.power_gap_w,
        .spin_max_tracking_error_rpm=chassis_spin_state.max_tracking_error_rpm,
        .spin_body_tracking_error_rps=chassis_spin_state.body_tracking_error_rps,
        .spin_peak_wheel_rpm=chassis_spin_state.peak_wheel_rpm,
        .spin_peak_body_rps=chassis_spin_state.peak_body_rps,
#endif
    };
    taskENTER_CRITICAL();
    chassis_telemetry = telemetry;
    taskEXIT_CRITICAL();

    // 推送反馈消息
#ifdef ONE_BOARD
    PubPushMessage(chassis_pub, (void *)&chassis_feedback_data);
#endif
#ifdef CHASSIS_BOARD
    (void)RefereeGetHeatSnapshot(&chassis_feedback_data.heat);
    chassis_feedback_data.heat_robot_age_ms=HAL_GetTick()-chassis_feedback_data.heat.robot_state_tick_ms;
    chassis_feedback_data.heat_power_age_ms=HAL_GetTick()-chassis_feedback_data.heat.power_heat_tick_ms;
    CANCommSend(chasiss_can_comm, (void *)&chassis_feedback_data);
#endif // CHASSIS_BOARD
}
