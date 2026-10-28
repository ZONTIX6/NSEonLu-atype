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

#include "chassis.h"
#include "chassis_motion.h"
#include "chassis_kinematics.h"
#include "chassis_power_budget.h"
#include "chassis_energy_config.h"
#include "chassis_power_outer_loop.h"
#include "chassis_power.h"
#include "dji_power_adapter.h"
#include "chassis_power_config.h"
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

/* 根据robot_def.h中的macro自动计算的参数 */
#define HALF_WHEEL_BASE (WHEEL_BASE / 2.0f)     // 半轴距
#define HALF_TRACK_WIDTH (TRACK_WIDTH / 2.0f)   // 半轮距
#define PERIMETER_WHEEL (RADIUS_WHEEL * 2 * PI) // 轮子周长
#define CHASSIS_POWER_SAFETY_FACTOR 0.98f   // 功率预算安全系数,用于降低功率预算,避免功率计和底盘功率模型的误差导致底盘超功率

/* 底盘应用包含的模块和信息存储,底盘是单例模式,因此不需要为底盘建立单独的结构体 */
#ifdef CHASSIS_BOARD // 如果是底盘板,使用板载IMU获取底盘转动角速度
#include "can_comm.h"
#include "ins_task.h"
static CANCommInstance *chasiss_can_comm; // 双板通信CAN comm
attitude_t *Chassis_IMU_data;
#endif // CHASSIS_BOARD
#ifdef ONE_BOARD
static Publisher_t *chassis_pub;                    // 用于发布底盘的数据
static Subscriber_t *chassis_sub;                   // 用于订阅底盘的控制命令
#endif                                              // !ONE_BOARD
static Chassis_Ctrl_Cmd_s chassis_cmd_recv;         // 底盘接收到的控制命令
static Chassis_Upload_Data_s chassis_feedback_data; // 底盘回传的反馈数据

static Referee_Interactive_info_t ui_data; // UI数据，将底盘中的数据传入此结构体的对应变量中，UI会自动检测是否变化，对应显示UI

 DJIMotorInstance *motor_lf, *motor_rf, *motor_lb, *motor_rb; // left right forward back
static bool chassis_power_ready;
static bool chassis_power_feedback_ready;
static ChassisPowerBudgetOutput_s chassis_budget_output;// 底盘功率预算输出
static uint32_t chassis_budget_dwt_count;
static ChassisPowerControlSnapshot_s pending_power_control;
static ChassisPower_Init_Config_s power_module_config;
static uint32_t power_config_version;
static ChassisTelemetrySnapshot_s chassis_telemetry;
static ChassisVelocityFeedback_s chassis_velocity_data;
const ChassisVelocityFeedback_s *const volatile chassis_velocity = &chassis_velocity_data;
static Publisher_t *heat_pub;
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

#if CHASSIS_SPIN_DYNAMIC_ENABLED
static ChassisSpinState_s chassis_spin_state;
static ChassisSpinInput_s chassis_spin_input;
static uint32_t chassis_spin_dwt_count;
#endif
static ChassisSpinConfig_s chassis_spin_config = {
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE
    .fixed_wz = CHASSIS_SPIN_SINE_CENTER_WZ,
    .min_wz = CHASSIS_SPIN_SINE_MIN_WZ,
#else
    .fixed_wz = CHASSIS_SPIN_FIXED_WZ,
    .min_wz = CHASSIS_SPIN_MIN_WZ,
#endif
    .max_wz = CHASSIS_SPIN_MAX_WZ,
    .gain_wz_per_w_s = CHASSIS_SPIN_POWER_GAIN,
    .rise_wz_per_s = CHASSIS_SPIN_RISE_RATE,
    .fall_wz_per_s = CHASSIS_SPIN_FALL_RATE,
    .power_lpf_tau_s = CHASSIS_SPIN_POWER_LPF_TAU_S,
    .power_deadband_w = CHASSIS_SPIN_POWER_DEADBAND_W,
    .settle_time_s = CHASSIS_SPIN_SETTLE_TIME_S,
    .budget_drop_reset_w = CHASSIS_SPIN_BUDGET_DROP_RESET_W,
    .sine_amplitude_wz = CHASSIS_SPIN_SINE_AMPLITUDE_WZ,
    .sine_period_s = CHASSIS_SPIN_SINE_PERIOD_S,
    .sine_ramp_s = CHASSIS_SPIN_SINE_RAMP_S,
    .sine_rise_wz_per_s = CHASSIS_SPIN_SINE_RISE_RATE,
    .sine_fall_wz_per_s = CHASSIS_SPIN_SINE_FALL_RATE,
    .sine_direction = CHASSIS_SPIN_SINE_DIRECTION,
    .extension_enabled=CHASSIS_SPIN_EXTENSION_ENABLE!=0,
    .hard_wz=CHASSIS_SPIN_HARD_WZ,
    .body_hard_rps=CHASSIS_SPIN_BODY_HARD_RPS,
    .wheel_hard_rpm=CHASSIS_SPIN_WHEEL_HARD_RPM,
    .extension_rise_wz_per_s=CHASSIS_SPIN_EXTENSION_RISE_RATE,
};

