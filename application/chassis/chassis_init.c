#include "chassis_internal.h"
#include "../../config/01_pid/pid_config.h"
#include "dji_power_adapter.h"
#include "../../config/20_chassis_power/chassis_power_defaults.h"
#include "chassis_power_config.h"
#include "chassis_energy_config.h"
#include "board_config.h"
#include "referee_task.h"
#include "referee_UI.h"
#include "super_cap.h"
#include "heat_control.h"
#include "runtime_tuning.h"
#include "bsp_dwt.h"
#if POWER_METER_ENABLE
#include "power_meter.h"
#endif

#ifdef CHASSIS_BOARD
#include "ins_task.h"
CANCommInstance *chasiss_can_comm;
attitude_t *Chassis_IMU_data;
#endif
#ifdef ONE_BOARD
Publisher_t *chassis_pub;
Subscriber_t *chassis_sub;
#endif
static Referee_Interactive_info_t ui_data;
DJIMotorInstance *motor_lf, *motor_rf, *motor_lb, *motor_rb;
bool chassis_power_ready;
static bool chassis_power_feedback_ready;
Publisher_t *heat_pub;

void ChassisInit()
{
    heat_pub = PubRegister(HEAT_CONTROL_DATA_TOPIC, sizeof(HeatControlData_s));
    ChassisTuningRegister();
    RuntimeTuningRestore(TUNING_POWER, ChassisTuningApplyPower);
    RuntimeTuningRestore(TUNING_SPIN, ChassisTuningApplySpin);
    // 四个轮子的参数一样,改tx_id和反转标志位即可
    Motor_Init_Config_s chassis_motor_config = {
        .can_init_config.can_handle = &hcan1,
        .controller_param_init_config = {
            .speed_PID = pid_chassis_speed_config,
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

    ChassisPower_Init_Config_s power_config=ChassisPowerSpeedDefaultConfig();
    power_config.tuning_source=RuntimeTuningSource(TUNING_POWER);

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

