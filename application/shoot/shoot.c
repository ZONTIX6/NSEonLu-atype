#include "shoot.h"
#include "../../config/01_pid/pid_config.h"
#include "robot_def.h"

#include "math.h"
#include "dji_motor.h"
#include "message_center.h"
#include "bsp_dwt.h"
#include "general_def.h"
#include "heat_control.h"
#include "FreeRTOS.h"
#include "task.h"

/* 对于双发射机构的机器人,将下面的数据封装成结构体即可,生成两份shoot应用实例 */
static DJIMotorInstance *friction_l, *friction_r, *loader; // 拨盘电机
// static servo_instance *lid; 需要增加弹舱盖

static Publisher_t *shoot_pub;
static Shoot_Ctrl_Cmd_s shoot_cmd_recv; // 来自cmd的发射控制信息
static Subscriber_t *shoot_sub;
static Shoot_Upload_Data_s shoot_feedback_data; // 来自cmd的发射控制信息

// dwt定时,计算冷却用
static float loader_target_angle = 0;
static HeatControlInstance shoot_heat;

void ShootInit()
{
    // 左摩擦轮
    Motor_Init_Config_s friction_config = {
        .can_init_config = {
            .can_handle = &hcan2,
        },
        .controller_param_init_config = {
            .speed_PID = pid_shoot_friction_speed_config,
            .current_PID = pid_shoot_friction_current_config,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP,
            .close_loop_type = SPEED_LOOP | CURRENT_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_REVERSE,
        },
        .motor_type = M3508};
    friction_config.can_init_config.tx_id = 3,
    friction_l = DJIMotorInit(&friction_config);

    friction_config.can_init_config.tx_id = 4; // 右摩擦轮,改txid和方向就行
    friction_config.controller_setting_init_config.motor_reverse_flag = MOTOR_DIRECTION_NORMAL;
    friction_r = DJIMotorInit(&friction_config);

    // 拨盘电机
    Motor_Init_Config_s loader_config = {
        .can_init_config = {
            .can_handle = &hcan2,
            .tx_id = 7,
        },
        .controller_param_init_config = {
            .angle_PID = pid_shoot_loader_angle_config,
            .speed_PID = pid_shoot_loader_speed_config,
            .current_PID = pid_shoot_loader_current_config,
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SPEED_LOOP, // 初始化成SPEED_LOOP,让拨盘停在原地,防止拨盘上电时乱转
            .close_loop_type = CURRENT_LOOP | SPEED_LOOP | ANGLE_LOOP,
            .motor_reverse_flag = MOTOR_DIRECTION_NORMAL, // 注意方向设置为拨盘的拨出的击发方向
        },
        .motor_type = M2006 // 英雄使用m3508
    };
    loader = DJIMotorInit(&loader_config);

    shoot_pub = PubRegister("shoot_feed", sizeof(Shoot_Upload_Data_s));
    shoot_sub = SubRegister("shoot_cmd", sizeof(Shoot_Ctrl_Cmd_s));
    HeatControlInit(&shoot_heat, NULL);
}

/* A discrete batch must settle into fresh referee telemetry before another batch
 * or burst can start. Keep this history across stop/re-enable to avoid reusing heat. */
static float FrictionTarget(Bullet_Speed_e speed)
{
    switch (speed) {
    case SMALL_AMU_15: return SHOOT_FRICTION_SPEED_15;
    case SMALL_AMU_18: return SHOOT_FRICTION_SPEED_18;
    case SMALL_AMU_30: return SHOOT_FRICTION_SPEED_30;
    default: return SHOOT_FRICTION_SPEED_DEFAULT;
    }
}

static bool FreshMeasure(const DJI_Motor_Measure_s *m, uint32_t now)
{
    return m->rx_sequence != 0 && (uint32_t)(now-m->rx_tick_ms)<=SHOOT_MOTOR_FEEDBACK_MAX_AGE_MS &&
        isfinite(m->speed_aps) && isfinite(m->total_angle);
}

