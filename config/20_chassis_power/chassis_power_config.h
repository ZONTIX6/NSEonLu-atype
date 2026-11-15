#ifndef CHASSIS_POWER_CONFIG_H
#define CHASSIS_POWER_CONFIG_H

/* 此处数值是启动默认配置，不会自动覆盖已保存的在线调参值。
 * CONTROL_ENABLE、CONTROL_STAGE、CAPTURE_ENABLE、POWER_METER_ENABLE 可使用 Make NAME=value /
 * CMake -DNAME=value 切换；其余直接编辑对应宏并重新构建。
 * 直接力控要求阶段 3；采集 USART2 和其它串口设备的冲突由板级配置检查。 */

/* 底盘功率预算与输出分配共用的安全系数。 */
#define CHASSIS_POWER_SAFETY_FACTOR 0.98f /* 无单位；用于功率预算与分配的共同安全折减系数。 */

/* 临时关闭速度后端功控：0 仅观测功率，PID 电流只保留幅值及启停保护，
 * 小陀螺使用已有 fixed_wz；改回 1 并重新构建、烧录即可恢复阶段 3 功控。
 * 这是编译开关，不受已保存的在线调参值覆盖；直接力控必须设为 1。 */
#ifndef CHASSIS_POWER_CONTROL_ENABLE
#define CHASSIS_POWER_CONTROL_ENABLE 0
#endif
#if CHASSIS_POWER_CONTROL_ENABLE != 0 && CHASSIS_POWER_CONTROL_ENABLE != 1
#error CHASSIS_POWER_CONTROL_ENABLE must be 0 or 1
#endif

/* 速度后端功率分配：误差 rpm、功率 W、电流 CAN 命令值。 */
#define CHASSIS_POWER_SMALL_ERROR_RPM 500.0f /* 四轮转子速度误差绝对值之和的阈值，单位 rpm；不超过时平均分配功率。 */
#define CHASSIS_POWER_RESERVED_THRESHOLD_W 54.0f /* 总有效预算阈值，单位 W；低于它按误差分配，不预留逐轮基础功率。 */
#define CHASSIS_POWER_PER_MOTOR_RESERVED_W 8.0f /* 达到预留门槛后每台电机先获得的基础功率，单位 W。 */
#define CHASSIS_POWER_MAX_CURRENT_COMMAND 15000.0f /* 电机输出电流 CAN 命令幅值上限；命令计数，不能直接当作 A。 */
/*CHASSIS_POWER_CONTROL_STAGE
0    维持现有控制，新增算法只计算诊断结果
1    启用独立快保护、恢复及慢积分隔离
2    增加启动额度、预测约束和反馈不足时的增长兜底
3    增加完整慢环学习门槛、连续死区和积分回算*/
/* 阶段是逐步叠加的控制能力，不是切换底盘速度/力控后端。 */
#ifndef CHASSIS_POWER_CONTROL_STAGE
#define CHASSIS_POWER_CONTROL_STAGE 3 /* 功控阶段：默认 3；0 观测，1 快保护，2 启动保护，3 完整慢环，具体区别见上方说明。 */
#endif
#ifndef CHASSIS_POWER_CAPTURE_ENABLE
#define CHASSIS_POWER_CAPTURE_ENABLE 0 /* 功控采集开关：默认 0 关闭，1 开启；要求 LOG_OUTPUT=2 或 3 以使用 USART2。 */
#endif
#ifndef POWER_METER_ENABLE
#define POWER_METER_ENABLE 1 /* 负载功率计开关：默认 1 启用，0 不注册功率计；自适应直接力控要求为 1。 */
#endif
#if CHASSIS_POWER_CONTROL_STAGE < 0 || CHASSIS_POWER_CONTROL_STAGE > 3
#error CHASSIS_POWER_CONTROL_STAGE must be 0, 1, 2 or 3
#endif
#if CHASSIS_POWER_CAPTURE_ENABLE != 0 && CHASSIS_POWER_CAPTURE_ENABLE != 1
#error CHASSIS_POWER_CAPTURE_ENABLE must be 0 or 1
#endif
#if CHASSIS_POWER_CAPTURE_ENABLE
#include "../90_board/board_config.h"
#if !BOARD_LOG_UART_ENABLED
#error CHASSIS_POWER_CAPTURE_ENABLE requires LOG_OUTPUT=2 or 3 (USART2 transport).
#endif
#endif
#endif
