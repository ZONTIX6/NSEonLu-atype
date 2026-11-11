#ifndef CHASSIS_CONTROL_CONFIG_H
#define CHASSIS_CONTROL_CONFIG_H

/* 切换底盘后端会改变参与编译的源文件，单改本头文件不足以切换。
 * Make：make CHASSIS_CONTROL_MODE=1 BUILD_DIR=build/force
 * CMake：cmake -S . -B build/cmake-force -DCHASSIS_CONTROL_MODE=1，再构建该目录。
 * 两种构建系统默认均选择 0，并可能以 -D 覆盖头文件。其它开关可改此处默认值，
 * 或在 Make 传 NAME=value、CMake 配置时传 -DNAME=value；更换开关使用独立构建目录。 */
#define CHASSIS_CONTROL_SPEED 0 /* 速度环后端编号：0，默认选择。 */
#define CHASSIS_CONTROL_FORCE 1 /* 直接力控后端编号：1，需要完成车辆力控标定。 */
#ifndef CHASSIS_CONTROL_MODE
#define CHASSIS_CONTROL_MODE CHASSIS_CONTROL_SPEED /* 后端选择：0 速度环，1 直接力控；默认 0，必须通过上方构建参数切换。 */
#endif
#define CHASSIS_FORCE_EXIT_COAST 0 /* 力控退出方式编号：0，撤销驱动力后滑行。 */
#define CHASSIS_FORCE_EXIT_BRAKE 1 /* 力控退出方式编号：1，按制动转矩、停止死区和超时限值制动。 */
#ifndef CHASSIS_FORCE_EXIT_MODE
#define CHASSIS_FORCE_EXIT_MODE CHASSIS_FORCE_EXIT_COAST /* 退出策略：默认 0 滑行；改为 1 前须填写并验证车辆 brake_* 参数。 */
#endif
#ifndef CHASSIS_FORCE_CALIBRATION_VALID
#define CHASSIS_FORCE_CALIBRATION_VALID 0 /* 标定验收开关：默认 0 未验收、禁止驱动；1 已验收，仍须通过所有物理参数校验。 */
#endif
#define CHASSIS_FORCE_SPIN_FIXED_TORQUE 0 /* 力控自旋策略编号：0，固定请求转矩。 */
#define CHASSIS_FORCE_SPIN_POWER_ADAPTIVE 1 /* 力控自旋策略编号：1，根据负载功率调整请求转矩。 */
#ifndef CHASSIS_FORCE_SPIN_MODE
#define CHASSIS_FORCE_SPIN_MODE CHASSIS_FORCE_SPIN_POWER_ADAPTIVE /* 自旋策略：默认 1 功率自适应；0 固定转矩；模式 1 要求启用负载功率计。 */
#endif
#if CHASSIS_CONTROL_MODE != 0 && CHASSIS_CONTROL_MODE != 1
#error Invalid CHASSIS_CONTROL_MODE
#endif
#if CHASSIS_FORCE_EXIT_MODE != 0 && CHASSIS_FORCE_EXIT_MODE != 1
#error Invalid CHASSIS_FORCE_EXIT_MODE
#endif
#if CHASSIS_FORCE_CALIBRATION_VALID != 0 && CHASSIS_FORCE_CALIBRATION_VALID != 1
#error Invalid CHASSIS_FORCE_CALIBRATION_VALID
#endif
#if CHASSIS_FORCE_SPIN_MODE != 0 && CHASSIS_FORCE_SPIN_MODE != 1
#error Invalid CHASSIS_FORCE_SPIN_MODE
#endif
#endif