static ChassisPowerBudgetConfig_s chassis_budget_config = {
    .empty_energy_ratio = 0.10f,
    .full_energy_ratio = 0.30f,
    .max_boost_w = 100.0f,
    .rise_rate_w_per_s = 200.0f,
    .demand_margin_w = CHASSIS_POWER_DEMAND_MARGIN_W,
    .buffer = {
        .max_energy_j = CHASSIS_BUFFER_MAX_ENERGY_J,
        .recovery_enter_j = CHASSIS_BUFFER_RECOVERY_ENTER_J,
        .recovery_exit_j = CHASSIS_BUFFER_RECOVERY_EXIT_J,
        .hard_floor_j = CHASSIS_BUFFER_HARD_FLOOR_J,
        .prediction_window_s = CHASSIS_BUFFER_PREDICTION_WINDOW_S,
        .guard_window_s = CHASSIS_BUFFER_GUARD_WINDOW_S,
        .settlement_delay_s = CHASSIS_BUFFER_SETTLEMENT_DELAY_S,
        .energy_margin_j = CHASSIS_BUFFER_ENERGY_MARGIN_J,
        .power_margin_w = CHASSIS_BUFFER_POWER_MARGIN_W,
        .max_boost_w = CHASSIS_BUFFER_BOOST_MAX_W,
        .recovery_power_w = CHASSIS_BUFFER_RECOVERY_POWER_W,
        .recovery_target_time_s = CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S,
        .observer_max_step_s = CHASSIS_BUFFER_OBSERVER_MAX_STEP_S,
        .recovery_hold_s = CHASSIS_BUFFER_RECOVERY_HOLD_S,
        .rise_rate_w_per_s = CHASSIS_BUFFER_RISE_RATE_W_PER_S,
        .safety_factor = CHASSIS_POWER_SAFETY_FACTOR,
        .correction_rise_j_per_s = CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S,
    },
};

/* 功率外环:用功率计实测功率对比模型预测值,在"基准预算"上叠加一个有界的修正量 */
static ChassisPowerOuterLoopConfig_s chassis_outer_loop_config = {
    .kp = 1.0f, // 比例增益,误差 = 目标功率 - 实测功率(W),输出为预算修正量(W)
    .ki = 1.5f, // 积分增益,积分限幅为[-基准预算, max_positive_correction_w]
    .measurement_lpf_tau_s = 0.030f, // 功率计读数一阶低通时间常数(s),抑制采样噪声
    .activation_margin_w = 5.0f, // 介入阈值(W):需求或实测功率距目标不到此值时才进入ACTIVE
    .error_deadband_w = 0.3f, // 误差死区(W):|误差|不超过此值时按0处理,停止积分
    .max_positive_correction_w = 8.0f, // 修正量上限(W):最多只上调8W;下限为-基准预算,即最狠可压到0
    .positive_slew_rate_w_per_s = 60.0f, // 修正量上升/回零速率(W/s);下调不限速,立即生效
    .budget_drop_reset_w = 1.0f, // 基准预算单周期跌落超过此值(W)时,清零正向积分与修正量
    .antiwindup_gain_per_s = 10,
    .low_demand_delay_s = .5f,
    .low_demand_decay_w_per_s = 1,
};

static const MotorPowerModelConfig_s m3508_power_model = {
    .k0 = 1.3123816,
    .k1 = -0.25836727,
    .k2 = 0.0007424524,
    .k3 = 0.0024271158,
    .k4 = 0.081198552,
    .k5 = 3.890596e-08,
    .current_conversion = 1000.0f,// 电流单位转换,功率模型使用A,电机模块使用mA
};

