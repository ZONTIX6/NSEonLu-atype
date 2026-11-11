#ifndef CHASSIS_SPIN_CONFIG_H
#define CHASSIS_SPIN_CONFIG_H

/* 本文件定义速度后端的小陀螺参数；直接力控的转矩参数见 chassis_force_vehicle_config.c。
 * 定义行是源码默认值；可直接修改，或通过 Make NAME=value / CMake -DNAME=value 覆盖。
 * 例如 Make：make CHASSIS_SPIN_MODE=2 BUILD_DIR=build/spin-sine；
 * CMake：cmake -S . -B build/cmake-spin-sine -DCHASSIS_SPIN_MODE=2。
 * Wz 是历史命令尺度，不是电机 rpm、车体转/秒或 rad/s；物理转速需经底盘运动学换算。 */

/* 0: 沿用原有固定自旋目标; 1: 转速已跟上且功率有余量时提高自旋目标。 */
#ifndef CHASSIS_SPIN_ADAPTIVE_ENABLE
#define CHASSIS_SPIN_ADAPTIVE_ENABLE 1 /* 兼容开关：默认 1 自适应、0 固定；仅未显式指定 CHASSIS_SPIN_MODE 时决定模式。 */
#endif
#if CHASSIS_SPIN_ADAPTIVE_ENABLE != 0 && CHASSIS_SPIN_ADAPTIVE_ENABLE != 1
#error CHASSIS_SPIN_ADAPTIVE_ENABLE must be 0 or 1.
#endif

#define CHASSIS_SPIN_MODE_FIXED 0 /* 自旋模式编号：0，固定 Wz 目标。 */
#define CHASSIS_SPIN_MODE_ADAPTIVE 1 /* 自旋模式编号：1，功率余量允许时提高 Wz 目标。 */
#define CHASSIS_SPIN_MODE_SINE 2 /* 自旋模式编号：2，按正弦轨迹改变同向 Wz 目标。 */
#ifndef CHASSIS_SPIN_MODE
#if CHASSIS_SPIN_ADAPTIVE_ENABLE
#define CHASSIS_SPIN_MODE CHASSIS_SPIN_MODE_ADAPTIVE /* 模式选择：0 固定、1 自适应、2 正弦；未覆盖时由兼容开关选择，工程默认 1。 */
#else
#define CHASSIS_SPIN_MODE CHASSIS_SPIN_MODE_FIXED /* 模式选择：0 固定、1 自适应、2 正弦；未覆盖时由兼容开关选择，工程默认 1。 */
#endif
#endif
#if CHASSIS_SPIN_MODE < CHASSIS_SPIN_MODE_FIXED || CHASSIS_SPIN_MODE > CHASSIS_SPIN_MODE_SINE
#error CHASSIS_SPIN_MODE must be 0 (fixed), 1 (adaptive), or 2 (sine).
#endif
#define CHASSIS_SPIN_DYNAMIC_ENABLED (CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_FIXED) /* 派生开关：固定模式为 0，其余模式为 1；由模式自动计算，不单独修改。 */

/* 下列 Wz 参数使用工程既有命令尺度，不能直接填入电机 rpm。 */
#ifndef CHASSIS_SPIN_FIXED_WZ
#define CHASSIS_SPIN_FIXED_WZ 8000.0f /* 固定模式目标/自适应初始目标，单位为本工程 Wz 命令尺度。 */
#endif
#ifndef CHASSIS_SPIN_MIN_WZ
#define CHASSIS_SPIN_MIN_WZ 3000.0f /* 自适应模式的 Wz 目标下限，单位 Wz 命令尺度，须为正且不大于初始目标。 */
#endif
#ifndef CHASSIS_SPIN_MAX_WZ
#define CHASSIS_SPIN_MAX_WZ 9000.0f /* 动态自旋目标软上限，单位 Wz 命令尺度，须不小于初始或正弦峰值。 */
#endif
#ifndef CHASSIS_SPIN_POWER_GAIN
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
#define CHASSIS_SPIN_POWER_GAIN 80.0f /* 功率误差到目标变化速率的增益，单位 Wz/(W*s)；自适应默认 80，其余默认 40。 */
#else
#define CHASSIS_SPIN_POWER_GAIN 40.0f /* 功率误差到目标变化速率的增益，单位 Wz/(W*s)；自适应默认 80，其余默认 40。 */
#endif
#endif
#ifndef CHASSIS_SPIN_RISE_RATE
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
#define CHASSIS_SPIN_RISE_RATE 3600.0f /* 自适应目标上升速率上限，单位 Wz/s；自适应默认 3600，其余默认 1200。 */
#else
#define CHASSIS_SPIN_RISE_RATE 1200.0f /* 自适应目标上升速率上限，单位 Wz/s；自适应默认 3600，其余默认 1200。 */
#endif
#endif
#ifndef CHASSIS_SPIN_FALL_RATE
#define CHASSIS_SPIN_FALL_RATE 2400.0f /* 非正弦目标下降速率上限，单位 Wz/s；正弦使用独立下降速率。 */
#endif
#ifndef CHASSIS_SPIN_POWER_LPF_TAU_S
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
#define CHASSIS_SPIN_POWER_LPF_TAU_S 0.05f /* 实测功率低通时间常数，单位 s；自适应默认 0.05，其余默认 0.1。 */
#else
#define CHASSIS_SPIN_POWER_LPF_TAU_S 0.1f /* 实测功率低通时间常数，单位 s；自适应默认 0.05，其余默认 0.1。 */
#endif
#endif
#ifndef CHASSIS_SPIN_POWER_DEADBAND_W
#define CHASSIS_SPIN_POWER_DEADBAND_W 1.0f /* 功率误差死区，单位 W；死区内不继续按误差增加或降低目标。 */
#endif
#ifndef CHASSIS_SPIN_SETTLE_TIME_S
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE
#define CHASSIS_SPIN_SETTLE_TIME_S 0.01f /* 允许增长前的稳定确认时间，单位 s；自适应默认 0.01，其余默认 0.2。 */
#else
#define CHASSIS_SPIN_SETTLE_TIME_S 0.2f /* 允许增长前的稳定确认时间，单位 s；自适应默认 0.01，其余默认 0.2。 */
#endif
#endif
#ifndef CHASSIS_SPIN_TRACKING_ERROR_RPM
#define CHASSIS_SPIN_TRACKING_ERROR_RPM 500.0f /* 轮速跟踪误差门槛，单位转子 rpm；跟踪不足时限制目标增长。 */
#endif
#ifndef CHASSIS_SPIN_BUDGET_DROP_RESET_W
#define CHASSIS_SPIN_BUDGET_DROP_RESET_W 1.0f /* 自适应预算下降复位门槛，单位 W；正弦模式不据此缩放轨迹。 */
#endif

