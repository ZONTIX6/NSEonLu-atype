#ifndef CHASSIS_CONFIG_H
#define CHASSIS_CONFIG_H

/* 调整方式：修改相应 #define 右侧并重新编译；带 #ifndef 的参数也支持同名 -D 覆盖。
 * 功率计启停值 POWER_METER_ENABLE 位于 chassis_power_config.h：0=关闭，1=启用。
 * 下方 POWER_METER_ENABLE_IS_VALID_* 是内部合法值检查工具，不是调参项，不要修改。
 */

/* 底盘类型、跟随策略、几何尺寸、轮向与台架功率配置；修改参数后重新编译。 */

/*
 * 底盘台架功率回退。正式固件必须保持为 0；仅在车轮架空调试且没有裁判系统时设为 1。
 * 启用后，只有裁判功率快照无效时才使用 CHASSIS_POWER_BENCH_LIMIT_W。
 */
#ifndef CHASSIS_POWER_BENCH_TEST
/* 台架功率回退开关：0=关闭（默认，正式固件使用），1=启用；仅在裁判功率快照无效时回退。将右侧改为 0/1 或用同名编译参数覆盖。 */
#define CHASSIS_POWER_BENCH_TEST 0
#endif
#ifndef CHASSIS_POWER_BENCH_LIMIT_W
/* 台架回退功率上限，单位 W；默认 50，仅 CHASSIS_POWER_BENCH_TEST=1 且裁判功率无效时生效。 */
#define CHASSIS_POWER_BENCH_LIMIT_W 50.0f // 底盘功率回退限制，单位 W
#endif

#if CHASSIS_POWER_BENCH_TEST != 0 && CHASSIS_POWER_BENCH_TEST != 1
#error CHASSIS_POWER_BENCH_TEST must be 0 or 1.
#endif

/* 功率计 CAN1 状态接收；设为 0 时不创建实例或注册 CAN 过滤器。 */


#include "../20_chassis_power/chassis_power_config.h"

#define POWER_METER_ENABLE_IS_VALID_0 1// 0 表示不启用功率计
#define POWER_METER_ENABLE_IS_VALID_1 1// 1 表示启用功率计
#define POWER_METER_ENABLE_IS_VALID_IMPL(value) POWER_METER_ENABLE_IS_VALID_##value
#define POWER_METER_ENABLE_IS_VALID(value) POWER_METER_ENABLE_IS_VALID_IMPL(value)

#if !POWER_METER_ENABLE_IS_VALID(POWER_METER_ENABLE)
#error POWER_METER_ENABLE must be 0 or 1.
#endif

#undef POWER_METER_ENABLE_IS_VALID
#undef POWER_METER_ENABLE_IS_VALID_IMPL
#undef POWER_METER_ENABLE_IS_VALID_1
#undef POWER_METER_ENABLE_IS_VALID_0

/* 机器人重要参数定义,注意根据不同机器人进行修改,浮点数需要以.0或f结尾,无符号以u结尾 */
// 底盘类型：A 板默认四轮 X 型全向轮，麦克纳姆轮作为可选配置保留。
/* 底盘类型固定编号：0=麦克纳姆轮；此编号不要修改，切换请改 CHASSIS_DRIVE_TYPE。 */
#define CHASSIS_DRIVE_MECANUM 0
/* 底盘类型固定编号：1=四轮 X 型全向轮；此编号不要修改，切换请改 CHASSIS_DRIVE_TYPE。 */
#define CHASSIS_DRIVE_OMNI_X 1
#ifndef CHASSIS_DRIVE_TYPE
/* 底盘类型选择：默认 CHASSIS_DRIVE_OMNI_X（1）；改成 CHASSIS_DRIVE_MECANUM（0）切换麦轮，也可用同名编译参数覆盖。尺寸与轮向需符合实际机构。 */
#define CHASSIS_DRIVE_TYPE CHASSIS_DRIVE_OMNI_X
#endif

#if CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_MECANUM && \
    CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_OMNI_X
#error CHASSIS_DRIVE_TYPE must be CHASSIS_DRIVE_MECANUM or CHASSIS_DRIVE_OMNI_X.
#endif

// 原跟随档策略：0 保留车身跟转；1 以云台为前方平移，车身不主动跟转。
#ifndef CHASSIS_FOLLOW_MODE
/* 随动档策略：0=底盘跟随云台转向（默认），1=按云台前方平移且底盘不主动跟转；修改右侧或用同名编译参数覆盖。仅随动档生效。 */
#define CHASSIS_FOLLOW_MODE 0
#endif
#if CHASSIS_FOLLOW_MODE != 0 && CHASSIS_FOLLOW_MODE != 1
#error CHASSIS_FOLLOW_MODE must be 0 or 1.
#endif

