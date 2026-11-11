#include "pid_config.h"

/* 调整方法：修改下方对应实例的字段值并重新编译、烧录。未显式填写的字段默认为 0。
 * 每组注明误差单位 E 和输出单位 U；dt 使用秒，因此 Ki 的单位为 U/(E·s)，Kd 为 U·s/E。
 * Improve 为位标志：0 关闭全部改善环节；用 | 组合所需选项，删去某项即可关闭该项。
 * PID_Integral_Limit=0x01：积分限幅；PID_Derivative_On_Measurement=0x02：微分先行；
 * PID_Trapezoid_Intergral=0x04：梯形积分；PID_OutputFilter=0x10：输出低通。
 * 本文件中三个改善标志的组合默认值为 0x07；仅积分限幅的默认值为 0x01。
 * 底盘 Flash 恢复/在线调参可能覆盖默认值，修改源码后应同时核对运行时实际参数。 */

/* 底盘四个 M3508 共用的速度环，误差单位为度/秒。
 * Flash 保存值和在线调参可在启动后覆盖电机实例中的默认参数。 */
/* 误差 E：轮速差（度/秒）；输出 U：DJI CAN 电流命令刻度，不是安培。 */
const PID_Init_Config_s pid_chassis_speed_config = {
    .Kp = 0.75f, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .IntegralLimit = 3000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 | 增删标志切换，0 关闭全部。 */
    .MaxOut = 15000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
    .Output_LPF_RC = 0.3, /* 输出低通时间常数，单位秒；需 Improve 含 PID_OutputFilter 才生效，当前默认未启用该标志。 */
};

/* 云台 Yaw 轴：角度环。 */
/* 误差 E：Yaw 角度差（度）；输出 U：角速度目标（当前 A 板为度/秒）。 */
const PID_Init_Config_s pid_gimbal_yaw_angle_config = {
    .Kp = 8, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .DeadBand = 0.1, /* 误差死区，单位 E；误差绝对值不超过此值时输出归零。 */
    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 | 增删标志切换，0 关闭全部。 */
    .IntegralLimit = 100, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 500, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 云台 Yaw 轴：速度环。 */
/* 误差 E：Yaw 角速度差（当前 A 板为度/秒）；输出 U：DJI CAN 电流命令刻度。 */
const PID_Init_Config_s pid_gimbal_yaw_speed_config = {
    .Kp = 50, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 200, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 | 增删标志切换，0 关闭全部。 */
    .IntegralLimit = 3000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 20000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 云台 Pitch 轴：角度环。 */
/* 误差 E：Pitch 角度差（度）；输出 U：角速度目标（当前 A 板为度/秒）。 */
const PID_Init_Config_s pid_gimbal_pitch_angle_config = {
    .Kp = 10, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 | 增删标志切换，0 关闭全部。 */
    .IntegralLimit = 100, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 500, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 云台 Pitch 轴：速度环。 */
/* 误差 E：Pitch 角速度差（当前 A 板为度/秒）；输出 U：DJI CAN 电流命令刻度。 */
const PID_Init_Config_s pid_gimbal_pitch_speed_config = {
    .Kp = 50, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 350, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Trapezoid_Intergral | PID_Integral_Limit | PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 | 增删标志切换，0 关闭全部。 */
    .IntegralLimit = 2500, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 20000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 发射机构左右摩擦轮共用的速度环。 */
/* 误差 E：摩擦轮转子角速度差（度/秒）；输出 U：电流环目标（DJI 原始反馈刻度）。 */
const PID_Init_Config_s pid_shoot_friction_speed_config = {
    .Kp = 20, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 | 加入其他改善标志。 */
    .IntegralLimit = 10000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 15000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 发射机构左右摩擦轮共用的电流环。 */
/* 误差 E：摩擦轮电流差（DJI 原始反馈刻度）；输出 U：DJI CAN 电流命令刻度。 */
const PID_Init_Config_s pid_shoot_friction_current_config = {
    .Kp = 0.7, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 0.1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 | 加入其他改善标志。 */
    .IntegralLimit = 10000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 15000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 拨弹盘角度环。若启用位置环控制发弹，需要较大的 I 值保证输出力矩的线性度，
 * 否则接近拨出位置时力矩会大幅下降；这里保留原有默认值。 */
/* 误差 E：拨弹电机转子角度差（度）；输出 U：转子角速度目标（度/秒）。 */
const PID_Init_Config_s pid_shoot_loader_angle_config = {
    .Kp = 10, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .MaxOut = 5000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 拨弹盘速度环。 */
/* 误差 E：拨弹电机转子角速度差（度/秒）；输出 U：电流环目标（DJI 原始反馈刻度）。 */
const PID_Init_Config_s pid_shoot_loader_speed_config = {
    .Kp = 10, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 | 加入其他改善标志。 */
    .IntegralLimit = 5000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 5000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 拨弹盘电流环。 */
/* 误差 E：拨弹电机电流差（DJI 原始反馈刻度）；输出 U：DJI CAN 电流命令刻度。 */
const PID_Init_Config_s pid_shoot_loader_current_config = {
    .Kp = 0.7, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 0.1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 | 加入其他改善标志。 */
    .IntegralLimit = 5000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .MaxOut = 5000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
};

/* 当前 A 板 MPU6500 温控，输出为占空比百分数（0～100）。 */
/* 误差 E：温差（℃）；输出 U：加热占空比百分数（百分比点）。 */
const PID_Init_Config_s pid_imu_mpu6500_temperature_config = {
    .MaxOut = 100, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
    .IntegralLimit = 15, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .DeadBand = 0, /* 误差死区，单位 E；误差绝对值不超过此值时输出归零。 */
    .Kp = 50, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = 0x01, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 | 加入其他改善标志。 */
};

/* 可选 BMI088 温控，供旧 ins_task.c 使用，输出为定时器比较值。
 * 与 A 板占空比输出的量纲不同，不应互换两组参数。 */
/* 误差 E：温差（℃）；输出 U：加热定时器比较值（计数刻度）。 */
const PID_Init_Config_s pid_imu_bmi088_temperature_config = {
    .MaxOut = 2000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */
    .IntegralLimit = 300, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */
    .DeadBand = 0, /* 误差死区，单位 E；误差绝对值不超过此值时输出归零。 */
    .Kp = 1000, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */
    .Ki = 20, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */
    .Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */
    .Improve = 0x01, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 | 加入其他改善标志。 */
};
