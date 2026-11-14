#include "bsp_init.h"
#include "robot.h"
#include "imu_capture_config.h"
#include "robot_def.h"
#include "runtime/robot_task.h"
#if !IMU_CAPTURE_ONLY
#include "tuning_flash.h"
#include "runtime/tuning_policy.h"
#include "jscope.h"
#endif
#include "board/board_devices.h"
#if IMU_CAPTURE_CAN_TX_ENABLE || CHASSIS_IMU_CAN_RX_ENABLE
#include "runtime/chassis_imu_can.h"
#endif

// 编译warning,提醒开发者修改机器人参数
#ifndef ROBOT_DEF_PARAM_WARNING
#define ROBOT_DEF_PARAM_WARNING
#pragma message "Check robot parameters in config/ before running; see config/README.md for tuning entries."
#endif // !ROBOT_DEF_PARAM_WARNING

#if !IMU_CAPTURE_ONLY && (defined(ONE_BOARD) || defined(CHASSIS_BOARD))
#include "chassis.h"
#endif

#if !IMU_CAPTURE_ONLY && (defined(ONE_BOARD) || defined(GIMBAL_BOARD))
#include "gimbal.h"
#include "shoot.h"
#include "robot_cmd.h"
#endif


void RobotInit()
{  
    // 关闭中断,防止在初始化过程中发生中断
    // 请不要在初始化过程中使用中断和延时函数！
    // 若必须,则只允许使用DWT_Delay()
    __disable_irq();
    
    BSPInit();
    (void)BoardDevicesInit();
#if IMU_CAPTURE_CAN_TX_ENABLE || CHASSIS_IMU_CAN_RX_ENABLE
    ChassisImuCanInit();
#endif
#if !IMU_CAPTURE_ONLY
    TuningFlashInit();
    TuningPolicyInit();
    JScopeInit();

#if defined(ONE_BOARD) || defined(GIMBAL_BOARD)
    RobotCMDInit();
    GimbalInit();
    ShootInit();
#endif

#if defined(ONE_BOARD) || defined(CHASSIS_BOARD)
    ChassisInit();
#endif
#endif /* !IMU_CAPTURE_ONLY */

    OSTaskInit(); // 创建基础任务

    // 初始化完成,开启中断
    __enable_irq();
}

void RobotTask()
{
#if !IMU_CAPTURE_ONLY
#if defined(ONE_BOARD) || defined(GIMBAL_BOARD)
    RobotCMDTask();
    GimbalTask();
    ShootTask();
#endif

#if defined(ONE_BOARD) || defined(CHASSIS_BOARD)
    ChassisTask();
#endif
#endif /* !IMU_CAPTURE_ONLY */

}
