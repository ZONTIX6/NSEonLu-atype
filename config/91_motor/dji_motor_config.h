#ifndef DJI_MOTOR_CONFIG_H
#define DJI_MOTOR_CONFIG_H

/* 修改下方宏值后重新编译、烧录；直接定义的宏不通过 -D 覆盖。
 * 此文件仅由 DJI 驱动包含，不共用于其他电机驱动。 */
#define DJI_MOTOR_CNT 12 /* 可注册 DJI 电机实例上限，单位个；默认 12，同时决定实例/采样缓存容量。 */

/* 反馈滤波系数：设为 1 时关闭滤波；沿用 DJI 驱动现有宏名。 */
#define SPEED_SMOOTH_COEF 0.85f /* 速度新样本权重，无量纲；默认 0.85，旧值权重为 1−本值，改为 1 关闭速度平滑。 */
#define CURRENT_SMOOTH_COEF 0.9f /* 电流新样本权重，无量纲；默认 0.9，旧值权重为 1−本值，改为 1 关闭电流平滑。 */

#endif
