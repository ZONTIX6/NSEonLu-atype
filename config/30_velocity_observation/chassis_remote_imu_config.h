#ifndef CHASSIS_REMOTE_IMU_CONFIG_H
#define CHASSIS_REMOTE_IMU_CONFIG_H

/* 第二块板固定到底盘的融合观测参数。修改后重新构建/烧录主板。
 * 初值仅支持名义安装的观测试运行，不是实车标定；不参与底盘控制。
 * 内部坐标 +X前/+Y左/+Z上；对外速度接口仍为前/右正方向。
 * 名义安装：传感器轴与底盘轴重合，芯片在底盘中心。必须按实物修改；
 * 若安装方向/芯片偏移不符，FUSED也不能证明速度精度。
 */
#ifndef CHASSIS_REMOTE_IMU_CONFIG_VERIFIED
#define CHASSIS_REMOTE_IMU_CONFIG_VERIFIED 0 /* 0=未实测初值；完成安装/参数审核后才设1；只作记录标识，不放行控制。 */
#endif
#if CHASSIS_REMOTE_IMU_CONFIG_VERIFIED != 0 && CHASSIS_REMOTE_IMU_CONFIG_VERIFIED != 1
#error CHASSIS_REMOTE_IMU_CONFIG_VERIFIED must be 0 or 1.
#endif

/* Hamilton [w,x,y,z]，从传感器坐标旋转到固定底盘坐标。 */
#define CHASSIS_REMOTE_IMU_SENSOR_TO_BODY {1.0f, 0.0f, 0.0f, 0.0f}
/* 底盘中心到MPU6500芯片的向量，底盘前/左/上，单位m。 */
#define CHASSIS_REMOTE_IMU_OFFSET_BODY_M {0.0f, 0.0f, 0.0f}

/* 观测试运行初值；分别为(m/s)^2、(m/s²)^2、m²/s⁵、m/s。
 * 无实测证据时不把这些数值或VERIFIED=1当作验收结果。 */
#define CHASSIS_REMOTE_IMU_WHEEL_VARIANCE 0.01f
#define CHASSIS_REMOTE_IMU_ACCEL_VARIANCE 0.25f
#define CHASSIS_REMOTE_IMU_JERK_DENSITY 1.0f
#define CHASSIS_REMOTE_IMU_MAX_RESIDUAL_MPS 1.5f

/* 配对只约束主板估计时刻，不证明跨板真实同步。
 * 源年龄<=20ms、已知年龄<=60ms、当前轮速<=20ms另由适配器检查。
 * 三点中心补偿每侧15~30ms；漏一组50Hz数据会重置后重新预热。 */
#define CHASSIS_REMOTE_IMU_MAX_PAIR_SPAN_MS 15U
#define CHASSIS_REMOTE_IMU_MAX_WHEEL_SKEW_MS 3U
#define CHASSIS_REMOTE_IMU_MAX_TIMING_JITTER_MS 10U

/* 展开到 ChassisRemoteFusionConfig_s；不在头文件内产生重复静态实例。 */
#define CHASSIS_REMOTE_IMU_CONFIG_INITIALIZER { \
    .sensor_to_body_quaternion = CHASSIS_REMOTE_IMU_SENSOR_TO_BODY, \
    .imu_offset_body_m = CHASSIS_REMOTE_IMU_OFFSET_BODY_M, \
    .filter = { .wheel_variance = CHASSIS_REMOTE_IMU_WHEEL_VARIANCE, \
        .accel_variance = CHASSIS_REMOTE_IMU_ACCEL_VARIANCE, \
        .jerk_density = CHASSIS_REMOTE_IMU_JERK_DENSITY, \
        .max_velocity_residual_mps = CHASSIS_REMOTE_IMU_MAX_RESIDUAL_MPS, \
        .max_age_us = 60000U, .max_skew_us = 0U, .max_gap_us = 60000U }, \
    .max_pair_span_ms = CHASSIS_REMOTE_IMU_MAX_PAIR_SPAN_MS, \
    .max_wheel_skew_ms = CHASSIS_REMOTE_IMU_MAX_WHEEL_SKEW_MS, \
    .max_timing_jitter_ms = CHASSIS_REMOTE_IMU_MAX_TIMING_JITTER_MS }

#endif
