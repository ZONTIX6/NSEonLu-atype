#ifndef REMOTE_CONFIG_H
#define REMOTE_CONFIG_H

/* 调整方式：修改相应 #define 右侧后重新编译；这些是增益/阈值参数，没有 0/1 启停宏。
 * DBUS/图传来源由遥控器左开关在运行时选择：上档=图传键鼠，中/下档=摇杆控制。
 * 云台增量与 RobotCMDTask 周期相关；若修改任务频率，应重新确认这些增益。
 */

/* 遥控器摇杆增益：云台为每任务周期角度增量，底盘沿用速度指令单位。 */
/* 摇杆 yaw 角度增量系数，单位度/(摇杆计数·任务周期)；默认 0.005，值越大转动越灵敏。 */
#define REMOTE_STICK_YAW_GAIN 0.005f
/* 摇杆 pitch 角度增量系数，单位度/(摇杆计数·任务周期)；默认 0.001，受云台角度限幅约束。 */
#define REMOTE_STICK_PITCH_GAIN 0.001f
/* 摇杆到底盘既有速度指令的增益，单位速度指令/摇杆计数；默认 40.0，仅速度控制路径使用，不直接等同于车身 mm/s。 */
#define REMOTE_STICK_CHASSIS_GAIN 40.0f
/* 力控制路径摇杆输入归一化满量程，单位摇杆计数；默认 660，应与遥控器协议实际范围一致。 */
#define REMOTE_STICK_FULL_SCALE 660

/* 遥控器拨轮阈值，负方向依次开启摩擦轮和连射。 */
/* 拨轮值小于该阈值时请求开启摩擦轮，单位遥控器计数；默认 -100，阈值为负方向。 */
#define REMOTE_DIAL_FRICTION_THRESHOLD (-100)
/* 拨轮值小于该阈值时请求连射，单位遥控器计数；默认 -200，仍受摩擦轮就绪和热量条件约束。 */
#define REMOTE_DIAL_SHOOT_THRESHOLD (-200)
/* 拨轮值大于该阈值时触发急停，单位遥控器计数；默认 +300，与负方向的发射拨轮阈值区分。 */
#define REMOTE_DIAL_EMERGENCY_THRESHOLD 300
/* 摇杆云台输入死区及安全复位回中范围，单位遥控器计数；默认 ±30，还须满足键鼠释放与开关状态条件。 */
#define REMOTE_NEUTRAL_THRESHOLD 30

/* DBUS 键鼠与图传键鼠的底盘速度、云台鼠标灵敏度。 */
/* DBUS 键鼠的单轴底盘速度指令幅值，单位既有速度指令；默认 300，供保留的 MouseKeySet 路径使用。 */
#define REMOTE_KEY_CHASSIS_SPEED 300
/* 图传键鼠的单轴基础底盘速度指令，单位既有速度指令；默认 3000，再乘 C 键速度百分比。仅速度控制路径使用。 */
#define REMOTE_VTX_KEY_CHASSIS_SPEED 3000
/* 鼠标位移归一化除数，单位鼠标输入计数；默认 660，yaw/pitch 与 DBUS/图传共用。 */
#define REMOTE_MOUSE_FULL_SCALE 660
/* 归一化鼠标横向输入对应的 yaw 角度增量，单位度/任务周期；默认 10，与 REMOTE_MOUSE_FULL_SCALE 配合调整灵敏度。 */
#define REMOTE_MOUSE_YAW_GAIN 10
/* 归一化鼠标纵向输入对应的 pitch 角度增量，单位度/任务周期；默认 10，与 REMOTE_MOUSE_FULL_SCALE 配合调整灵敏度。 */
#define REMOTE_MOUSE_PITCH_GAIN 10

/* C 键循环切换的速度档位，单位：%。 */
/* C 键循环的第 1 档速度/力比例，单位 %；默认 40，图传速度路径或力控制路径实际应用该比例。 */
#define REMOTE_CHASSIS_SPEED_LEVEL_1 40
/* C 键循环的第 2 档速度/力比例，单位 %；默认 60，按 1→2→3→4→1 循环切换。 */
#define REMOTE_CHASSIS_SPEED_LEVEL_2 60
/* C 键循环的第 3 档速度/力比例，单位 %；默认 80，运行时按 C 键切换。 */
#define REMOTE_CHASSIS_SPEED_LEVEL_3 80
/* C 键循环的第 4 档速度/力比例，单位 %；默认 100，表示完整基础指令。 */
#define REMOTE_CHASSIS_SPEED_LEVEL_4 100

/* 输入/反馈有效期，单位：ms；双板离线重装计数单位沿用 daemon。 */
/* 安全状态允许的遥控器数据最大年龄，单位 ms；默认 100，超时视为输入离线。 */
#define REMOTE_INPUT_MAX_AGE_MS 100U
/* 安全状态允许的云台 yaw 反馈最大年龄，单位 ms；默认 20，超时禁止正常驱动。 */
#define REMOTE_YAW_FEEDBACK_MAX_AGE_MS 20U
/* 双板模式下云台板接收底盘反馈的最大年龄，单位 ms；默认 10，仅 GIMBAL_BOARD 路径使用。 */
#define REMOTE_BOARD_FEEDBACK_MAX_AGE_MS 10U
/* 双板通信 daemon 收到有效数据后的重装计数，单位守护任务周期；默认 10，当前守护任务周期约 10 ms，与上方数据时效检查独立。 */
#define REMOTE_BOARD_OFFLINE_RELOAD_COUNT 10

#endif
