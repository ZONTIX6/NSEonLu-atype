#include "chassis_spin.h"
#include "chassis_spin_config.h"

/* 小陀螺启动默认值：每个字段的值在 chassis_spin_config.h 中选择。
 * 在线调参及启动时恢复的 Flash 保存值可以更新此运行时副本；修改源码默认
 * 后若数值未生效，须同时核对保存值。各模式可用范围由 ChassisSpinConfigIsValid 校验。 */
ChassisSpinConfig_s chassis_spin_config = {
#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE
    .fixed_wz = CHASSIS_SPIN_SINE_CENTER_WZ, /* 正弦分支为轨迹中心，其余为固定/初始目标；单位 Wz 命令尺度。 */
    .min_wz = CHASSIS_SPIN_SINE_MIN_WZ, /* 对应模式的目标下限，单位 Wz 命令尺度。 */
#else
    .fixed_wz = CHASSIS_SPIN_FIXED_WZ, /* 正弦分支为轨迹中心，其余为固定/初始目标；单位 Wz 命令尺度。 */
    .min_wz = CHASSIS_SPIN_MIN_WZ, /* 对应模式的目标下限，单位 Wz 命令尺度。 */
#endif
    .max_wz = CHASSIS_SPIN_MAX_WZ, /* 目标软上限，单位 Wz 命令尺度。 */
    .gain_wz_per_w_s = CHASSIS_SPIN_POWER_GAIN, /* 功率误差到目标变化率的增益，单位 Wz/(W*s)。 */
    .rise_wz_per_s = CHASSIS_SPIN_RISE_RATE, /* 普通目标增长速率上限，单位 Wz/s。 */
    .fall_wz_per_s = CHASSIS_SPIN_FALL_RATE, /* 普通目标下降速率上限，单位 Wz/s。 */
    .power_lpf_tau_s = CHASSIS_SPIN_POWER_LPF_TAU_S, /* 实测功率低通时间常数，单位 s。 */
    .power_deadband_w = CHASSIS_SPIN_POWER_DEADBAND_W, /* 功率误差死区，单位 W。 */
    .settle_time_s = CHASSIS_SPIN_SETTLE_TIME_S, /* 增长前稳定确认时间，单位 s。 */
    .budget_drop_reset_w = CHASSIS_SPIN_BUDGET_DROP_RESET_W, /* 自适应预算下降复位阈值，单位 W；正弦不使用该复位缩放。 */
    .sine_amplitude_wz = CHASSIS_SPIN_SINE_AMPLITUDE_WZ, /* 正弦振幅，单位 Wz 命令尺度。 */
    .sine_period_s = CHASSIS_SPIN_SINE_PERIOD_S, /* 正弦完整周期，单位 s。 */
    .sine_ramp_s = CHASSIS_SPIN_SINE_RAMP_S, /* 正弦振幅建立时间，单位 s。 */
    .sine_rise_wz_per_s = CHASSIS_SPIN_SINE_RISE_RATE, /* 正弦目标上升速率上限，单位 Wz/s。 */
    .sine_fall_wz_per_s = CHASSIS_SPIN_SINE_FALL_RATE, /* 正弦目标下降速率上限，单位 Wz/s。 */
    .sine_direction = CHASSIS_SPIN_SINE_DIRECTION, /* 正弦方向：+1 正向，-1 反向。 */
    .extension_enabled=CHASSIS_SPIN_EXTENSION_ENABLE!=0, /* 软上限扩展是否启用；0 关闭、非零开启，默认关闭。 */
    .hard_wz=CHASSIS_SPIN_HARD_WZ, /* 扩展目标硬上限，单位 Wz 命令尺度；0 跟随软上限。 */
    .body_hard_rps=CHASSIS_SPIN_BODY_HARD_RPS, /* 车体实测转速硬上限，单位转/秒。 */
    .wheel_hard_rpm=CHASSIS_SPIN_WHEEL_HARD_RPM, /* 电机转子实测转速硬上限，单位 rpm。 */
    .extension_rise_wz_per_s=CHASSIS_SPIN_EXTENSION_RISE_RATE, /* 扩展区间目标增长速率上限，单位 Wz/s。 */
};
