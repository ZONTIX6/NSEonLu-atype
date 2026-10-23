// app
#include "robot_def.h"
#include "board_config.h"
#include "vehicle_safety.h"
#include "runtime_tuning.h"
#include <stdlib.h>
#include "robot_cmd.h"
// module
#include "remote_control.h"
#include "vt02_control.h"
#include "ins_task.h"
#include "master_process.h"
#include "message_center.h"
#include "general_def.h"
#include "dji_motor.h"
#include "user_lib.h"
#include "heat_control.h"
// bsp
#include "bsp_dwt.h"
#include "bsp_log.h"

// 私有宏,自动将编码器转换成角度值
#define YAW_ALIGN_ANGLE (YAW_CHASSIS_ALIGN_ECD * ECD_ANGLE_COEF_DJI) // 对齐时的角度,0-360
#define PTICH_HORIZON_ANGLE (PITCH_HORIZON_ECD * ECD_ANGLE_COEF_DJI) // pitch水平时电机的角度,0-360
#define YAW_MAX_TRACK_ERROR 60.0f

/* cmd应用包含的模块实例指针和交互信息存储*/
#ifdef GIMBAL_BOARD // 对双板的兼容,条件编译
#include "can_comm.h"
static Publisher_t *dual_heat_pub;
static CANCommInstance *cmd_can_comm; // 双板通信
#endif
#ifdef ONE_BOARD
static Publisher_t *chassis_cmd_pub;   // 底盘控制消息发布者
static Subscriber_t *chassis_feed_sub; // 底盘反馈信息订阅者
#endif                                 // ONE_BOARD

static Chassis_Ctrl_Cmd_s chassis_cmd_send;      // 发送给底盘应用的信息,包括控制信息和UI绘制相关
static Chassis_Upload_Data_s chassis_fetch_data; // 从底盘应用接收的反馈信息信息,底盘功率枪口热量与底盘运动状态等

static RC_ctrl_t rc_snapshot[2];
static RC_ctrl_t *rc_data=rc_snapshot;              // 遥控器数据,初始化时返回
static Vt02_Ctrl_t vt_snapshot;
static Vt02_Ctrl_t *vt02_data=&vt_snapshot;          // 图传键鼠数据,初始化时返回
static Vision_Recv_s *vision_recv_data; // 视觉接收数据指针,初始化时返回
// static Vision_Send_s vision_send_data;  // 视觉发送数据

static Publisher_t *gimbal_cmd_pub;            // 云台控制消息发布者
static Subscriber_t *gimbal_feed_sub;          // 云台反馈信息订阅者
static Gimbal_Ctrl_Cmd_s gimbal_cmd_send;      // 传递给云台的控制信息
static Gimbal_Upload_Data_s gimbal_fetch_data; // 从云台获取的反馈信息

static Publisher_t *shoot_cmd_pub;           // 发射控制消息发布者
static Subscriber_t *shoot_feed_sub;         // 发射反馈信息订阅者
static Shoot_Ctrl_Cmd_s shoot_cmd_send;      // 传递给发射的控制信息
static Shoot_Upload_Data_s shoot_fetch_data; // 从发射获取的反馈信息
static HeatControlInstance heat_control;

static Robot_Status_e robot_state; // 机器人整体工作状态

