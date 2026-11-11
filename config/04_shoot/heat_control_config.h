#ifndef HEAT_CONTROL_CONFIG_H
#define HEAT_CONTROL_CONFIG_H

/* 修改对应 #define 右侧后重新编译；本文件均为热量策略数值，不设置发射启停开关。
 * 运行时仍需有效且及时的裁判热量/冷却信息；发射端还会检查摩擦轮状态与电机反馈。
 */

/* 单发热量统一配置入口，应与裁判系统实际配置一致。 */
/* 默认单发热量，单位裁判热量值/发；默认 10.0，应与弹种和裁判配置一致，可由 HeatControlInit 的有效参数覆盖。 */
#define HEAT_CONTROL_BULLET_HEAT 10.0f
/* 默认物理射频上限，单位发/秒；默认 30.0，可由 HeatControlInit 的有效 max_shoot_rate 覆盖。 */
#define HEAT_CONTROL_DEFAULT_MAX_RATE 30.0f
/* 为裁判传输和执行延迟预留的弹数，单位发；默认 7.0，预留热量=本值×单发热量，也用于离散批次许可。 */
#define HEAT_CONTROL_RESERVE_SHOTS 7.0f /* Delay reserve; see integration validation report. */
/* 本地射频更新周期及公式使用的冷却周期，单位 s；默认 0.1，对应裁判 10 Hz 冷却节奏，但不与裁判相位同步。 */
#define HEAT_CONTROL_SETTLE_PERIOD_S 0.1f /* 裁判系统按 10 Hz 结算冷却 */
/* 剩余热量的射频规划窗口，单位 s；默认 0.5，应大于 0，窗口越长越早降低射频，独立于冷却更新周期。 */
#define HEAT_CONTROL_PLAN_PERIOD_S 0.5   /* 热量规划窗口；越长越早降速，与冷却周期独立 */

#endif
