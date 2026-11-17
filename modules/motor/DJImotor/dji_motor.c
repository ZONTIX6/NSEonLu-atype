#include "dji_motor.h"
#include "general_def.h"
#include "bsp_dwt.h"
#include "bsp_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "vehicle_safety.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint8_t idx = 0; // register idx,是该文件的全局电机索引,在注册时使用
/* DJI电机的实例,此处仅保存指针,内存的分配将通过电机实例初始化时通过malloc()进行 */
static DJIMotorInstance *dji_motor_instance[DJI_MOTOR_CNT] = {NULL}; // 会在control任务中遍历该指针数组进行pid计算

/**
 * @brief 由于DJI电机发送以四个一组的形式进行,故对其进行特殊处理,用6个(2can*3group)can_instance专门负责发送
 *        该变量将在 DJIMotorControl() 中使用,分组在 MotorSenderGrouping()中进行
 *
 * @note  因为只用于发送,所以不需要在bsp_can中注册
 *
 * C610(m2006)/C620(m3508):0x1ff,0x200;
 * GM6020:0x1ff,0x2ff
 * 反馈(rx_id): GM6020: 0x204+id ; C610/C620: 0x200+id
 * can1: [0]:0x1FF,[1]:0x200,[2]:0x2FF
 * can2: [3]:0x1FF,[4]:0x200,[5]:0x2FF
 */