void RobotCMDInit()
{
    (void)RemoteControlInit(BOARD_REMOTE_UART);   // 修改为对应串口,注意如果是自研板dbus协议串口需选用添加了反相器的那个
    vision_recv_data = VisionInit(BOARD_VISION_UART); // 视觉通信串口
    (void)Vt02ControlInit(BOARD_VTX_UART);   // 图传键鼠通信串口


    gimbal_cmd_pub = PubRegister("gimbal_cmd", sizeof(Gimbal_Ctrl_Cmd_s));
    gimbal_feed_sub = SubRegister("gimbal_feed", sizeof(Gimbal_Upload_Data_s));
    shoot_cmd_pub = PubRegister("shoot_cmd", sizeof(Shoot_Ctrl_Cmd_s));
    shoot_feed_sub = SubRegister("shoot_feed", sizeof(Shoot_Upload_Data_s));
    HeatControl_Init_Config_s heat_config = {
        .bullet_heat = HEAT_CONTROL_BULLET_HEAT,
        .max_shoot_rate = HEAT_CONTROL_DEFAULT_MAX_RATE,
    };
    HeatControlInit(&heat_control, &heat_config);

#ifdef ONE_BOARD // 双板兼容
    chassis_cmd_pub = PubRegister("chassis_cmd", sizeof(Chassis_Ctrl_Cmd_s));
    chassis_feed_sub = SubRegister("chassis_feed", sizeof(Chassis_Upload_Data_s));
#endif // ONE_BOARD
#ifdef GIMBAL_BOARD
    CANComm_Init_Config_s comm_conf = {
        .can_config = {
            .can_handle = &hcan2,
            .tx_id = 0x312,
            .rx_id = 0x311,
        },
        .recv_data_len = sizeof(Chassis_Upload_Data_s),
        .send_data_len = sizeof(Chassis_Ctrl_Cmd_s),
    };
    comm_conf.daemon_count=10;
    cmd_can_comm = CANCommInit(&comm_conf);
    dual_heat_pub=PubRegister(HEAT_CONTROL_DATA_TOPIC,sizeof(HeatControlData_s));
#endif // GIMBAL_BOARD
    gimbal_cmd_send.pitch = 0;

    robot_state = ROBOT_READY; // 启动时机器人进入工作模式,后续加入所有应用初始化完成之后再进入
}

/**
 * @brief 根据gimbal app传回的当前电机角度计算和零位的误差
 *        单圈绝对角度的范围是0~360,说明文档中有图示
 *
 */
static void CalcOffsetAngle()
{
    // 别名angle提高可读性,不然太长了不好看,虽然基本不会动这个函数
    static float angle;
    angle = gimbal_fetch_data.yaw_motor_single_round_angle; // 从云台获取的当前yaw电机单圈角度
    // 取云台相对底盘的最近角度，避免跨越0/360度时得到接近360度的误差
    chassis_cmd_send.offset_angle = theta_format(angle - YAW_ALIGN_ANGLE);
}

/**
 * @brief 对云台角度进行软件限位，防止超出机械边界(硬截断)
 *        目前仅对 pitch 做限幅，yaw 可无限旋转
 */
static void GimbalAngleLimit()
{
    gimbal_cmd_send.pitch = float_constrain(gimbal_cmd_send.pitch, PITCH_MIN_ANGLE, PITCH_MAX_ANGLE);
}

/**
 * @brief 控制输入为遥控器(调试时)的模式和控制量设置
 *
 */
static void RemoteControlSet()
{
    // 控制底盘和云台运行模式,云台待添加,云台是否始终使用IMU数据?
    if (switch_is_down(rc_data[TEMP].rc.switch_right)) // 右侧开关状态[下],小陀螺
    {
        chassis_cmd_send.chassis_mode = CHASSIS_ROTATE;
        gimbal_cmd_send.gimbal_mode = GIMBAL_GYRO_MODE;
    }
    else if (switch_is_mid(rc_data[TEMP].rc.switch_right)) // 右侧开关状态[中],随动
    {
        chassis_cmd_send.chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;
        gimbal_cmd_send.gimbal_mode = GIMBAL_GYRO_MODE;
    }
    else if (switch_is_up(rc_data[TEMP].rc.switch_right)) // 右侧开关状态[上],初始状态
    {
        chassis_cmd_send.chassis_mode = CHASSIS_NO_FOLLOW;  //CHASSIS_ZERO_FORCE;
        gimbal_cmd_send.gimbal_mode = GIMBAL_GYRO_MODE;     //GIMBAL_ZERO_FORCE;
    }
    // 云台参数,确定云台控制数据
    // 视觉控制尚未实现，中/下档均保留人工摇杆控制，不让目标标志屏蔽输入。
    if (switch_is_down(rc_data[TEMP].rc.switch_left) || switch_is_mid(rc_data[TEMP].rc.switch_left))
    { // 按照摇杆的输出大小进行角度增量,增益系数需调整
        gimbal_cmd_send.yaw -= 0.005f * (float)rc_data[TEMP].rc.rocker_l_;
        gimbal_cmd_send.pitch += 0.001f * (float)rc_data[TEMP].rc.rocker_l1;
    }
    // 云台软件限位

    // 底盘参数,目前没有加入小陀螺(调试似乎暂时没有必要),系数需要调整
    chassis_cmd_send.vx = -40.0f * (float)rc_data[TEMP].rc.rocker_r1; // 右摇杆竖直方向：前进/后退
    chassis_cmd_send.vy = 40.0f * (float)rc_data[TEMP].rc.rocker_r_; // 右摇杆水平方向：左/右横移

    // 发射参数
    // if (switch_is_up(rc_data[TEMP].rc.switch_right)) // 右侧开关状态[上],弹舱打开
    //     ;                                            // 弹舱舵机控制,待添加servo_motor模块,开启
    // else
    //     ; // 弹舱舵机控制,待添加servo_motor模块,关闭

    // 摩擦轮控制,拨轮向上打为负,向下为正
    if (rc_data[TEMP].rc.dial < -100) // 向上超过100,打开摩擦轮
        shoot_cmd_send.friction_mode = FRICTION_ON;
    else
        shoot_cmd_send.friction_mode = FRICTION_OFF;
    // 拨弹控制,遥控器固定为一种拨弹模式,可自行选择
    if (rc_data[TEMP].rc.dial < -200)
        shoot_cmd_send.load_mode = LOAD_BURSTFIRE;
    else
        shoot_cmd_send.load_mode = LOAD_STOP;
}

