#ifndef CHASSIS_REMOTE_IMU_CONFIG_H
#define CHASSIS_REMOTE_IMU_CONFIG_H

/* 下方A板固定底盘IMU的融合观测参数。修改后重新构建/烧录主板。
 * 初值仅支持名义安装的观测试运行，不是实车标定；不参与底盘控制。
 * 内部坐标 +X前/+Y左/+Z上；对外速度接口仍为前/右正方向。
 * 用户图示为+X前/+Y右：采集板在图示+Y方向116mm，转换到内部坐标为-Y。
 * 安装旋转由官方A板U3位号图、MPU6500图14与照片推导，仍需轴向实测确认。
 * 若安装方向/芯片偏移不符，FUSED也不能证明速度精度。
 */
#ifndef CHASSIS_REMOTE_IMU_CONFIG_VERIFIED
#define CHASSIS_REMOTE_IMU_CONFIG_VERIFIED 0 /* 0=未实测初值；完成安装/参数审核后才设1；只作记录标识，不放行控制。 */
#endif
#if CHASSIS_REMOTE_IMU_CONFIG_VERIFIED != 0 && CHASSIS_REMOTE_IMU_CONFIG_VERIFIED != 1
#error CHASSIS_REMOTE_IMU_CONFIG_VERIFIED must be 0 or 1.
#endif

/* Hamilton [w,x,y,z]，从传感器坐标旋转到固定底盘前/左/上。
 * 照片中USB朝上/XT30朝下/板正面朝上，底盘前方朝照片右：
 * 芯片+X向后、+Y向右、+Z向上，故绕Z旋转180度；推导见双板集成说明。 */
#define CHASSIS_REMOTE_IMU_SENSOR_TO_BODY {0.0f, 0.0f, 0.0f, 1.0f}
/* 底盘中心到IMU采样点，前/左/上，单位m；用户+Y(右)116mm -> 内部-Y。
 * 用户已确认116mm量到MPU6500芯片位置，不是A板中心。
 * 由主板中心补偿一次性扣除偏移引起的向心/切向加速度；采集板不再扣除。 */
#define CHASSIS_REMOTE_IMU_OFFSET_BODY_M {0.0f, -0.116f, 0.0f}

/* 观测试运行初值；分别为(m/s)^2、(m/s²)^2、m²/s⁵、m/s。
 * 无实测证据时不把这些数值或VERIFIED=1当作验收结果。 */
#define CHASSIS_REMOTE_IMU_WHEEL_VARIANCE 0.01f
#define CHASSIS_REMOTE_IMU_ACCEL_VARIANCE 0.25f
#define CHASSIS_REMOTE_IMU_JERK_DENSITY 1.0f
#define CHASSIS_REMOTE_IMU_MAX_RESIDUAL_MPS 1.5f

/* 配对用首帧主板接收时刻减去采集板发送时源年龄，仍非硬件同步；
 * 中心补偿dt使用采集板源时钟，不能直接相减两块板的绝对时间。
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
