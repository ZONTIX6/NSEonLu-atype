#ifndef IMU_CAPTURE_CONFIG_H
#define IMU_CAPTURE_CONFIG_H

/* 第二块 A 板的 IMU 采集固件与整车接收端选择。全部开关只允许 0/1。
 * 默认值如下；实际编译命令的同名值优先（Makefile 也为这些选项提供默认 0）。
 * 切换方法：make ... IMU_CAPTURE_ONLY=1；CMake 使用 -DIMU_CAPTURE_ONLY=1。
 * 其余开关同样传同名参数；切换组合使用新的构建目录并重新烧录对应板。
 * 必须同步传给构建系统，IMU_CAPTURE_ONLY 还决定数值计算文件的 -fno-fast-math 选项。 */
#ifndef IMU_CAPTURE_ONLY
#define IMU_CAPTURE_ONLY 0 /* 默认 0：正常整车任务；1：仅采集/温控/诊断，需速度后端且 CHASSIS_VELOCITY_FUSION_ENABLE=0。 */
#endif
#if IMU_CAPTURE_ONLY != 0 && IMU_CAPTURE_ONLY != 1
#error IMU_CAPTURE_ONLY must be 0 or 1.
#endif
#ifndef IMU_CAPTURE_CAN_TX_ENABLE
#define IMU_CAPTURE_CAN_TX_ENABLE 0 /* 默认 0：不发送 IMU CAN 帧；1：启用采集板发送，必须同时 IMU_CAPTURE_ONLY=1。 */
#endif
#ifndef CHASSIS_IMU_CAN_RX_ENABLE
#define CHASSIS_IMU_CAN_RX_ENABLE 0 /* 默认 0：不接收外部 IMU CAN 帧；1：启用整车端接收，须满足下方接收约束。 */
#endif
#ifndef CHASSIS_REMOTE_IMU_FUSION_ENABLE
#define CHASSIS_REMOTE_IMU_FUSION_ENABLE 0 /* 0=仅记录远端IMU；1=固定底盘IMU融合观测，仍不进入控制。须同时传给构建系统。 */
#endif
#if CHASSIS_REMOTE_IMU_FUSION_ENABLE != 0 && CHASSIS_REMOTE_IMU_FUSION_ENABLE != 1
#error CHASSIS_REMOTE_IMU_FUSION_ENABLE must be 0 or 1.
#endif
/* 角色和速度后端由观测器/CAN适配器在robot_def加载后检查；此头可能先于默认角色被包含。 */
#if CHASSIS_REMOTE_IMU_FUSION_ENABLE && (!CHASSIS_IMU_CAN_RX_ENABLE || !CHASSIS_VELOCITY_FUSION_ENABLE || CHASSIS_VELOCITY_FUSION_CALIBRATED || IMU_CAPTURE_ONLY)
#error Remote IMU fusion requires CAN RX, velocity observation, normal application and no local gimbal calibration.
#endif
#if IMU_CAPTURE_CAN_TX_ENABLE != 0 && IMU_CAPTURE_CAN_TX_ENABLE != 1
#error IMU_CAPTURE_CAN_TX_ENABLE must be 0 or 1.
#endif
#if CHASSIS_IMU_CAN_RX_ENABLE != 0 && CHASSIS_IMU_CAN_RX_ENABLE != 1
#error CHASSIS_IMU_CAN_RX_ENABLE must be 0 or 1.
#endif
#if IMU_CAPTURE_CAN_TX_ENABLE && !IMU_CAPTURE_ONLY
#error IMU_CAPTURE_CAN_TX_ENABLE requires IMU_CAPTURE_ONLY=1.
#endif
/* 接收端还须 ONE_BOARD、CHASSIS_CONTROL_MODE=CHASSIS_CONTROL_SPEED、
 * CHASSIS_VELOCITY_FUSION_ENABLE=1、CHASSIS_VELOCITY_FUSION_CALIBRATED=0（本地云台路径关闭）、
 * VOFA_TELEMETRY_ENABLE=1、VOFA_TELEMETRY_MODE=1；约束在 chassis_imu_can.c 编译检查。 */
#if CHASSIS_IMU_CAN_RX_ENABLE && IMU_CAPTURE_ONLY
#error CHASSIS_IMU_CAN_RX_ENABLE requires the normal robot application.
#endif

#endif