/**
 * @brief 输入为键鼠时模式和控制量设置
 *
 */
static __attribute__((unused)) void MouseKeySet()
{
    chassis_cmd_send.vx = rc_data[TEMP].key[KEY_PRESS].w * 300 - rc_data[TEMP].key[KEY_PRESS].s * 300; // 系数待测
    chassis_cmd_send.vy = rc_data[TEMP].key[KEY_PRESS].d * 300 - rc_data[TEMP].key[KEY_PRESS].a * 300;

    gimbal_cmd_send.yaw += (float)rc_data[TEMP].mouse.x / 660 * 10; // 系数待测
    gimbal_cmd_send.pitch += (float)rc_data[TEMP].mouse.y / 660 * 10;

    switch (rc_data[TEMP].key_count[KEY_PRESS][Key_Z] % 3) // Z键设置弹速
    {
    case 0:
        shoot_cmd_send.bullet_speed = 15;
        break;
    case 1:
        shoot_cmd_send.bullet_speed = 18;
        break;
    default:
        shoot_cmd_send.bullet_speed = 30;
        break;
    }
    switch (rc_data[TEMP].key_count[KEY_PRESS][Key_E] % 4) // E键设置发射模式
    {
    case 0:
        shoot_cmd_send.load_mode = LOAD_STOP;
        break;
    case 1:
        shoot_cmd_send.load_mode = LOAD_1_BULLET;
        break;
    case 2:
        shoot_cmd_send.load_mode = LOAD_3_BULLET;
        break;
    default:
        shoot_cmd_send.load_mode = LOAD_BURSTFIRE;
        break;
    }
    switch (rc_data[TEMP].key_count[KEY_PRESS][Key_R] % 2) // R键开关弹舱
    {
    case 0:
        shoot_cmd_send.lid_mode = LID_OPEN;
        break;
    default:
        shoot_cmd_send.lid_mode = LID_CLOSE;
        break;
    }
    switch (rc_data[TEMP].key_count[KEY_PRESS][Key_F] % 2) // F键开关摩擦轮
    {
    case 0:
        shoot_cmd_send.friction_mode = FRICTION_OFF;
        break;
    default:
        shoot_cmd_send.friction_mode = FRICTION_ON;
        break;
    }
    switch (rc_data[TEMP].key_count[KEY_PRESS][Key_C] % 4) // C键设置底盘速度
    {
    case 0:
        chassis_cmd_send.chassis_speed_buff = 40;
        break;
    case 1:
        chassis_cmd_send.chassis_speed_buff = 60;
        break;
    case 2:
        chassis_cmd_send.chassis_speed_buff = 80;
        break;
    default:
        chassis_cmd_send.chassis_speed_buff = 100;
        break;
    }
    switch (rc_data[TEMP].key[KEY_PRESS].shift) // 待添加 按shift允许超功率 消耗缓冲能量
    {
    case 1:

        break;

    default:

        break;
    }
}