static CANInstance sender_assignment[DJI_SENDER_GROUP_COUNT] = {
    [0] = {.txconf.StdId = 0x1ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [1] = {.txconf.StdId = 0x200, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [2] = {.txconf.StdId = 0x2ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [3] = {.txconf.StdId = 0x1ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [4] = {.txconf.StdId = 0x200, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [5] = {.txconf.StdId = 0x2ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
};

/**
 * @brief 6个用于确认是否有电机注册到sender_assignment中的标志位,防止发送空帧,此变量将在DJIMotorControl()使用
 *        flag的初始化在 MotorSenderGrouping()中进行
 */
static uint8_t sender_enable_flag[DJI_SENDER_GROUP_COUNT] = {0};
static DJIMotorSubmission_s cycle_submission;
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
static void (*speed_feedback_observer)(void *,unsigned,const DJI_Motor_Measure_s *);
static void *speed_feedback_context;
bool DJIMotorSetSpeedFeedbackObserver(
    void (*observer)(void *,unsigned,const DJI_Motor_Measure_s *),void *context)
{
    taskENTER_CRITICAL();
    const bool accepted=!observer || !speed_feedback_observer ||
        (speed_feedback_observer==observer && speed_feedback_context==context);
    if(accepted) {speed_feedback_context=context;speed_feedback_observer=observer;}
    taskEXIT_CRITICAL();
    return accepted;
}
#endif

static DJIMotorOutputGroup_s *output_group;
static DJIMotorOutputHooks_s output_hooks;

static float motor_output[DJI_MOTOR_CNT];
static float speed_reference_aps[DJI_MOTOR_CNT];
static float speed_feedback_aps[DJI_MOTOR_CNT];
static DJI_Motor_Measure_s cycle_measure[DJI_MOTOR_CNT];
static float cycle_reference[DJI_MOTOR_CNT];
static Motor_Working_Type_e cycle_stop[DJI_MOTOR_CNT];
static bool cycle_online[DJI_MOTOR_CNT];
static DJIMotorGroupControl_s cycle_control;
static uint32_t tuning_stop_since, tuning_stop_tick;
static bool tuning_stopped, tuning_stop_seen;
static bool OwnsRawCurrent(unsigned index)
{
    if(!output_group || !output_group->owns_raw_current) return false;
    for(unsigned w=0;w<4;++w) if(index==output_group->motor_indices[w]) return true;
    return false;
}

void DJIMotorResetPID(PIDInstance *pid,float measured)
{
    pid->Iout=pid->ITerm=pid->Last_ITerm=0;
    pid->Dout=pid->Last_Dout=0;
    pid->Output=pid->Last_Output=0;
    pid->Last_Measure=measured;pid->Last_Err=-measured;
    (void)DWT_GetDeltaT(&pid->DWT_CNT);
}
static void UpdateTuningStopState(uint32_t now,bool transmitted)
{
    bool stopped=idx>0 && transmitted;
    for (unsigned i=0;i<idx;++i)
        stopped &= cycle_stop[i]==MOTOR_STOP && cycle_online[i] &&
            (uint32_t)(now-cycle_measure[i].rx_tick_ms)<=20 &&
            isfinite(cycle_measure[i].speed_aps) && fabsf(cycle_measure[i].speed_aps)<=300;
    taskENTER_CRITICAL();
    if (!stopped) {tuning_stop_seen=false; tuning_stopped=false;}
    else {
        if (!tuning_stop_seen || (uint32_t)(now-tuning_stop_tick)>5)
            tuning_stop_since=now;
        tuning_stopped=(uint32_t)(now-tuning_stop_since)>=100;
        tuning_stop_seen=true;
    }
    tuning_stop_tick=now;
    taskEXIT_CRITICAL();
}
bool DJIMotorCanSaveParameters(uint32_t now)
{
    taskENTER_CRITICAL();
    bool safe=tuning_stopped && (uint32_t)(now-tuning_stop_tick)<=5;
    for (unsigned i=0;i<idx;++i) safe &= dji_motor_instance[i]->stop_flag==MOTOR_STOP;
    taskEXIT_CRITICAL();
    return safe;
}
bool DJIMotorGetStopProof(uint32_t now, uint32_t *oldest_feedback_age_ms)
{
    if(oldest_feedback_age_ms==NULL) return false;
    taskENTER_CRITICAL();
    bool safe=DJIMotorCanSaveParameters(now);
    uint32_t age=0;
    for(unsigned i=0;i<idx;++i) {
        uint32_t motor_age=now-dji_motor_instance[i]->measure.rx_tick_ms;
        if(motor_age>age) age=motor_age;
        safe &= dji_motor_instance[i]->measure.rx_sequence!=0;
    }
    *oldest_feedback_age_ms=age;
    taskEXIT_CRITICAL();
    return safe && age<=20U;
}

/**
 * @brief 根据电调/拨码开关上的ID,根据说明书的默认id分配方式计算发送ID和接收ID,
 *        并对电机进行分组以便处理多电机控制命令
 */
static void MotorSenderGrouping(DJIMotorInstance *motor, CAN_Init_Config_s *config)
{
    const uint8_t bus_offset = (uint8_t)(3 * CANBusIndex(config->can_handle));
    uint8_t motor_id = config->tx_id - 1; // 下标从零开始,先减一方便赋值
    uint8_t motor_send_num;
    uint8_t motor_grouping;

    switch (motor->motor_type)
    {
    case M2006:
    case M3508:
        if (motor_id < 4) // 根据ID分组
        {
            motor_send_num = motor_id;
            motor_grouping = bus_offset + 1;
        }
        else
        {
            motor_send_num = motor_id - 4;
            motor_grouping = bus_offset;
        }

        // 计算接收id并设置分组发送id
        config->rx_id = 0x200 + motor_id + 1;   // 把ID+1,进行分组设置
        sender_enable_flag[motor_grouping] = 1; // 设置发送标志位,防止发送空帧
        motor->message_num = motor_send_num;
        motor->sender_group = motor_grouping;
        sender_assignment[motor_grouping].can_handle = config->can_handle;

        // 检查是否发生id冲突
        for (size_t i = 0; i < idx; ++i)
        {
            if (dji_motor_instance[i]->motor_can_instance->can_handle == config->can_handle && dji_motor_instance[i]->motor_can_instance->rx_id == config->rx_id)
            {
                LOGERROR("[dji_motor] ID crash. Check in debug mode, add dji_motor_instance to watch to get more information.");
                uint16_t can_bus = (uint16_t)(CANBusIndex(config->can_handle) + 1);
                while (1) // 6020的id 1-4和2006/3508的id 5-8会发生冲突(若有注册,即1!5,2!6,3!7,4!8) (1!5!,LTC! (((不是)
                    LOGERROR("[dji_motor] id [%d], can_bus [%d]", config->rx_id, can_bus);
            }
        }
        break;

    case GM6020:
        if (motor_id < 4)
        {
            motor_send_num = motor_id;
            motor_grouping = bus_offset;
        }
        else
        {
            motor_send_num = motor_id - 4;
            motor_grouping = bus_offset + 2;
        }

        config->rx_id = 0x204 + motor_id + 1;   // 把ID+1,进行分组设置
        sender_enable_flag[motor_grouping] = 1; // 只要有电机注册到这个分组,置为1;在发送函数中会通过此标志判断是否有电机注册
        motor->message_num = motor_send_num;
        motor->sender_group = motor_grouping;
        sender_assignment[motor_grouping].can_handle = config->can_handle;

        for (size_t i = 0; i < idx; ++i)
        {
            if (dji_motor_instance[i]->motor_can_instance->can_handle == config->can_handle && dji_motor_instance[i]->motor_can_instance->rx_id == config->rx_id)
            {
                LOGERROR("[dji_motor] ID crash. Check in debug mode, add dji_motor_instance to watch to get more information.");
                uint16_t can_bus = (uint16_t)(CANBusIndex(config->can_handle) + 1);
                while (1) // 6020的id 1-4和2006/3508的id 5-8会发生冲突(若有注册,即1!5,2!6,3!7,4!8) (1!5!,LTC! (((不是)
                    LOGERROR("[dji_motor] id [%d], can_bus [%d]", config->rx_id, can_bus);
            }
        }
        break;

    default: // other motors should not be registered here
        while (1)
            LOGERROR("[dji_motor]You must not register other motors using the API of DJI motor."); // 其他电机不应该在这里注册
    }
}

/**
 * @todo  是否可以简化多圈角度的计算？
 * @brief 根据返回的can_instance对反馈报文进行解析
 *
 * @param _instance 收到数据的instance,通过遍历与所有电机进行对比以选择正确的实例
 */
#if CHASSIS_FORCE_CALIBRATION_ENABLE
static void (*feedback_observer)(const DJIMotorInstance *,int16_t,int16_t,uint32_t);
bool DJIMotorAuxiliaryStopped(DJIMotorInstance *const wheels[4],uint32_t now)
{
    bool safe=true;
    taskENTER_CRITICAL();
    for(unsigned n=0;n<idx;n++) {
        DJIMotorInstance *m=dji_motor_instance[n];bool wheel=false;
        for(unsigned w=0;w<4;w++) wheel |= m==wheels[w];
        if(wheel) continue;
        safe &= m->stop_flag==MOTOR_STOP && DaemonIsOnline(m->daemon) &&
            (m->measure.rx_sequence || m->measure.rx_cycles) && (uint32_t)(now-m->measure.rx_tick_ms)<=5 &&
            isfinite(m->measure.speed_aps) && fabsf(m->measure.speed_aps)<=120 &&
            abs(m->measure.raw_rpm)<=20;
    }
    taskEXIT_CRITICAL();return safe;
}
bool DJIMotorRegisterFeedbackObserver(void (*observer)(const DJIMotorInstance *,int16_t,int16_t,uint32_t))
{
    if(!observer || feedback_observer) return false;
    feedback_observer=observer;return true;
}
#endif
static void DecodeDJIMotor(CANInstance *_instance)
{
    // 这里对can instance的id进行了强制转换,从而获得电机的instance实例地址
    // _instance指针指向的id是对应电机instance的地址,通过强制转换为电机instance的指针,再通过->运算符访问电机的成员motor_measure,最后取地址获得指针
    uint8_t *rxbuff = _instance->rx_buff;
    DJIMotorInstance *motor = (DJIMotorInstance *)_instance->id;
    DJI_Motor_Measure_s *measure = &motor->measure; // measure要多次使用,保存指针减小访存开销

#if CHASSIS_VELOCITY_FUSION_ENABLE
    if (_instance->rx_len!=8U) return;
    const uint32_t received_tick=HAL_GetTick();
    const uint32_t received_cycles=DWT_GetCycleCount();
#endif

    DaemonReload(motor->daemon);
    motor->dt = DWT_GetDeltaT(&motor->feed_cnt);

    // 解析数据并对电流和速度进行滤波,电机的反馈报文具体格式见电机说明手册
    measure->last_ecd = measure->ecd;
    measure->ecd = ((uint16_t)rxbuff[0]) << 8 | rxbuff[1];
    measure->angle_single_round = ECD_ANGLE_COEF_DJI * (float)measure->ecd;
    measure->speed_aps = (1.0f - SPEED_SMOOTH_COEF) * measure->speed_aps +
                         RPM_2_ANGLE_PER_SEC * SPEED_SMOOTH_COEF * (float)((int16_t)(rxbuff[2] << 8 | rxbuff[3]));
    measure->real_current = (1.0f - CURRENT_SMOOTH_COEF) * measure->real_current +
                            CURRENT_SMOOTH_COEF * (float)((int16_t)(rxbuff[4] << 8 | rxbuff[5]));
    measure->temperature = rxbuff[6];

    // 多圈角度计算,前提是假设两次采样间电机转过的角度小于180°,自己画个图就清楚计算过程了
    if (measure->ecd - measure->last_ecd > 4096)
        measure->total_round--;
    else if (measure->ecd - measure->last_ecd < -4096)
        measure->total_round++;
    measure->total_angle = measure->total_round * 360 + measure->angle_single_round;
#if CHASSIS_VELOCITY_FUSION_ENABLE
    measure->rx_tick_ms=received_tick;
#else
    measure->rx_tick_ms=HAL_GetTick();
#endif
    ++measure->rx_sequence;
#if CHASSIS_FORCE_CALIBRATION_ENABLE || CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE || CHASSIS_VELOCITY_FUSION_ENABLE
    measure->raw_rpm=(int16_t)((uint16_t)rxbuff[2]<<8|rxbuff[3]);
    measure->raw_current=(int16_t)((uint16_t)rxbuff[4]<<8|rxbuff[5]);
#if CHASSIS_VELOCITY_FUSION_ENABLE
    measure->rx_cycles=received_cycles;
#else
    measure->rx_cycles=DWT_GetCycleCount();
#endif
#endif
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    if(feedback_observer) feedback_observer(motor,measure->raw_rpm,measure->raw_current,measure->rx_cycles);
#endif
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
    if(speed_feedback_observer) for(unsigned n=0;n<idx;++n) if(dji_motor_instance[n]==motor) {
        speed_feedback_observer(speed_feedback_context,n,measure);break;
    }
#endif
}

static void DJIMotorLostCallback(void *motor_ptr)
{
    DJIMotorInstance *motor = (DJIMotorInstance *)motor_ptr;
    uint16_t can_bus = (uint16_t)(CANBusIndex(motor->motor_can_instance->can_handle) + 1);
    LOGWARNING("[dji_motor] Motor lost, can bus [%d] , id [%d]", can_bus, motor->motor_can_instance->tx_id);
}

// 电机初始化,返回一个电机实例
DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *config)
{
    if (config == NULL || idx >= DJI_MOTOR_CNT ||
        CANBusIndex(config->can_init_config.can_handle) < 0 ||
        config->can_init_config.tx_id < 1 || config->can_init_config.tx_id > 8 ||
        (config->motor_type != M3508 && config->motor_type != M2006 && config->motor_type != GM6020))
        return NULL;
    DJIMotorInstance *instance = (DJIMotorInstance *)malloc(sizeof(DJIMotorInstance));
    if (instance == NULL) return NULL;
    memset(instance, 0, sizeof(DJIMotorInstance));

    // motor basic setting 电机基本设置
    instance->motor_type = config->motor_type;                         // 6020 or 2006 or 3508
    instance->motor_settings = config->controller_setting_init_config; // 正反转,闭环类型等

    // motor controller init 电机控制器初始化
    PIDInit(&instance->motor_controller.current_PID, &config->controller_param_init_config.current_PID);
    PIDInit(&instance->motor_controller.speed_PID, &config->controller_param_init_config.speed_PID);
    PIDInit(&instance->motor_controller.angle_PID, &config->controller_param_init_config.angle_PID);
    instance->motor_controller.other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
    instance->motor_controller.other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;
    instance->motor_controller.current_feedforward_ptr = config->controller_param_init_config.current_feedforward_ptr;
    instance->motor_controller.speed_feedforward_ptr = config->controller_param_init_config.speed_feedforward_ptr;
    // 后续增加电机前馈控制器(速度和电流)

    // 电机分组,因为至多4个电机可以共用一帧CAN控制报文
    MotorSenderGrouping(instance, &config->can_init_config);

    // 注册电机到CAN总线
    config->can_init_config.can_module_callback = DecodeDJIMotor; // set callback
    config->can_init_config.id = instance;                        // set id,eq to address(it is identity)
    instance->motor_can_instance = CANRegister(&config->can_init_config);

    // 注册守护线程
    Daemon_Init_Config_s daemon_config = {
        .callback = DJIMotorLostCallback,
        .owner_id = instance,
        .reload_count = 2, // 20ms未收到数据则丢失
    };
    instance->daemon = DaemonRegister(&daemon_config);

    DJIMotorEnable(instance);
    dji_motor_instance[idx++] = instance;
    return instance;
}

/* 电流只能通过电机自带传感器监测,后续考虑加入力矩传感器应变片等 */
void DJIMotorChangeFeed(DJIMotorInstance *motor, Closeloop_Type_e loop, Feedback_Source_e type)
{
    if (loop == ANGLE_LOOP)
        motor->motor_settings.angle_feedback_source = type;
    else if (loop == SPEED_LOOP)
        motor->motor_settings.speed_feedback_source = type;
    else
        LOGERROR("[dji_motor] loop type error, check memory access and func param"); // 检查是否传入了正确的LOOP类型,或发生了指针越界
}

void DJIMotorStop(DJIMotorInstance *motor)
{
    motor->stop_flag = MOTOR_STOP;
}

void DJIMotorEnable(DJIMotorInstance *motor)
{
    motor->stop_flag = MOTOR_ENALBED;
}

/* 修改电机的实际闭环对象 */
void DJIMotorOuterLoop(DJIMotorInstance *motor, Closeloop_Type_e outer_loop)
{
    motor->motor_settings.outer_loop_type = outer_loop;
}

// 设置参考值
void DJIMotorSetRef(DJIMotorInstance *motor, float ref)
{
    motor->motor_controller.pid_ref = ref;
}

bool DJIMotorFindIndex(const DJIMotorInstance *motor, uint8_t *motor_index)
{
    if (motor == NULL || motor_index == NULL) return false;
    for (uint8_t i = 0; i < idx; ++i)
    {
        if (dji_motor_instance[i] == motor)
        {
            *motor_index = i;
            return true;
        }
    }
    return false;
}

bool DJIMotorGetControlSnapshot(const DJIMotorInstance *motor, DJIMotorControlSnapshot_s *out)
{
    if (!out) return false;
    taskENTER_CRITICAL();
    *out = motor ? motor->control_snapshot : (DJIMotorControlSnapshot_s){0};
    taskEXIT_CRITICAL();
    return out->available;
}

bool DJIMotorRegisterOutputGroup(DJIMotorOutputGroup_s *group, const DJIMotorOutputHooks_s *hooks)
{
    if (group==NULL || hooks==NULL || output_group!=NULL || hooks->read_control==NULL ||
        hooks->limit==NULL || hooks->record_final==NULL || !isfinite(group->max_current_command) ||
        group->max_current_command<=0 || group->max_current_command>32767) return false;
    if(group->owns_raw_current && (!hooks->prepare_current || !hooks->validate_current)) return false;
    for(unsigned w=0;w<4;++w) {
        if(group->motor_indices[w]>=idx) return false;
        for(unsigned j=0;j<w;++j) if(group->motor_indices[w]==group->motor_indices[j]) return false;
    }
    output_hooks=*hooks;output_group=group;group->registered=true;
    if(output_hooks.policy && output_hooks.policy->registered)
        output_hooks.policy->registered(group, dji_motor_instance);
    return true;
}

static void ReadMotorCycle(void)
{
    taskENTER_CRITICAL();
    if (output_group != NULL) {
        cycle_control = output_group->control;
        output_hooks.read_control(output_hooks.context);
    }
    for (unsigned i = 0; i < idx; ++i) {
        cycle_measure[i] = dji_motor_instance[i]->measure;
        cycle_reference[i] = dji_motor_instance[i]->motor_controller.pid_ref;
        cycle_stop[i] = dji_motor_instance[i]->stop_flag;
        cycle_online[i] = DaemonIsOnline(dji_motor_instance[i]->daemon) != 0;
    }
    taskEXIT_CRITICAL();
}

// 为所有电机实例计算三环PID,发送控制报文
typedef struct {
    const DJIMotorCycle_s *cycle;
    int16_t *commands;
    bool *enabled;
} MotorEnqueueGuard_s;
static void ValidateBeforeEnqueue(void *context)
{
    MotorEnqueueGuard_s *gate=context;
    if(output_hooks.validate_current(output_hooks.context,gate->cycle,gate->commands,gate->enabled,HAL_GetTick())) return;
    for(unsigned w=0;w<4;++w) {
        const unsigned index=output_group->motor_indices[w];
        DJIMotorInstance *member=dji_motor_instance[index];
        gate->commands[index]=0;gate->enabled[index]=false;motor_output[index]=0;
        sender_assignment[member->sender_group].tx_buff[2*member->message_num]=0;
        sender_assignment[member->sender_group].tx_buff[2*member->message_num+1]=0;
    }
}
void DJIMotorControl()
{
    if(output_hooks.policy && output_hooks.policy->process) output_hooks.policy->process();
    uint8_t group, num;
    int16_t set;
    DJIMotorInstance *motor;
    Motor_Control_Setting_s *motor_setting;
    Motor_Controller_s *motor_controller;
    DJI_Motor_Measure_s *measure;
    float pid_measure, pid_ref;
    ReadMotorCycle();
    const uint32_t control_tick_ms = HAL_GetTick();
    bool speed_calculated[DJI_MOTOR_CNT] = {false};
    const DJIMotorCycle_s cycle={.count=idx,.motors=dji_motor_instance,.measure=cycle_measure,
        .current=motor_output,.actual_aps=speed_feedback_aps,.target_aps=speed_reference_aps,
        .stop=cycle_stop,.online=cycle_online,.submission=&cycle_submission};
    if(output_hooks.policy && output_hooks.policy->prepare)
        output_hooks.policy->prepare(&cycle,&cycle_control,cycle_reference);

    // 第一阶段：计算所有 DJI 电机的原始输出，暂不写入 CAN 缓冲区。
    for (size_t i = 0; i < idx; ++i)
    {
        motor = dji_motor_instance[i];
        motor_setting = &motor->motor_settings;
        motor_controller = &motor->motor_controller;
        measure = &cycle_measure[i];
        pid_ref = cycle_reference[i];
        speed_reference_aps[i] = 0.0f;
        speed_feedback_aps[i] = measure->speed_aps;
        if(OwnsRawCurrent(i)) {motor_output[i]=0;continue;}

        if(cycle_stop[i]==MOTOR_STOP || !cycle_online[i] ||
            cycle_measure[i].rx_sequence==0 ||
            (uint32_t)(HAL_GetTick()-cycle_measure[i].rx_tick_ms)>20U) {
            motor_output[i]=0;
            DJIMotorResetPID(&motor_controller->speed_PID,measure->speed_aps);
            DJIMotorResetPID(&motor_controller->angle_PID,measure->total_angle);
            DJIMotorResetPID(&motor_controller->current_PID,measure->real_current);
            continue;
        }

        if (motor_setting->motor_reverse_flag == MOTOR_DIRECTION_REVERSE)
            pid_ref *= -1;

        if ((motor_setting->close_loop_type & ANGLE_LOOP) && motor_setting->outer_loop_type == ANGLE_LOOP)
        {
            if (motor_setting->angle_feedback_source == OTHER_FEED)
                pid_measure = *motor_controller->other_angle_feedback_ptr;
            else
                pid_measure = measure->total_angle;
            pid_ref = PIDCalculate(&motor_controller->angle_PID, pid_measure, pid_ref);
        }

        if ((motor_setting->close_loop_type & SPEED_LOOP) && (motor_setting->outer_loop_type & (ANGLE_LOOP | SPEED_LOOP)))
        {
            if (motor_setting->feedforward_flag & SPEED_FEEDFORWARD)
                pid_ref += *motor_controller->speed_feedforward_ptr;

            if (motor_setting->speed_feedback_source == OTHER_FEED)
                pid_measure = *motor_controller->other_speed_feedback_ptr;
            else
                pid_measure = measure->speed_aps;

            speed_reference_aps[i] = pid_ref;
            speed_feedback_aps[i] = pid_measure;
            pid_ref = PIDCalculate(&motor_controller->speed_PID, pid_measure, pid_ref);
            speed_calculated[i] = true;
        }

        if (motor_setting->feedforward_flag & CURRENT_FEEDFORWARD)
            pid_ref += *motor_controller->current_feedforward_ptr;
        if (motor_setting->close_loop_type & CURRENT_LOOP)
            pid_ref = PIDCalculate(&motor_controller->current_PID, measure->real_current, pid_ref);

        if (motor_setting->feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE)
            pid_ref *= -1;

        motor_output[i] = pid_ref;
    }

    /* Apply late stop requests before publishing the power diagnostic snapshot. */
    for(unsigned i=0;i<idx;++i) {
        if(dji_motor_instance[i]->stop_flag==MOTOR_STOP || !VehicleSafetyAllowsDrive(HAL_GetTick())) {
            cycle_stop[i]=MOTOR_STOP;motor_output[i]=0;
        }
    }
    // 第二阶段：只修改已注册的四个底盘电机输出。
    memset(&cycle_submission,0,sizeof cycle_submission);
    float raw_output[DJI_MOTOR_CNT];
    if(output_group && output_hooks.prepare_current) output_hooks.prepare_current(output_hooks.context,&cycle);
    memcpy(raw_output,motor_output,sizeof raw_output);
    if (output_group != NULL) output_hooks.limit(output_hooks.context,&cycle);

    if(output_hooks.policy && output_hooks.policy->gate) output_hooks.policy->gate(&cycle);

    if (!VehicleSafetyAllowsDrive(HAL_GetTick())) {
        if(output_hooks.policy && output_hooks.policy->cancel) output_hooks.policy->cancel();
        for(unsigned i=0;i<idx;++i) {cycle_stop[i]=MOTOR_STOP;motor_output[i]=0;}
    }

    int16_t final_commands[DJI_MOTOR_CNT]={0};
    bool final_enabled[DJI_MOTOR_CNT]={false};
    // 第三阶段：所有 DJI 电机共用同一套 CAN 打包和发送路径。
    for (size_t i = 0; i < idx; ++i)
    {
        motor = dji_motor_instance[i];
        set = isfinite(motor_output[i]) ? (int16_t)fmaxf(-32768,fminf(32767,motor_output[i])) : 0;
        group = motor->sender_group;
        num = motor->message_num;
        final_enabled[i]=cycle_stop[i]!=MOTOR_STOP && motor->stop_flag!=MOTOR_STOP && cycle_online[i] &&
            cycle_measure[i].rx_sequence!=0 && (uint32_t)(HAL_GetTick()-cycle_measure[i].rx_tick_ms)<=20U;
        if (!final_enabled[i]) set=0;
        final_commands[i]=set;motor_output[i]=set;
        sender_assignment[group].tx_buff[2 * num] = (uint8_t)(set >> 8);         // 低八位
        sender_assignment[group].tx_buff[2 * num + 1] = (uint8_t)(set & 0x00ff); // 高八位

    }

    bool transmitted = true;
    uint8_t submitted_groups=0,successful_groups=0;
    for (size_t i = 0; i < DJI_SENDER_GROUP_COUNT; ++i)
    {
        if (sender_enable_flag[i])
        {
            bool contains_output_group=false;
            if(output_group) for(unsigned w=0;w<4;++w)
                contains_output_group |= dji_motor_instance[output_group->motor_indices[w]]->sender_group==i;
            submitted_groups |= (uint8_t)(1U<<i);
            MotorEnqueueGuard_s gate={.cycle=&cycle,.commands=final_commands,.enabled=final_enabled};
            const bool guarded=contains_output_group && output_hooks.validate_current;
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
            cycle_submission.begin_cycles[i]=DWT_GetCycleCount();
#endif
            const bool success=(guarded ? CANTransmitGuarded(&sender_assignment[i],1,ValidateBeforeEnqueue,&gate) :
                CANTransmit(&sender_assignment[i],1))!=0;
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
            cycle_submission.end_cycles[i]=DWT_GetCycleCount();
#endif
            memcpy(cycle_submission.bytes[i],sender_assignment[i].tx_buff,8);
            for(unsigned n=0;n<idx;++n) if(dji_motor_instance[n]->sender_group==i)
                cycle_submission.enabled_at_submit[n]=final_enabled[n];
            if (success) successful_groups |= (uint8_t)(1U<<i);
            transmitted &= success;
        }
    }
    cycle_submission.submitted_groups=submitted_groups;
    cycle_submission.successful_groups=successful_groups;
    /* A later group's guard can clear the shared buffers. Preserve bytes that
     * were already attempted, including failed submissions, in final history. */
    for(unsigned n=0;n<idx;++n) {
        const unsigned g=dji_motor_instance[n]->sender_group,slot=dji_motor_instance[n]->message_num;
        if(submitted_groups&(1U<<g)) {
            final_commands[n]=(int16_t)((uint16_t)cycle_submission.bytes[g][2*slot]<<8 |
                                        cycle_submission.bytes[g][2*slot+1]);
            final_enabled[n]=cycle_submission.enabled_at_submit[n];
        }
    }
    if (output_group != NULL) output_hooks.record_final(output_hooks.context,&cycle,final_commands,
        final_enabled,submitted_groups,successful_groups);
    for (unsigned i = 0; i < idx; ++i) {
        motor = dji_motor_instance[i];
        const PIDInstance *speed = &motor->motor_controller.speed_PID;
        const PIDInstance *angle = &motor->motor_controller.angle_PID;
        const unsigned group_bit = 1U << motor->sender_group;
        DJIMotorControlSnapshot_s snapshot = {.available = true, .tick_ms = control_tick_ms,
            .reference = cycle_reference[i], .command = final_commands[i],
            .flags = (final_enabled[i] ? DJI_CONTROL_ENABLED : 0U) |
                (submitted_groups & group_bit ? DJI_CONTROL_CAN_ATTEMPTED : 0U) |
                (successful_groups & group_bit ? DJI_CONTROL_CAN_ACCEPTED : 0U)};
        if (speed_calculated[i]) {
            snapshot.speed_dps = speed_feedback_aps[i];
            snapshot.speed_iout = speed->Iout;
            snapshot.flags |= DJI_CONTROL_SPEED_CALCULATED;
            if ((speed->Improve & PID_Integral_Limit) && speed->IntegralLimit > 0 &&
                fabsf(speed->Iout) >= speed->IntegralLimit)
                snapshot.flags |= DJI_CONTROL_INTEGRAL_AT_LIMIT;
            if (speed->MaxOut > 0 && fabsf(speed->Output) >= speed->MaxOut)
                snapshot.flags |= DJI_CONTROL_SPEED_AT_LIMIT;
            if ((motor->motor_settings.close_loop_type & ANGLE_LOOP) &&
                motor->motor_settings.outer_loop_type == ANGLE_LOOP &&
                angle->MaxOut > 0 && fabsf(angle->Output) >= angle->MaxOut)
                snapshot.flags |= DJI_CONTROL_ANGLE_AT_LIMIT;
        }
        taskENTER_CRITICAL();
        motor->control_snapshot = snapshot;
        taskEXIT_CRITICAL();
    }
    UpdateTuningStopState(HAL_GetTick(),transmitted);
    if(output_hooks.policy && output_hooks.policy->record)
        output_hooks.policy->record(&cycle,raw_output,transmitted);
}