/* 用于自旋变速策略的时间变量 */
// static float t;

/* 私有函数计算的中介变量,设为静态避免参数传递的开销 */
static float chassis_vx, chassis_vy;                      // 将云台系的速度投影到底盘
static float vt_lf, vt_rf, vt_lb, vt_rb;                  // 底盘速度解算后的临时输出,待进行限幅

static void RegisterChassisTuning(void)
{
    const ChassisPowerOuterLoopConfig_s *o = &chassis_outer_loop_config;
    const ChassisPowerBudgetConfig_s *b = &chassis_budget_config;
    const float power[] = {o->kp, o->ki, o->measurement_lpf_tau_s,
        o->activation_margin_w, o->error_deadband_w, o->max_positive_correction_w,
        o->positive_slew_rate_w_per_s, o->budget_drop_reset_w, o->antiwindup_gain_per_s,
        o->low_demand_delay_s, o->low_demand_decay_w_per_s,
        b->max_boost_w, b->rise_rate_w_per_s, b->demand_margin_w,
        b->empty_energy_ratio, b->full_energy_ratio, b->buffer.max_boost_w,
        b->buffer.recovery_power_w, b->buffer.recovery_target_time_s};
    RuntimeTuningRegister(TUNING_POWER, power, CHASSIS_POWER_CONTROL_STAGE);
    const ChassisSpinConfig_s *s = &chassis_spin_config;
    const float spin[] = {s->fixed_wz, s->min_wz, s->max_wz, s->gain_wz_per_w_s,
        s->rise_wz_per_s, s->fall_wz_per_s, s->power_lpf_tau_s, s->power_deadband_w,
        s->settle_time_s, s->budget_drop_reset_w, s->sine_amplitude_wz,
        s->sine_period_s, s->sine_ramp_s, s->sine_rise_wz_per_s,
        s->sine_fall_wz_per_s, (float)s->sine_direction};
    RuntimeTuningRegister(TUNING_SPIN, spin, CHASSIS_SPIN_MODE);
}
static bool ApplyPowerTuning(const float *v)
{
    const float upper[] = {100, 100, 10, 1000, 100, 1000, 10000, 1000,
        1000, 60, 1000, 1000, 10000, 1000, 1, 1, 1000, 1000, 60};
    for (unsigned i = 0; i < 19; ++i)
        if (!isfinite(v[i]) || v[i] < 0 || v[i] > upper[i]) return false;
    ChassisPowerOuterLoopConfig_s outer = {
        .kp=v[0], .ki=v[1], .measurement_lpf_tau_s=v[2],
        .activation_margin_w=v[3], .error_deadband_w=v[4],
        .max_positive_correction_w=v[5], .positive_slew_rate_w_per_s=v[6],
        .budget_drop_reset_w=v[7], .antiwindup_gain_per_s=v[8],
        .low_demand_delay_s=v[9], .low_demand_decay_w_per_s=v[10]};
    ChassisPowerBudgetConfig_s budget = chassis_budget_config;
    budget.max_boost_w=v[11]; budget.rise_rate_w_per_s=v[12];
    budget.demand_margin_w=v[13]; budget.empty_energy_ratio=v[14];
    budget.full_energy_ratio=v[15]; budget.buffer.max_boost_w=v[16];
    budget.buffer.recovery_power_w=v[17]; budget.buffer.recovery_target_time_s=v[18];
    if (!ChassisPowerOuterLoopConfigIsValid(&outer) ||
        !ChassisPowerBudgetConfigIsValid(&budget) ||
        !ChassisBufferBudgetConfigIsValid(&budget.buffer)) return false;
    if (chassis_power_ready) {
        ChassisPower_Init_Config_s next=power_module_config;
        next.outer=outer;next.budget=budget;
        if (!ChassisPowerRequestConfig(&next,power_config_version+1)) return false;
        ++power_config_version;power_module_config=next;
    }
    chassis_outer_loop_config = outer;
    chassis_budget_config = budget;
    return true;
}
static bool ApplySpinTuning(const float *v)
{
    const float upper[] = {8000,8000,8000,1000,10000,10000,10,1000,60,
        1000,6000,120,60,10000,10000};
    for (unsigned i = 0; i < 15; ++i)
        if (!isfinite(v[i]) || v[i] < 0 || v[i] > upper[i]) return false;
    if (v[15] != -1 && v[15] != 1) return false;
    ChassisSpinConfig_s spin = {
        .fixed_wz=v[0], .min_wz=v[1], .max_wz=v[2], .gain_wz_per_w_s=v[3],
        .rise_wz_per_s=v[4], .fall_wz_per_s=v[5], .power_lpf_tau_s=v[6],
        .power_deadband_w=v[7], .settle_time_s=v[8], .budget_drop_reset_w=v[9],
        .sine_amplitude_wz=v[10], .sine_period_s=v[11], .sine_ramp_s=v[12],
        .sine_rise_wz_per_s=v[13], .sine_fall_wz_per_s=v[14],
        .sine_direction=(int8_t)v[15]};
    /* Persisted/runtime payload remains exactly 16 legacy fields. Calibration
     * can only come from this firmware, never from bytes after old Flash data. */
    spin.extension_enabled=CHASSIS_SPIN_EXTENSION_ENABLE!=0;
    spin.hard_wz=CHASSIS_SPIN_HARD_WZ;
    spin.body_hard_rps=CHASSIS_SPIN_BODY_HARD_RPS;
    spin.wheel_hard_rpm=CHASSIS_SPIN_WHEEL_HARD_RPM;
    spin.extension_rise_wz_per_s=CHASSIS_SPIN_EXTENSION_RISE_RATE;
    if (!ChassisSpinConfigIsValid(&spin)) return false;
#if CHASSIS_SPIN_DYNAMIC_ENABLED
    if (chassis_cmd_recv.chassis_mode == CHASSIS_ROTATE) {
        if (spin.sine_direction != chassis_spin_config.sine_direction ||
            !ChassisSpinRetune(&spin, &chassis_spin_state)) return false;
    } else chassis_spin_state = (ChassisSpinState_s){0};
#endif
    chassis_spin_config = spin;
    return true;
}

