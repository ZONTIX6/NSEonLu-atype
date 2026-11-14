#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* A 板 USART1 固定用于 DBUS；UART1_USE 已废弃。
 * 修改下方默认值后重新编译；带 #ifndef 的选项也可用构建参数覆盖，
 * 例如 make LOG_OUTPUT=2 REFEREE_UART_PORT=6；CMake 使用 -DLOG_OUTPUT=2。
 * 串口归属必须一致，下面的编译检查会拒绝已知资源冲突。 */
#ifdef UART1_USE
#error UART1_USE is retired: USART1 is DBUS; use REFEREE_ENABLE and LOG_OUTPUT.
#endif
#ifndef REFEREE_ENABLE
#define REFEREE_ENABLE 1 /* 裁判系统开关：0=关闭，1=开启；默认开启。 */
#endif
#ifndef REFEREE_UART_PORT
#define REFEREE_UART_PORT 6 /* 裁判串口：6=USART6（A板编号16、最左侧丝印6），7=UART7；默认6。 */
#endif
#ifndef LOG_OUTPUT
#define LOG_OUTPUT 1 /* 日志输出：0=关闭，1=RTT，2=USART2，3=RTT与USART2同时输出。 */
#endif
#ifndef CHASSIS_IMU_CAN_PORT
#define CHASSIS_IMU_CAN_PORT 2 /* 主板与IMU采集板通信：1=CAN1，2=CAN2（默认）；两板共用，修改后分别重新编译烧录。 */
#endif
#ifndef VOFA_TELEMETRY_MODE
#define VOFA_TELEMETRY_MODE 2 /* VOFA传输：0=旧19路JustFloat，1=旧FireWater，2=JustFloat用途观测。 */
#endif
#include "../_vofa/vofa_config.h" /* 功率/卡尔曼的日常切换入口。 */
#ifndef ATYPE_BENCH_MODE
#define ATYPE_BENCH_MODE 0 /* 台架模式：0=整车正常控制，1=台架门控（云台/发射锁停）；atype-bench构建默认设1。 */
#endif
#if REFEREE_ENABLE != 0 && REFEREE_ENABLE != 1
#error REFEREE_ENABLE must be 0 or 1.
#endif
#if REFEREE_UART_PORT != 6 && REFEREE_UART_PORT != 7
#error REFEREE_UART_PORT must be 6 (USART6) or 7 (UART7); other UARTs are reserved.
#endif
#if LOG_OUTPUT < 0 || LOG_OUTPUT > 3
#error LOG_OUTPUT must be 0, 1, 2, or 3.
#endif
#if CHASSIS_IMU_CAN_PORT != 1 && CHASSIS_IMU_CAN_PORT != 2
#error CHASSIS_IMU_CAN_PORT must be 1 (CAN1) or 2 (CAN2).
#endif
#if VOFA_TELEMETRY_MODE != 0 && VOFA_TELEMETRY_MODE != 1 && VOFA_TELEMETRY_MODE != 2
#error VOFA_TELEMETRY_MODE must be 0, 1 or 2.
#endif
#if VOFA_OBSERVATION_PROFILE != 0 && VOFA_OBSERVATION_PROFILE != 1 && VOFA_OBSERVATION_PROFILE != 2
#error VOFA_OBSERVATION_PROFILE must be 0, 1 or 2.
#endif
#if ATYPE_BENCH_MODE != 0 && ATYPE_BENCH_MODE != 1
#error ATYPE_BENCH_MODE must be 0 or 1.
#endif
#define BOARD_LOG_UART_ENABLED (LOG_OUTPUT == 2 || LOG_OUTPUT == 3) /* 按LOG_OUTPUT自动判断是否启用串口日志，不单独修改。 */
#define BOARD_LOG_RTT_ENABLED (LOG_OUTPUT == 1 || LOG_OUTPUT == 3) /* 按LOG_OUTPUT自动判断是否启用RTT日志，不单独修改。 */

#define BOARD_REMOTE_UART (&huart1) /* DBUS遥控器固定连接USART1；更改句柄前须同步硬件与串口初始化。 */
#define BOARD_VISION_UART (&huart3) /* 视觉通信绑定USART3；启用方式见robot_config.h。 */
#define BOARD_VTX_UART (&huart8) /* 图传键鼠输入绑定UART8。 */
#if REFEREE_UART_PORT == 7
#define BOARD_REFEREE_UART (&huart7) /* 裁判串口句柄，由REFEREE_UART_PORT自动选择，不单独修改。 */
#else
#define BOARD_REFEREE_UART (&huart6) /* 裁判串口句柄，由REFEREE_UART_PORT自动选择，不单独修改。 */
#endif
#define BOARD_LOG_UART (&huart2) /* 串口日志绑定USART2；由LOG_OUTPUT决定是否启用。 */
#define BOARD_VOFA_UART (&huart7) /* VOFA遥测绑定UART7，不能与裁判或UART7辅助设备同时占用。 */
#define BOARD_SUPERCAP_CAN (&hcan1) /* 超级电容通信绑定CAN1；协议CAN ID仍由协议模块定义。 */
#define BOARD_POWER_METER_CAN (&hcan1) /* 功率计通信绑定CAN1；启用开关POWER_METER_ENABLE见chassis_power_config.h。 */
#if CHASSIS_IMU_CAN_PORT == 1
#define BOARD_CHASSIS_IMU_CAN (&hcan1) /* 板间IMU通信句柄，由CHASSIS_IMU_CAN_PORT自动选择，不单独修改。 */
#else
#define BOARD_CHASSIS_IMU_CAN (&hcan2) /* 板间IMU通信句柄，由CHASSIS_IMU_CAN_PORT自动选择，不单独修改。 */
#endif

