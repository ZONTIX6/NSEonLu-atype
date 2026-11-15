#ifndef BOARD_FLASH_CONFIG_H
#define BOARD_FLASH_CONFIG_H

/* 两块 A 板的构建/烧录入口，日常可修改 BOARD_FLASH_TARGET 或传 -Board。
 * 本文件由 scripts/build-board.ps1 读取，再将完整选项传给 Make；不直接包含进固件。
 * 构建：powershell -File scripts/build-board.ps1 -Action Build
 * 构建成功后烧录：powershell -File scripts/build-board.ps1 -Action Flash
 * 只查看选择和命令：powershell -File scripts/build-board.ps1 -Action Show
 * 下载器应连接所选物理板；脚本不会识别连接的物理板。
 * 各定义使用单个整数或双引号字符串，不写表达式/条件分支；原手动 Make/CMake 入口不读取本文件。 */
#define BOARD_FLASH_TARGET 1 /* 烧录目标：1=整车主板，2=下方独立底盘IMU采集板。 */
#define BOARD_FLASH_PROGRAMMER 1 /* 下载器：1=DAPLink/OpenOCD，2=J-Link/J-Flash；仅Flash动作使用。 */
#define BOARD_FLASH_PROFILE "atype-vehicle" /* 两板共用编译profile，任务由各板采集开关区分。 */

/* 板间CAN统一修改 ../90_board/board_config.h 中的 CHASSIS_IMU_CAN_PORT：1=CAN1（默认），2=CAN2。 */
/* 第一块：整车主板。通过所选CAN接收下方A板IMU，UART7统一记录；
 * 远端IMU参与只读速度融合观测，不进入控制；安装/噪声参数另见远端IMU配置。它不是CHASSIS_BOARD/GIMBAL_BOARD分板模式。 */
#define BOARD_FLASH_1_NAME "第一块：整车主板" /* 命令执行前显示的板卡名称。 */
#define BOARD_FLASH_1_BUILD_DIR "build/board-1-main" /* 主板输出目录；build下使用英文字母/数字/下划线/连字符和斜杠。 */
#define BOARD_FLASH_1_ONE_BOARD 1 /* 单板整车角色：必须为1，不与CHASSIS_BOARD或GIMBAL_BOARD同时定义。 */
#define BOARD_FLASH_1_CHASSIS_CONTROL_MODE 0 /* 控制后端：0=速度控制；当前远端IMU观测路径要求0。 */
#define BOARD_FLASH_1_CHASSIS_FORCE_CALIBRATION_ENABLE 0 /* 力控标定工具：0=关闭；本方案不编译标定固件。 */
#define BOARD_FLASH_1_ATYPE_BENCH_MODE 0 /* 台架门控：0=正常整车应用；此项不代替遥控器的启停。 */
#define BOARD_FLASH_1_IMU_CAPTURE_ONLY 0 /* 0=完整整车任务；本入口不启用独立本地IMU诊断任务。 */
#define BOARD_FLASH_1_IMU_CAPTURE_CAN_TX_ENABLE 0 /* 0=主板不发送采集帧，避免与第二块重复发送。 */
#define BOARD_FLASH_1_CHASSIS_IMU_CAN_RX_ENABLE 1 /* 1=接收所选底盘IMU；0=关闭外部接收。 */
#define BOARD_FLASH_1_CHASSIS_VELOCITY_FUSION_ENABLE 1 /* 1=启用速度观测接口；当前远端接收路径要求1。 */
#define BOARD_FLASH_1_CHASSIS_VELOCITY_FUSION_CALIBRATED 0 /* 0=不启用本地云台标定；远端固定底盘IMU使用独立的安装与噪声配置。 */
#define BOARD_FLASH_1_CHASSIS_REMOTE_IMU_FUSION_ENABLE 1 /* 1=用固定底盘IMU做只读卡尔曼观测；不表示安装与噪声参数已实测审核，也不接管控制。 */
#define BOARD_FLASH_1_VOFA_TELEMETRY_ENABLE 1 /* 1=开启UART7统一遥测，需保持UART7未被裁判/辅助设备占用。 */
#define BOARD_FLASH_1_VOFA_TELEMETRY_MODE 2 /* 2=JustFloat二进制观测；0/1保留旧格式，不使用用途选择。 */
/* 功率/卡尔曼切换统一修改 ../_vofa/vofa_config.h 中的 VOFA_OBSERVATION_PROFILE。 */
#define BOARD_FLASH_1_USB_TELEMETRY_ENABLE 1 /* 1=保留USB网页遥测；视觉应使用UART，不能同时选USB视觉。 */

/* 第二块：刚性固定在底盘的下方 A 板。仅采样、温控和所选CAN发送，不初始化整车电机/遥控任务。
 * 位置补偿在主板完成，参数位于 30_velocity_observation/chassis_remote_imu_config.h。 */
#define BOARD_FLASH_2_NAME "第二块：底盘IMU采集板" /* 命令执行前显示的板卡名称。 */
#define BOARD_FLASH_2_BUILD_DIR "build/board-2-imu" /* 独立输出目录，不能与第一块相同。 */
#define BOARD_FLASH_2_ONE_BOARD 1 /* 同工程默认角色；IMU_CAPTURE_ONLY选择独立采集任务。 */
#define BOARD_FLASH_2_CHASSIS_CONTROL_MODE 0 /* 0=速度后端编译配置；采集模式不会启动底盘控制。 */
#define BOARD_FLASH_2_CHASSIS_FORCE_CALIBRATION_ENABLE 0 /* 0=关闭力控标定工具。 */
#define BOARD_FLASH_2_ATYPE_BENCH_MODE 0 /* 0=不使用整车台架任务。 */
#define BOARD_FLASH_2_IMU_CAPTURE_ONLY 1 /* 1=仅IMU采集固件。 */
#define BOARD_FLASH_2_IMU_CAPTURE_CAN_TX_ENABLE 1 /* 1=经所选CAN发送IMU，主板对应开启接收。 */
#define BOARD_FLASH_2_CHASSIS_IMU_CAN_RX_ENABLE 0 /* 0=不作为整车接收端。 */
#define BOARD_FLASH_2_CHASSIS_VELOCITY_FUSION_ENABLE 0 /* 0=不运行整车速度融合。 */
#define BOARD_FLASH_2_CHASSIS_VELOCITY_FUSION_CALIBRATED 0 /* 0=不加载整车融合标定。 */
#define BOARD_FLASH_2_CHASSIS_REMOTE_IMU_FUSION_ENABLE 0 /* 0=采集板只发送IMU。 */
#define BOARD_FLASH_2_VOFA_TELEMETRY_ENABLE 0 /* 0=关闭本板UART7遥测，由主板统一输出。 */
#define BOARD_FLASH_2_VOFA_TELEMETRY_MODE 1 /* 预留格式；遥测关闭时不产生串口输出。 */
#define BOARD_FLASH_2_USB_TELEMETRY_ENABLE 0 /* 0=关闭USB网页遥测，独立采集固件要求关闭。 */

#endif