void ChassisInit()
{
    heat_pub = PubRegister(HEAT_CONTROL_DATA_TOPIC, sizeof(HeatControlData_s));
    RegisterChassisTuning();
    RuntimeTuningRestore(TUNING_POWER, ApplyPowerTuning);
    RuntimeTuningRestore(TUNING_SPIN, ApplySpinTuning);
    // 四个轮子的参数一样,改tx_id和反转标志位即可
    Motor_Init_Config_s chassis_motor_config = {
        .can_init_config.can_handle = &hcan1,
        .controller_param_init_config = {
            .speed_PID = {
                .Kp = 4.5f, // A-board vehicle baseline, error in degrees/s
                .Ki = 0,   // 0
                .Kd = 0,   // 0
                .IntegralLimit = 3000,
                .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement,
                .MaxOut = 15000,
                .Output_LPF_RC = 0.3,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = SPEED_LOOP,
        },
        .motor_type = M3508,
    };
    //  @todo: 当前还没有设置电机的正反转,仍然需要手动添加reference的正负号,需要电机module的支持,待修改.
    chassis_motor_config.can_init_config.tx_id = 1;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    motor_lf = DJIMotorInit(&chassis_motor_config);

    chassis_motor_config.can_init_config.tx_id = 4;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    motor_rf = DJIMotorInit(&chassis_motor_config);

    chassis_motor_config.can_init_config.tx_id = 2;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    motor_lb = DJIMotorInit(&chassis_motor_config);

    chassis_motor_config.can_init_config.tx_id = 3;
    chassis_motor_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    motor_rb = DJIMotorInit(&chassis_motor_config);

    ChassisPower_Init_Config_s power_config = {
        .budget=chassis_budget_config,.outer=chassis_outer_loop_config,
        .transient=ChassisPowerTransientDefaultConfig(),.stage=CHASSIS_POWER_CONTROL_STAGE,
        .use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP,
        .meter_enabled=POWER_METER_ENABLE!=0,.buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0,
        .algorithm = {
            .safety_factor = CHASSIS_POWER_SAFETY_FACTOR,
            .small_error_threshold_rpm = 500.0f,
            .reserved_power_threshold_w = 54.0f,
            .per_motor_reserved_power_w = 8.0f,
            .max_current_command = 15000.0f,
        },
    };
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
        power_config.models[i] = m3508_power_model;

    power_module_config=power_config;
    const DJIPowerAdapterConfig_s adapter_config={.motors={motor_lf,motor_rf,motor_lb,motor_rb},
        .count=4,.max_current_command=power_config.algorithm.max_current_command};
    chassis_power_ready = ChassisPowerInit(&power_config) && DJIPowerAdapterInit(&adapter_config);
    chassis_power_feedback_ready = ChassisPowerFeedbackInit();
    if (!chassis_power_ready)
    {
        DJIMotorStop(motor_lf);
        DJIMotorStop(motor_rf);
        DJIMotorStop(motor_lb);
        DJIMotorStop(motor_rb);
    }

#if REFEREE_ENABLE
    (void)UITaskInit(BOARD_REFEREE_UART, &ui_data); // 裁判系统初始化,会同时初始化UI
#endif

#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP
    SuperCap_Init_Config_s cap_conf = {
        .can_config = {
            .can_handle = BOARD_SUPERCAP_CAN,
            .tx_id = SUPERCAP_COMMAND_CAN_ID,
            .rx_id = SUPERCAP_STATUS_CAN_ID,
        },
        .offline_reload_count = 20U,
        .input_power_feedback_enabled = CHASSIS_BUFFER_POWER_ENABLE != 0,
    };
    (void)SuperCapInit(&cap_conf); // 超级电容初始化
#endif
#if POWER_METER_ENABLE
    const PowerMeter_Init_Config_s power_meter_config = {
        .can_config = {
            .can_handle = BOARD_POWER_METER_CAN,
            .tx_id = POWER_METER_STATUS_CAN_ID,
            .rx_id = POWER_METER_STATUS_CAN_ID,
        },
        .offline_reload_count = 10U,
    };
    (void)PowerMeterInit(&power_meter_config);
#endif
    DWT_GetDeltaT(&chassis_budget_dwt_count);

    // 发布订阅初始化,如果为双板,则需要can comm来传递消息
#ifdef CHASSIS_BOARD
    Chassis_IMU_data = INS_Init(); // 底盘IMU初始化

    CANComm_Init_Config_s comm_conf = {
        .can_config = {
            .can_handle = &hcan2,
            .tx_id = 0x311,
            .rx_id = 0x312,
        },
        .recv_data_len = sizeof(Chassis_Ctrl_Cmd_s),
        .send_data_len = sizeof(Chassis_Upload_Data_s),
    };
    comm_conf.daemon_count=10;
    chasiss_can_comm = CANCommInit(&comm_conf); // can comm初始化
#endif                                          // CHASSIS_BOARD

#ifdef ONE_BOARD // 单板控制整车,则通过pubsub来传递消息
    chassis_sub = SubRegister("chassis_cmd", sizeof(Chassis_Ctrl_Cmd_s));
    chassis_pub = PubRegister("chassis_feed", sizeof(Chassis_Upload_Data_s));
#endif // ONE_BOARD
}

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

#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
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
    ChassisPowerPolicyInput_s input={.now_ms=HAL_GetTick(),
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
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
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
    ChassisPowerPolicyOutput_s output;
    (void)ChassisPowerUpdatePolicy(&input,&output);
    if(output.cap_command_valid) {
        const bool submitted=SuperCapSendCommand(&output.cap_command);
        ChassisPowerCapCommandResult(&output.cap_command,submitted,HAL_GetTick(),
            input.cap.input_received_frame_count,&output);
    }
    chassis_budget_output=output.budget;
    pending_power_control=output.control;
    pending_power_control.control_enabled &= chassis_power_ready;
#if CHASSIS_SPIN_DYNAMIC_ENABLED && POWER_METER_ENABLE
    const ChassisPowerSnapshot_s power_state=input.previous;
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
#if CHASSIS_SPIN_DYNAMIC_ENABLED
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
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
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
#if CHASSIS_SPIN_DYNAMIC_ENABLED
    chassis_spin_input.target_power_w = pending_power_control.target_power_w;
    chassis_spin_input.control_enabled = pending_power_control.control_enabled;
    chassis_spin_input.energy_recovering = chassis_budget_output.recovering;
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
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
    RuntimeTuningProcess(TUNING_POWER, ApplyPowerTuning);
    RuntimeTuningProcess(TUNING_SPIN, ApplySpinTuning);
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

#if CHASSIS_SPIN_DYNAMIC_ENABLED
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
#if CHASSIS_SPIN_DYNAMIC_ENABLED
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
#if CHASSIS_SPIN_DYNAMIC_ENABLED
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