/**
 * @brief 控制输入为图传的模式和控制量设置
 *
 */
static void VideoTransmissionControlSet()
{
    chassis_cmd_send.chassis_mode = CHASSIS_NO_FOLLOW;
    gimbal_cmd_send.gimbal_mode = GIMBAL_GYRO_MODE;

    chassis_cmd_send.vx = -vt02_data->key[KEY_PRESS].w * 3000 + vt02_data->key[KEY_PRESS].s * 3000;
    chassis_cmd_send.vy = -vt02_data->key[KEY_PRESS].d * 3000 + vt02_data->key[KEY_PRESS].a * 3000;

    gimbal_cmd_send.yaw -= (float)vt02_data->mouse.x / 660 * 10;
    gimbal_cmd_send.pitch += (float)vt02_data->mouse.y / 660 * 10;

    switch (vt02_data->key_count[KEY_PRESS][Key_Z] % 3)   // 设置摩擦轮转速
    {
    case 0:
        shoot_cmd_send.bullet_speed = SMALL_AMU_15;
        break;
    case 1:
        shoot_cmd_send.bullet_speed = SMALL_AMU_18;
        break;
    default:
        shoot_cmd_send.bullet_speed = SMALL_AMU_30;
        break;
    }
    switch (vt02_data->key_count[KEY_PRESS][Key_E] % 4)   // 设置拨弹盘转动角度——射击模式
    {
    case 0:
        shoot_cmd_send.load_mode = LOAD_STOP;
        break;
    case 1:
        // 单发在左键保持期间持续发布,由shoot应用锁存到目标角度完成,防止短点击丢失
        shoot_cmd_send.load_mode = vt02_data->mouse.press_l ? LOAD_1_BULLET : LOAD_STOP;
        break;
    case 2:
        // 三连发在左键保持期间持续发布,由shoot应用锁存到目标角度完成,防止短点击丢失
        shoot_cmd_send.load_mode = vt02_data->mouse.press_l ? LOAD_3_BULLET : LOAD_STOP;
        break;
    default:
        // 连射时左键按下运行,松开停止
        shoot_cmd_send.load_mode = vt02_data->mouse.press_l ? LOAD_BURSTFIRE : LOAD_STOP;
        break;
    }
    switch (vt02_data->key_count[KEY_PRESS][Key_R] % 2)     // 开启/关闭弹仓
    {
    case 0:
        shoot_cmd_send.lid_mode = LID_OPEN;
        break;
    default:
        shoot_cmd_send.lid_mode = LID_CLOSE;
        break;
    }
    switch (vt02_data->key_count[KEY_PRESS][Key_F] % 2)     //开启摩擦轮
    {
    case 0:
        shoot_cmd_send.friction_mode = FRICTION_OFF;
        break;
    default:
        shoot_cmd_send.friction_mode = FRICTION_ON;
        break;
    }
    switch (vt02_data->key_count[KEY_PRESS][Key_C] % 4)     // 设置底盘速度
    {
    case 0:
        chassis_cmd_send.chassis_speed_buff = 40;
        break;
    case 1:
        chassis_cmd_send.chassis_speed_buff = 60;
        break;
    case 2:
        chassis_cmd_send.chassis_speed_buff = 80;
        break;
    default:
        chassis_cmd_send.chassis_speed_buff = 100;
        break;
    }
    const float speed_scale = chassis_cmd_send.chassis_speed_buff / 100.0f;
    chassis_cmd_send.vx *= speed_scale;
    chassis_cmd_send.vy *= speed_scale;
    switch (vt02_data->key_count[KEY_PRESS][Key_X] % 4)
    {
    case 0:
        chassis_cmd_send.chassis_mode = CHASSIS_ZERO_FORCE;
        break;
    case 1:
        chassis_cmd_send.chassis_mode = CHASSIS_NO_FOLLOW;
        break;
    case 2:
        chassis_cmd_send.chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;
        break;
    default:
        chassis_cmd_send.chassis_mode = CHASSIS_ROTATE;
        break;
    }
    switch (vt02_data->key[KEY_PRESS].shift)                // 待添加 按shift允许超功率 消耗缓冲能量
    {
    case 1:

        break;

    default:

        break;
    }
}
static void GimbalCommandLimit()
{
    float yaw_feedback = gimbal_fetch_data.gimbal_imu_data.YawTotalAngle;
    float yaw_error = gimbal_cmd_send.yaw - yaw_feedback;

    yaw_error = float_constrain(yaw_error,
                                -YAW_MAX_TRACK_ERROR,
                                 YAW_MAX_TRACK_ERROR);

    gimbal_cmd_send.yaw = yaw_feedback + yaw_error;

    gimbal_cmd_send.pitch =
        float_constrain(gimbal_cmd_send.pitch,
                        PITCH_MIN_ANGLE,
                        PITCH_MAX_ANGLE);
}