/* 小陀螺平移漂移补偿：速度后端的既有指令单位，不是 m/s。
 * 由 50/100/150 圈实测约 2.6 mm/圈左漂、0.39 mm/圈前漂得到初值。
 * 复测仍向后漂，前后补偿暂归零，保留右移 +28，待尺量位移后再调整。
 * 补偿在云台坐标中给出，
 * 再由 ChassisResolveMotion 按云台相对角转换到底盘坐标，因此在世界方向保持固定。
 * 仅 CHASSIS_ROTATE 生效；方向反转时由用户另行复测后调整符号。 */
#ifndef CHASSIS_SPIN_DRIFT_COMPENSATION_ENABLE
#define CHASSIS_SPIN_DRIFT_COMPENSATION_ENABLE 1
#endif
#ifndef CHASSIS_SPIN_DRIFT_COMPENSATION_VX
#define CHASSIS_SPIN_DRIFT_COMPENSATION_VX (0.0f) /* 暂不叠加前后补偿；前进为正。 */
#endif
#ifndef CHASSIS_SPIN_DRIFT_COMPENSATION_VY
#define CHASSIS_SPIN_DRIFT_COMPENSATION_VY (28.0f) /* 抵消向左漂移；速度后端右移为正。 */
#endif
#if CHASSIS_SPIN_DRIFT_COMPENSATION_ENABLE != 0 && CHASSIS_SPIN_DRIFT_COMPENSATION_ENABLE != 1
#error CHASSIS_SPIN_DRIFT_COMPENSATION_ENABLE must be 0 or 1.
#endif

// 机器人底盘修改的参数,单位为mm(毫米)
#ifndef WHEEL_BASE
/* 前后轮轴距，单位 mm；默认 350，按实际底盘测量值修改。 */
#define WHEEL_BASE 350              // 纵向轴距(前进后退方向)
#endif
#ifndef TRACK_WIDTH
/* 左右轮距，单位 mm；默认 300，按实际底盘测量值修改。 */
#define TRACK_WIDTH 300             // 横向轮距(左右平移方向)
#endif
#ifndef CENTER_GIMBAL_OFFSET_X
/* 云台旋转中心相对底盘几何中心的前后偏移，单位 mm；默认 0 表示居中，符号沿用底盘解算坐标约定。 */
#define CENTER_GIMBAL_OFFSET_X 0    // 云台旋转中心距底盘几何中心的距离,前后方向,云台位于正中心时默认设为0
#endif
#ifndef CENTER_GIMBAL_OFFSET_Y
/* 云台旋转中心相对底盘几何中心的左右偏移，单位 mm；默认 0 表示居中，符号沿用底盘解算坐标约定。 */
#define CENTER_GIMBAL_OFFSET_Y 0    // 云台旋转中心距底盘几何中心的距离,左右方向,云台位于正中心时默认设为0
#endif
#ifndef RADIUS_WHEEL
/* 车轮半径，单位 mm；实测半径 76 mm，用于轮速与车身速度换算。 */
#define RADIUS_WHEEL 76             // 轮子半径
#endif
#ifndef REDUCTION_RATIO_WHEEL
/* 轮电机转子转速/轮轴转速，无量纲减速比；默认 19.0，换电机或减速箱时按实物修改。 */
#define REDUCTION_RATIO_WHEEL 19.0f // 电机减速比,因为编码器量测的是转子的速度而不是输出轴的速度故需进行转换
#endif

// X 型全向轮方向校正：架空测试时若某一轮方向相反，只将对应值改为 -1.0f
#ifndef OMNI_X_LF_DIRECTION
/* X 型全向轮左前轮方向：1.0f=原方向（默认），-1.0f=反向；架空核对后只修改需要反向的轮。 */
#define OMNI_X_LF_DIRECTION 1.0f
#endif
#ifndef OMNI_X_RF_DIRECTION
/* X 型全向轮右前轮方向：1.0f=原方向（默认），-1.0f=反向；仅 X 型全向轮方向校正使用。 */
#define OMNI_X_RF_DIRECTION 1.0f
#endif
#ifndef OMNI_X_LB_DIRECTION
/* X 型全向轮左后轮方向：1.0f=原方向（默认），-1.0f=反向；仅 X 型全向轮方向校正使用。 */
#define OMNI_X_LB_DIRECTION 1.0f
#endif
#ifndef OMNI_X_RB_DIRECTION
/* X 型全向轮右后轮方向：1.0f=原方向（默认），-1.0f=反向；仅 X 型全向轮方向校正使用。 */
#define OMNI_X_RB_DIRECTION 1.0f
#endif

#endif
