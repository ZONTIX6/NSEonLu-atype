#ifndef FORCE_CALIBRATION_CONFIG_H
#define FORCE_CALIBRATION_CONFIG_H

/* 标定功能改变源文件列表，单改本头文件不能保证链接正确；Make/CMake 默认均为 0。
 * 启用须同时满足：ONE_BOARD、X 型全向轮、直接力控、功控阶段 3、台架模式、USB 遥测、
 * 负载功率计，以及 CHASSIS_FORCE_SPIN_MODE=0（固定转矩）。完整示例：
 * Make：make PROFILE=atype-bench CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_CALIBRATION_ENABLE=1
 *       CHASSIS_FORCE_SPIN_MODE=0 BUILD_DIR=build/force-calibration
 * CMake：cmake -S . -B build/cmake-force-calibration -DATYPE_PROFILE=atype-bench
 *        -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -DCHASSIS_FORCE_SPIN_MODE=0
 * 示例为同一条命令分行展示；已有覆盖选项仍须满足上述校验。 */
#ifndef CHASSIS_FORCE_CALIBRATION_ENABLE
#define CHASSIS_FORCE_CALIBRATION_ENABLE 0 /* 标定固件开关：默认 0 不编译标定模块，1 编译；必须使用上述构建参数切换。 */
#endif
#if CHASSIS_FORCE_CALIBRATION_ENABLE != 0 && CHASSIS_FORCE_CALIBRATION_ENABLE != 1
#error CHASSIS_FORCE_CALIBRATION_ENABLE must be 0 or 1
#endif
#define FC_RECORD_BYTES 48U /* 每条标定记录的线格式长度，单位字节；固定协议为 48，不能单独改此宏扩展记录。 */
#define FC_RECORD_CAPACITY 1280U /* 最多缓存的记录条数；默认 1280，记录区与控制状态合计受 64 KiB 静态断言约束。 */
#define FC_RESERVED_RECORDS 8U /* 采集接近容量时用于提前停止的预留记录数；默认8，直接影响容量截止点。 */
#define FC_MAX_CYCLE_US 5000U /* 标定控制周期最大允许间隔，单位 us；超过时停止标定或标记采集质量不合格。 */
#define FC_MAX_FEEDBACK_US 5000U /* 电机反馈最大允许年龄/相邻反馈间隔，单位 us；超过时停止标定或标记质量不合格。 */
#endif