/**
 * @brief  紧急停止,包括遥控器左上侧拨轮打满/重要模块离线/双板通信失效等
 *         停止的阈值'300'待修改成合适的值,或改为开关控制.
 *
 * @todo   后续修改为遥控器离线则电机停止(关闭遥控器急停),通过给遥控器模块添加daemon实现
 *
 */
static void EmergencyHandler(bool board_online)
{
    RemoteControlSnapshot_s remote={0};
    bool online=RemoteControlGetSnapshot(&remote) && remote.online && remote.has_sample &&
        (uint32_t)(HAL_GetTick()-remote.last_rx_tick_ms)<=100U;
    online &= INSIsHealthy(HAL_GetTick()) && board_online;
    online &= gimbal_fetch_data.yaw_feedback_valid &&
        (uint32_t)(HAL_GetTick()-gimbal_fetch_data.yaw_feedback_tick_ms)<=20U;
    const bool vtx_mode=switch_is_up(remote.control.rc.switch_left);
    Vt02_Ctrl_t vtx={0};
    if (vtx_mode) online &= Vt02ControlGetSnapshot(&vtx)!=0;
    const bool neutral=(!vtx_mode || (!vtx.key[KEY_PRESS].keys &&
        !vtx.mouse.press_l && !vtx.mouse.press_r && !vtx.mouse.x && !vtx.mouse.y && !vtx.mouse.z)) &&
        !remote.control.mouse.x && !remote.control.mouse.y &&
        abs(remote.control.rc.rocker_l_)<=30 &&
        abs(remote.control.rc.rocker_l1)<=30 && abs(remote.control.rc.rocker_r_)<=30 &&
        abs(remote.control.rc.rocker_r1)<=30 && abs(remote.control.rc.dial)<=30 &&
        !remote.control.key[KEY_PRESS].keys && !remote.control.mouse.press_l &&
        !remote.control.mouse.press_r;
    VehicleSafetyInput_s input={.input_online=online,
        .emergency=remote.control.rc.dial>300,
        .reset=neutral && switch_is_up(remote.control.rc.switch_right),
        .bench=ATYPE_BENCH_MODE && neutral && switch_is_down(remote.control.rc.switch_left) &&
            switch_is_up(remote.control.rc.switch_right)};
    VehicleSafetyPublish(&input, HAL_GetTick());
    const bool drive=VehicleSafetyAllowsDrive(HAL_GetTick());
    chassis_cmd_send.input_permitted=drive;
    chassis_cmd_send.safety_reset=drive && input.reset;
    chassis_cmd_send.bench_permitted=VehicleSafetyAllowsAutotune(HAL_GetTick());
    robot_state=drive ? ROBOT_READY : ROBOT_STOP;
    if (!drive) {
        chassis_cmd_send=(Chassis_Ctrl_Cmd_s){.chassis_mode=CHASSIS_ZERO_FORCE};
        gimbal_cmd_send.gimbal_mode=GIMBAL_ZERO_FORCE;
        gimbal_cmd_send.yaw=gimbal_fetch_data.gimbal_imu_data.YawTotalAngle;
        gimbal_cmd_send.pitch=gimbal_fetch_data.gimbal_imu_data.Pitch;
    }
    if (!drive || ATYPE_BENCH_MODE || RuntimeTuningTrialActive()) {
        gimbal_cmd_send.gimbal_mode=GIMBAL_ZERO_FORCE;
        shoot_cmd_send=(Shoot_Ctrl_Cmd_s){.shoot_mode=SHOOT_OFF,
            .load_mode=LOAD_STOP,.friction_mode=FRICTION_OFF};
    } else shoot_cmd_send.shoot_mode=SHOOT_ON;
}

