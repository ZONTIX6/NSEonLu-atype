#ifndef SHOOT_CONFIG_H
#define SHOOT_CONFIG_H

/* 调整方式：修改目标参数的 #define 右侧后重新编译；此文件均为数值参数，没有启停开关。
 * 发射启停、单发/三连发/连射由运行时控制模式决定；PID 在 pid_config.c，热量策略在 heat_control_config.h。
 */

/* 发射机构拨盘角度、减速比与装载量配置；修改参数后重新编译。 */

// 发射参数
#ifndef ONE_BULLET_DELTA_ANGLE
/* 每发弹丸对应拨盘输出轴角度，单位度；默认 45，按拨盘机械分度修改。 */
#define ONE_BULLET_DELTA_ANGLE 45    // 发射一发弹丸拨盘转动的距离,由机械设计图纸给出
#endif
#ifndef REDUCTION_RATIO_LOADER
/* 拨弹电机转子/拨盘输出轴的无量纲减速比；默认 36.0（M2006），更换为 M3508 时按实际减速比修改。 */
#define REDUCTION_RATIO_LOADER 36.0f // 2006拨盘电机的减速比,英雄需要修改为3508的19.0f
#endif
#ifndef NUM_PER_CIRCLE
/* 拨盘转动一周可供弹的数量，单位发/圈；默认 8，供连续射频换算电机速度使用，应与机械分度一致。 */
#define NUM_PER_CIRCLE 8            // 拨盘一圈的装载量
#endif

/* 单发拨弹目标为电机转子角度，单位：度。 */
/* 单发对应的电机转子角度，单位度；由拨盘分度×减速比计算，应修改上方两项而不是直接改公式。 */
#define SHOOT_LOADER_ONE_BULLET_MOTOR_ANGLE (ONE_BULLET_DELTA_ANGLE * REDUCTION_RATIO_LOADER)
/* 离散拨弹判定完成的转子角度误差上限，单位度；默认 5.0。 */
#define SHOOT_LOADER_ANGLE_TOLERANCE 5.0f

/* 各弹速档位的摩擦轮目标转速，单位：度/秒。 */
/* 15 m/s 弹速档的摩擦轮目标电机转速，单位度/秒；默认 4000，需通过实际测速标定。 */
#define SHOOT_FRICTION_SPEED_15 4000
/* 18 m/s 弹速档的摩擦轮目标电机转速，单位度/秒；默认 32000，需通过实际测速标定。 */
#define SHOOT_FRICTION_SPEED_18 32000
/* 30 m/s 弹速档的摩擦轮目标电机转速，单位度/秒；默认 8000，保留现有标定值。 */
#define SHOOT_FRICTION_SPEED_30 8000
/* 未匹配弹速档时的摩擦轮目标电机转速，单位度/秒；默认 32000，与各档位参数独立。 */
#define SHOOT_FRICTION_SPEED_DEFAULT 32000
/* 摩擦轮就绪转速误差占目标转速的比例，无量纲；默认 0.1（10%），左右轮均需满足。 */
#define SHOOT_FRICTION_SPEED_TOLERANCE_RATIO .1f
/* 左右摩擦轮持续满足转速误差要求的最短时间，单位 ms；默认 100，随后才允许拨弹。 */
#define SHOOT_FRICTION_STABLE_TIME_MS 100U

/* 数据时效与离散发射后等待裁判热量结算的时间，单位：ms。 */
/* 发射电机反馈最大允许年龄，单位 ms；默认 20，摩擦轮或拨弹电机反馈过期会阻止拨弹。 */
#define SHOOT_MOTOR_FEEDBACK_MAX_AGE_MS 20U
/* 发射端检查的裁判机器人状态/热量反馈最大年龄，单位 ms；默认 300，两个时间戳均须有效。热量模块另有独立时效检查。 */
#define SHOOT_HEAT_FEEDBACK_MAX_AGE_MS 300U
/* 离散批次结束后等待裁判热量结算的时间，单位 ms；默认 300，本地等待时间与新热量时间戳均须超过该间隔。 */
#define SHOOT_HEAT_SETTLE_DELAY_MS 300U

#endif