#ifndef OLED_USE_I2C
#define OLED_USE_I2C 0 /* OLED后端：0=SPI1（默认），1=I2C2；还需BOARD_OLED_ENABLE=1注册显示屏。 */
#endif
#if OLED_USE_I2C != 0 && OLED_USE_I2C != 1
#error OLED_USE_I2C must be 0 or 1.
#endif

/* 辅助设备选择：0=不接入，1=TFmini 激光测距，2=HC05 蓝牙，3=总线舵机。
 * 每个串口仅接一个点对点设备；相同辅助设备类型也不能在两个串口重复注册。 */
#ifndef BOARD_AUX_UART2_DEVICE
#define BOARD_AUX_UART2_DEVICE 0 /* USART2辅助设备：0=无，1=TFmini，2=HC05，3=总线舵机；非0时LOG_OUTPUT不能为2或3。 */
#endif
#ifndef BOARD_AUX_UART7_DEVICE
#define BOARD_AUX_UART7_DEVICE 0 /* UART7辅助设备：0=无，1=TFmini，2=HC05，3=总线舵机；非0时不能同时接UART7裁判或启用VOFA。 */
#endif
#ifndef BOARD_OLED_ENABLE
#define BOARD_OLED_ENABLE 0 /* OLED开关：0=不注册（默认），1=注册并初始化；后端由OLED_USE_I2C选择。 */
#endif
#ifndef BOARD_KEY_ENABLE
#define BOARD_KEY_ENABLE 0 /* 板载按键开关：0=不注册（默认），1=注册按键模块。 */
#endif
#if BOARD_AUX_UART2_DEVICE < 0 || BOARD_AUX_UART2_DEVICE > 3 || BOARD_AUX_UART7_DEVICE < 0 || BOARD_AUX_UART7_DEVICE > 3
#error Auxiliary UART device must be 0 (none), 1 (TFmini), 2 (HC05), or 3 (bus servo).
#endif
#if BOARD_AUX_UART2_DEVICE && BOARD_LOG_UART_ENABLED
#error USART2 cannot be shared by an auxiliary device and UART logging.
#endif
#if REFEREE_ENABLE && REFEREE_UART_PORT == 7 && BOARD_AUX_UART7_DEVICE
#error UART7 cannot be shared by the referee system and an auxiliary device.
#endif
#define BOARD_UART7_RESERVED_FOR_DEVICE ((REFEREE_ENABLE && REFEREE_UART_PORT == 7) || BOARD_AUX_UART7_DEVICE) /* 按裁判和辅助设备选择自动判断UART7是否被占用，不单独修改。 */
/* VOFA 使用速度控制快照和独占 UART7；自动默认值取决于下面条件。
 * 可显式用 make VOFA_TELEMETRY_ENABLE=0 关闭，设 1 开启时 UART7 必须空闲。 */
#ifndef VOFA_TELEMETRY_ENABLE
#if (!defined(CHASSIS_CONTROL_MODE) || CHASSIS_CONTROL_MODE == 0) && !BOARD_UART7_RESERVED_FOR_DEVICE
#define VOFA_TELEMETRY_ENABLE 1 /* VOFA发送开关：1=发送，0=关闭；默认速度后端且UART7空闲时为1，其余情况为0。 */
#else
#define VOFA_TELEMETRY_ENABLE 0 /* VOFA发送开关：1=发送，0=关闭；默认速度后端且UART7空闲时为1，其余情况为0。 */
#endif
#endif
#if VOFA_TELEMETRY_ENABLE != 0 && VOFA_TELEMETRY_ENABLE != 1
#error VOFA_TELEMETRY_ENABLE must be 0 or 1.
#endif
#if VOFA_TELEMETRY_ENABLE && BOARD_UART7_RESERVED_FOR_DEVICE
#error UART7 cannot be shared by VOFA telemetry and the referee system or an auxiliary device.
#endif
/* UART7实际波特率：仅全量JustFloat提速；旧VOFA模式及其他UART7用途保持115200。 */
#if VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE == 2
#define BOARD_UART7_BAUDRATE 230400U /* 模式2配合20ms输出；VOFA和串口接收设备须使用相同波特率。 */
#else
#define BOARD_UART7_BAUDRATE 115200U /* 模式0/1、VOFA关闭、裁判或辅助设备使用UART7时保留原值。 */
#endif
#if BOARD_AUX_UART2_DEVICE && BOARD_AUX_UART7_DEVICE && BOARD_AUX_UART2_DEVICE == BOARD_AUX_UART7_DEVICE
#error TFmini, HC05 and bus servo each support one UART instance.
#endif
#if BOARD_OLED_ENABLE < 0 || BOARD_OLED_ENABLE > 1 || BOARD_KEY_ENABLE < 0 || BOARD_KEY_ENABLE > 1
#error Optional OLED/key enables must be 0 or 1.
#endif

#endif