/* 仅完成带载整车标定后开启扩展；硬 Wz 为 0 时跟随当前软上限。 */
#ifndef CHASSIS_SPIN_EXTENSION_ENABLE
#define CHASSIS_SPIN_EXTENSION_ENABLE 0 /* 软上限扩展开关：默认 0 关闭，1 开启；仅支持自适应且必须完成负载标定。 */
#endif
#ifndef CHASSIS_SPIN_HARD_WZ
#define CHASSIS_SPIN_HARD_WZ 0.0f /* 扩展后的 Wz 硬上限；0 随当前软上限，启用扩展时须不低于软上限。 */
#endif
#ifndef CHASSIS_SPIN_BODY_HARD_RPS
#define CHASSIS_SPIN_BODY_HARD_RPS 0.0f /* 实测车体转速硬上限，单位转/秒（不是 rad/s）；默认 0 未标定，扩展要求正值。 */
#endif
#ifndef CHASSIS_SPIN_WHEEL_HARD_RPM
#define CHASSIS_SPIN_WHEEL_HARD_RPM 0.0f /* 实测电机转子转速硬上限，单位 rpm；默认 0 未标定，扩展要求正值。 */
#endif
#ifndef CHASSIS_SPIN_EXTENSION_RISE_RATE
#define CHASSIS_SPIN_EXTENSION_RISE_RATE 600.0f /* 超出软上限后的增长速率，单位 Wz/s；启用时须为正且不高于普通上升速率。 */
#endif
#if CHASSIS_SPIN_EXTENSION_ENABLE != 0 && CHASSIS_SPIN_EXTENSION_ENABLE != 1
#error CHASSIS_SPIN_EXTENSION_ENABLE must be 0 or 1.
#endif
#if CHASSIS_SPIN_EXTENSION_ENABLE && CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_ADAPTIVE
#error Soft-limit extension requires adaptive spin.
#endif

/* 正弦使用独立的可跟随目标；默认麦轮几何下稳态约为 3403..5672 rpm。
 * 沿用 4000 的中心会要求约 22689 rpm，使跟随/功率保护阻止振幅建立。 */
#ifndef CHASSIS_SPIN_SINE_CENTER_WZ
#define CHASSIS_SPIN_SINE_CENTER_WZ 800.0f /* 正弦轨迹中心，单位 Wz 命令尺度；中心减振幅不得低于正弦最小值。 */
#endif
#ifndef CHASSIS_SPIN_SINE_MIN_WZ
#define CHASSIS_SPIN_SINE_MIN_WZ 200.0f /* 正弦同向目标下限，单位 Wz 命令尺度；须为正，避免跨零换向。 */
#endif
#ifndef CHASSIS_SPIN_SINE_AMPLITUDE_WZ
#define CHASSIS_SPIN_SINE_AMPLITUDE_WZ 200.0f /* 正弦振幅，单位 Wz 命令尺度；须非负，中心加振幅不得超过软上限。 */
#endif
#ifndef CHASSIS_SPIN_SINE_PERIOD_S
#define CHASSIS_SPIN_SINE_PERIOD_S 4.0f /* 正弦完整周期，单位 s；须大于 0。 */
#endif
#ifndef CHASSIS_SPIN_SINE_RAMP_S
#define CHASSIS_SPIN_SINE_RAMP_S 1.0f /* 建立正弦振幅的时间，单位 s；须大于 0。 */
#endif
#ifndef CHASSIS_SPIN_SINE_RISE_RATE
#define CHASSIS_SPIN_SINE_RISE_RATE 400.0f /* 正弦目标上升速率上限，单位 Wz/s；须大于 0。 */
#endif
#ifndef CHASSIS_SPIN_SINE_FALL_RATE
#define CHASSIS_SPIN_SINE_FALL_RATE 2000.0f /* 正弦目标下降及超功率退让速率上限，单位 Wz/s；须大于 0。 */
#endif
#ifndef CHASSIS_SPIN_SINE_DIRECTION
#define CHASSIS_SPIN_SINE_DIRECTION 1 /* 正弦旋转方向：默认 +1 正向，-1 反向；只接受这两个值。 */
#endif

#endif
