#ifndef GIMBAL_CONFIG_H
#define GIMBAL_CONFIG_H

/* 修改相应 #define 右侧后重新编译；带 #ifndef 的项支持同名编译参数覆盖。
 * 数值方向项使用 +1/-1 切换，不是 0/1 开关；下方已区分当前生效项与保留的兼容参数。
 */

/* 云台编码器零点、角度范围和陀螺仪方向配置；修改参数后重新编译。 */

// 云台参数
#ifndef YAW_CHASSIS_ALIGN_ECD
/* 云台 yaw 与底盘正前方对齐时的编码器读数，单位计数（DJI 单圈 0～8191）；默认 350，用于底盘/云台朝向偏差换算。 */
#define YAW_CHASSIS_ALIGN_ECD 350// 云台和底盘对齐指向相同方向时的电机编码器值,若对云台有机械改动需要修改
#endif
#ifndef YAW_ECD_GREATER_THAN_4096
/* 旧版零点区间标志：0=对齐读数不大于 4096（默认），1=大于 4096；随 YAW_CHASSIS_ALIGN_ECD 同步修改。当前朝向差已使用角度归一化，此兼容标志未被控制代码读取。 */
#define YAW_ECD_GREATER_THAN_4096 0 // ALIGN_ECD值是否大于4096,是为1,否为0;用于计算云台偏转角度
#endif
#ifndef PITCH_HORIZON_ECD
/* pitch 处于水平时的电机编码器读数，单位计数（0～8191）；默认 3412。保留为水平机械零点参数，当前陀螺仪 pitch 限位不读取该值。 */
#define PITCH_HORIZON_ECD 3412      // 云台处于水平位置时编码器值,若对云台有机械改动需要修改
#endif
#ifndef PITCH_MAX_ANGLE
/* pitch 上限，单位度；默认 +30，需与实际反馈坐标一致，陀螺仪反馈时填写陀螺仪角度。 */
#define PITCH_MAX_ANGLE 30           // 云台竖直方向最大角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)
#endif
#ifndef PITCH_MIN_ANGLE
/* pitch 下限，单位度；默认 -30，应小于 PITCH_MAX_ANGLE，按实际机械行程修改。 */
#define PITCH_MIN_ANGLE -30           // 云台竖直方向最小角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)
#endif
#ifndef GYRO2GIMBAL_DIR_YAW
/* 兼容接口的 yaw 方向系数：1=陀螺仪与云台同向（默认），-1=反向；当前控制代码未读取，修改本项不会自动反转电机。 */
#define GYRO2GIMBAL_DIR_YAW 1   // 陀螺仪数据相较于云台的yaw的方向,1为相同,-1为相反
#endif
#ifndef GYRO2GIMBAL_DIR_PITCH
/* 兼容接口的 pitch 方向系数：1=同向（默认），-1=反向；当前控制代码未读取，修改本项不会自动反转电机。 */
#define GYRO2GIMBAL_DIR_PITCH 1 // 陀螺仪数据相较于云台的pitch的方向,1为相同,-1为相反
#endif
#ifndef GYRO2GIMBAL_DIR_ROLL
/* 兼容接口的 roll 方向系数：1=同向（默认），-1=反向；当前控制代码未读取，修改本项不会自动反转电机。 */
#define GYRO2GIMBAL_DIR_ROLL 1  // 陀螺仪数据相较于云台的roll的方向,1为相同,-1为相反
#endif

// 云台 yaw 跟踪允许的最大误差，单位：度。
/* yaw 目标相对当前反馈的最大允许偏差，单位度；默认 60.0，命令端按正负该值限幅。 */
#define YAW_MAX_TRACK_ERROR 60.0f

/* 小陀螺 yaw 输出前馈：仅接入单板速度后端，输入为实际轮速逆解的 rad/s。
 * 启动 4 复测中 1080 出现反向过补；按自旋与静止保持输出之差下调至 800 待复测。
 * 反向尚未标定，本版只补偿正转；设 ENABLE=0 可回退到纯 PID。
 */
#ifndef YAW_SPIN_FEEDFORWARD_ENABLE
#define YAW_SPIN_FEEDFORWARD_ENABLE 1
#endif
#ifndef YAW_SPIN_FEEDFORWARD_GAIN
#define YAW_SPIN_FEEDFORWARD_GAIN 800.0f /* DJI 命令刻度/(rad/s)，不是 A。 */
#endif
#ifndef YAW_SPIN_FEEDFORWARD_MAX_COMMAND
#define YAW_SPIN_FEEDFORWARD_MAX_COMMAND 8000.0f
#endif
#define YAW_SPIN_FEEDFORWARD_STOP_RADPS 0.05f /* 退出后的静止释放阈值。 */
#define YAW_SPIN_FEEDFORWARD_MAX_AGE_MS 20U

#if YAW_SPIN_FEEDFORWARD_ENABLE != 0 && YAW_SPIN_FEEDFORWARD_ENABLE != 1
#error YAW_SPIN_FEEDFORWARD_ENABLE must be 0 or 1
#endif

#endif
