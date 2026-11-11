#include "chassis_power_budget.h"
#include "chassis_power_outer_loop.h"
#include "chassis_energy_config.h"
#include "chassis_power_config.h"

/* 功率预算与外环启动默认值；直接修改数值后重新构建。
 * 以宏赋值的字段请到对应 config.h 调整；启动时恢复的 Flash 参数和在线调参
 * 可以覆盖这些运行时副本，因此修改默认值后还须核对现有保存值。 */
ChassisPowerBudgetConfig_s chassis_budget_config = {
    .empty_energy_ratio = 0.10f, /* 电容增补能力开始开放的能量比例，无单位；0～1，低于它不开放额外负载功率。 */
    .full_energy_ratio = 0.30f, /* 电容增补完全开放的能量比例，无单位；应高于空电门槛。 */
    .max_boost_w = 100.0f, /* 增补功率上限，单位 W；外层限制电容负载侧，buffer 内层限制裁判缓冲的输入侧。 */
    .rise_rate_w_per_s = 200.0f, /* 预算上升斜率，单位 W/s；外层用于负载侧，buffer 内层用于输入侧，下降立即生效。 */
    .demand_margin_w = CHASSIS_POWER_DEMAND_MARGIN_W, /* 高于基础预算的有效驱动需求所允许的申请余量，单位 W。 */
    .buffer = { /* 裁判缓冲能量配置；具体宏值统一在 chassis_energy_config.h 修改。 */
        .max_energy_j = CHASSIS_BUFFER_MAX_ENERGY_J, /* 裁判缓冲原始读数的有效上限，单位 J。 */
        .recovery_enter_j = CHASSIS_BUFFER_RECOVERY_ENTER_J, /* 保守能量估计不高于此值时进入回充，单位 J。 */
        .recovery_exit_j = CHASSIS_BUFFER_RECOVERY_EXIT_J, /* 退出回充的保守能量门槛，单位 J；还须满足连续保持时间。 */
        .hard_floor_j = CHASSIS_BUFFER_HARD_FLOOR_J, /* 预测能量硬底座，单位 J；低于它强制回充。 */
        .prediction_window_s = CHASSIS_BUFFER_PREDICTION_WINDOW_S, /* 缓冲能量释放与输入增补预测窗口，单位 s。 */
        .guard_window_s = CHASSIS_BUFFER_GUARD_WINDOW_S, /* 超限输入功率换算能量时使用的保护窗口，单位 s。 */
        .settlement_delay_s = CHASSIS_BUFFER_SETTLEMENT_DELAY_S, /* 裁判结算时刻相对收帧时刻的延迟裕量，单位 s。 */
        .energy_margin_j = CHASSIS_BUFFER_ENERGY_MARGIN_J, /* 从缓冲估计值中扣除的能量余量，单位 J。 */
        .power_margin_w = CHASSIS_BUFFER_POWER_MARGIN_W, /* 输入功率观测与预算预留的功率余量，单位 W。 */
        .max_boost_w = CHASSIS_BUFFER_BOOST_MAX_W, /* 增补功率上限，单位 W；外层限制电容负载侧，buffer 内层限制裁判缓冲的输入侧。 */
        .recovery_power_w = CHASSIS_BUFFER_RECOVERY_POWER_W, /* 回充状态下最大功率减让量，单位 W，并非每周期固定减量。 */
        .recovery_target_time_s = CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S, /* 把能量缺口换算成减让功率的目标恢复时间，单位 s。 */
        .observer_max_step_s = CHASSIS_BUFFER_OBSERVER_MAX_STEP_S, /* 观测器允许连续积分的最大时间间隔，单位 s。 */
        .recovery_hold_s = CHASSIS_BUFFER_RECOVERY_HOLD_S, /* 满足退出回充门槛后需要连续保持的时间，单位 s。 */
        .rise_rate_w_per_s = CHASSIS_BUFFER_RISE_RATE_W_PER_S, /* 预算上升斜率，单位 W/s；外层用于负载侧，buffer 内层用于输入侧，下降立即生效。 */
        .safety_factor = CHASSIS_POWER_SAFETY_FACTOR, /* 功率与能量预算共用的安全系数，无单位。 */
        .correction_rise_j_per_s = CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S, /* 新裁判帧向上修正能量估计的速率上限，单位 J/s；向下修正立即生效。 */
    },
};

/* 功率外环:用功率计实测功率对比模型预测值,在"基准预算"上叠加一个有界的修正量 */
ChassisPowerOuterLoopConfig_s chassis_outer_loop_config = {
    .kp = 1.0f, // 比例增益(无单位),误差 = 目标功率 - 实测功率(W),输出为预算修正量(W)
    .ki = 1.5f, // 积分增益(1/s),积分限幅为[-基准预算, max_positive_correction_w]
    .measurement_lpf_tau_s = 0.030f, // 功率计读数一阶低通时间常数(s),抑制采样噪声
    .activation_margin_w = 5.0f, // 介入阈值(W):需求或实测功率距目标不到此值时才进入ACTIVE
    .error_deadband_w = 0.3f, // 误差死区(W):|误差|不超过此值时按0处理,停止积分
    .max_positive_correction_w = 8.0f, // 修正量上限(W):最多只上调8W;下限为-基准预算,即最狠可压到0
    .positive_slew_rate_w_per_s = 60.0f, // 修正量上升/回零速率(W/s);下调不限速,立即生效
    .budget_drop_reset_w = 1.0f, // 基准预算单周期跌落超过此值(W)时,清零正向积分与修正量
    .antiwindup_gain_per_s = 10, /* 慢环积分回算的抗饱和增益，单位 1/s。 */
    .low_demand_delay_s = .5f, /* 低需求持续多久后衰减旧功率修正，单位 s。 */
    .low_demand_decay_w_per_s = 1, /* 低需求状态下旧功率修正回零的速度，单位 W/s。 */
};