void ShootTask(void)
{
    static loader_mode_e previous_request = LOAD_STOP;
    static bool active_batch, friction_tracking, fed, discrete_pending;
    static uint32_t friction_since, last_feed_tick;
    const uint32_t now=HAL_GetTick();
    DJI_Motor_Measure_s left, right, feed;
    taskENTER_CRITICAL();
    left=friction_l->measure; right=friction_r->measure; feed=loader->measure;
    taskEXIT_CRITICAL();
    SubGetMessage(shoot_sub, &shoot_cmd_recv);
    /* Refresh heat even when the trigger is released. */
    (void)HeatControlCalculate(&shoot_heat, 0);
    const bool heat_valid=shoot_heat.data_valid && shoot_heat.heat_limit>0 &&
        (uint32_t)(now-shoot_heat.robot_state_tick_ms)<=SHOOT_HEAT_FEEDBACK_MAX_AGE_MS &&
        (uint32_t)(now-shoot_heat.power_heat_tick_ms)<=SHOOT_HEAT_FEEDBACK_MAX_AGE_MS &&
        shoot_heat.current_heat<shoot_heat.heat_limit;
    const bool on=shoot_cmd_recv.shoot_mode==SHOOT_ON;
    const float target=FrictionTarget(shoot_cmd_recv.bullet_speed);
    const bool friction_on=on && shoot_cmd_recv.friction_mode==FRICTION_ON;
    const float left_target=friction_l->motor_settings.motor_reverse_flag==MOTOR_DIRECTION_REVERSE ? -target : target;
    const float right_target=friction_r->motor_settings.motor_reverse_flag==MOTOR_DIRECTION_REVERSE ? -target : target;
    const bool tracking=friction_on && FreshMeasure(&left,now) && FreshMeasure(&right,now) &&
        fabsf(left.speed_aps-left_target)<=target*SHOOT_FRICTION_SPEED_TOLERANCE_RATIO &&
        fabsf(right.speed_aps-right_target)<=target*SHOOT_FRICTION_SPEED_TOLERANCE_RATIO;
    if (!tracking) friction_tracking=false;
    else if (!friction_tracking) { friction_tracking=true; friction_since=now; }
    const bool ready=tracking && (uint32_t)(now-friction_since)>=SHOOT_FRICTION_STABLE_TIME_MS;
    DJIMotorSetRef(friction_l,friction_on ? target : 0);
    DJIMotorSetRef(friction_r,friction_on ? target : 0);
    if (on) { DJIMotorEnable(friction_l); DJIMotorEnable(friction_r); }
    else { DJIMotorStop(friction_l); DJIMotorStop(friction_r); }

    const loader_mode_e request=shoot_cmd_recv.load_mode;
    const bool discrete=request==LOAD_1_BULLET || request==LOAD_3_BULLET;
    const bool edge=discrete && previous_request==LOAD_STOP;
    const bool completed=active_batch && FreshMeasure(&feed,now) &&
        fabsf(feed.total_angle-loader_target_angle)<=SHOOT_LOADER_ANGLE_TOLERANCE;
    if (completed) active_batch=false;
    /* Signed differences support tick wrap; require source timestamps after the
     * heat reporting horizon, not merely a locally republished old snapshot. */
    const bool settled=!fed || (!active_batch && (uint32_t)(now-last_feed_tick)>SHOOT_HEAT_SETTLE_DELAY_MS &&
        heat_valid && (int32_t)(shoot_heat.power_heat_tick_ms-last_feed_tick)>(int32_t)SHOOT_HEAT_SETTLE_DELAY_MS);
    if (settled) discrete_pending=false;
    const bool permitted=on && ready && heat_valid && FreshMeasure(&feed,now);
    if (!permitted) active_batch=false;
    if (permitted && !active_batch && edge && settled) {
        const unsigned shots=request==LOAD_3_BULLET ? 3U : 1U;
        const float required=(shots+HEAT_CONTROL_RESERVE_SHOTS)*shoot_heat.bullet_heat;
        if (shoot_heat.heat_limit-shoot_heat.current_heat>=required) {
            loader_target_angle=feed.total_angle+shots*SHOOT_LOADER_ONE_BULLET_MOTOR_ANGLE;
            active_batch=true; discrete_pending=true;
        }
    }
    if (permitted && active_batch) {
        DJIMotorEnable(loader); DJIMotorOuterLoop(loader,ANGLE_LOOP);
        DJIMotorSetRef(loader,loader_target_angle);
        fed=true; last_feed_tick=now;
    } else if (permitted && !discrete_pending && request==LOAD_BURSTFIRE &&
               isfinite(shoot_cmd_recv.shoot_rate) && shoot_cmd_recv.shoot_rate>0) {
        DJIMotorEnable(loader); DJIMotorOuterLoop(loader,SPEED_LOOP);
        DJIMotorSetRef(loader,shoot_cmd_recv.shoot_rate*360*REDUCTION_RATIO_LOADER/NUM_PER_CIRCLE);
        fed=true; last_feed_tick=now;
    } else {
        DJIMotorOuterLoop(loader,SPEED_LOOP); DJIMotorSetRef(loader,0);
        DJIMotorStop(loader);
    }
    /* A denied/cancelled press is consumed; restoring readiness never resumes it.
     * While a batch runs, require a subsequent release before accepting a new one. */
    previous_request=active_batch ? (request==LOAD_STOP ? LOAD_1_BULLET : request) : request;
    PubPushMessage(shoot_pub, &shoot_feedback_data);
}