/* 机器人核心控制任务,200Hz频率运行(必须高于视觉发送频率) */
void RobotCMDTask()
{
    bool board_online=true;
    RemoteControlSnapshot_s remote={0};
    (void)RemoteControlGetSnapshot(&remote);
    rc_snapshot[TEMP]=remote.control;
    (void)Vt02ControlGetSnapshot(&vt_snapshot);

   // BMI088Acquire(bmi088_test,&bmi088_data) ;
    // 从其他应用获取回传数据
#ifdef ONE_BOARD
    SubGetMessage(chassis_feed_sub, (void *)&chassis_fetch_data);
#endif // ONE_BOARD
#ifdef GIMBAL_BOARD
    uint32_t chassis_rx_tick=0;
    const bool chassis_fresh=CANCommCopyFreshAt(cmd_can_comm,&chassis_fetch_data,sizeof chassis_fetch_data,&chassis_rx_tick,10U);
    board_online=chassis_fresh;
    if(!chassis_fresh) chassis_fetch_data=(Chassis_Upload_Data_s){0};
    const RefereeHeatSnapshot_s *heat=&chassis_fetch_data.heat;
    HeatControlData_s data={.current_heat=heat->current_heat,.heat_limit=heat->heat_limit,
        .cooling_rate=heat->cooling_rate,.valid=chassis_fresh && heat->valid,
        .robot_state_tick_ms=chassis_rx_tick-chassis_fetch_data.heat_robot_age_ms,
        .power_heat_tick_ms=chassis_rx_tick-chassis_fetch_data.heat_power_age_ms};
    PubPushMessage(dual_heat_pub,&data);
#endif // GIMBAL_BOARD
    SubGetMessage(shoot_feed_sub, &shoot_fetch_data);
    SubGetMessage(gimbal_feed_sub, &gimbal_fetch_data);

    // 根据gimbal的反馈值计算云台和底盘正方向的夹角,不需要传参,通过static私有变量完成
    CalcOffsetAngle();
    // 根据遥控器左侧开关,确定当前使用的控制模式为遥控器调试还是键鼠
    if (switch_is_down(rc_data[TEMP].rc.switch_left) || switch_is_mid(rc_data[TEMP].rc.switch_left))
        RemoteControlSet();
    else if (switch_is_up(rc_data[TEMP].rc.switch_left)) // 遥控器左侧开关状态为[上],图传控制
        VideoTransmissionControlSet();

    GimbalAngleLimit(); // 对云台角度进行软件限位,防止超出机械边界
    GimbalCommandLimit();

    EmergencyHandler(board_online); // 处理模块离线和遥控器急停等紧急情况

    /* 热量按裁判系统 10 Hz 结算，模块内部从消息中心读取最新热量数据。 */
    shoot_cmd_send.shoot_rate = HeatControlCalculate(
        &heat_control,
        shoot_cmd_send.shoot_mode == SHOOT_ON &&
            shoot_cmd_send.friction_mode == FRICTION_ON &&
            shoot_cmd_send.load_mode == LOAD_BURSTFIRE);

    // 设置视觉发送数据,还需增加加速度和角速度数据
    // VisionSetFlag(chassis_fetch_data.enemy_color,,chassis_fetch_data.bullet_speed)

    // 推送消息,双板通信,视觉通信等
    // 其他应用所需的控制数据在remotecontrolsetmode和mousekeysetmode中完成设置
#ifdef ONE_BOARD
    PubPushMessage(chassis_cmd_pub, (void *)&chassis_cmd_send);
#endif // ONE_BOARD
#ifdef GIMBAL_BOARD
    uint32_t aux_age=0;
    chassis_cmd_send.aux_stopped=DJIMotorGetStopProof(HAL_GetTick(),&aux_age);
    chassis_cmd_send.aux_feedback_age_ms=aux_age;
    CANCommSend(cmd_can_comm, (void *)&chassis_cmd_send);
#endif // GIMBAL_BOARD
    PubPushMessage(shoot_cmd_pub, (void *)&shoot_cmd_send);
    PubPushMessage(gimbal_cmd_pub, (void *)&gimbal_cmd_send);
}
