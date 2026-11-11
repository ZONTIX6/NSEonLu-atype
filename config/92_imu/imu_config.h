#ifndef IMU_CONFIG_H
#define IMU_CONFIG_H

/* 修改下方数值后重新编译、烧录；这些宏直接定义，不通过 -D 覆盖。
 * MPU6500 与旧 BMI088 参数分开命名，避免将不同硬件标定值混用。
 * 下列安装/磁力计标记当前无消费代码，修改它们不会改变实际安装变换或启停磁力计。 */
#define IMU_MPU6500_BOARD_DOWN (1) /* 保留的安装方向标记，默认 1；当前未接入逻辑，不支持用 0/1 切换实际朝向。 */
#define IMU_MPU6500_USE_IST8310 /* 保留的磁力计标记，默认已定义；当前未接入逻辑，删除定义也不会实际关闭磁力计。 */

/* A 板 MPU6500 陀螺仪零偏校准限制和超时回退标定值，单位 rad/s。 */
#define IMU_MPU6500_GYRO_OFFSET_LIMIT_X 0.04f /* X 轴零偏标定允许的绝对值门槛，单位 rad/s；超出时重试标定。 */
#define IMU_MPU6500_GYRO_OFFSET_LIMIT_Y 0.04f /* Y 轴零偏标定允许的绝对值门槛，单位 rad/s；超出时重试标定。 */
#define IMU_MPU6500_GYRO_OFFSET_LIMIT_Z 0.04f /* Z 轴零偏标定允许的绝对值门槛，单位 rad/s；超出时重试标定。 */
#define IMU_MPU6500_GYRO_OFFSET_X .037452f /* 标定超时回退的 X 轴陀螺仪零偏，单位 rad/s。 */
#define IMU_MPU6500_GYRO_OFFSET_Y .003360f /* 标定超时回退的 Y 轴陀螺仪零偏，单位 rad/s。 */
#define IMU_MPU6500_GYRO_OFFSET_Z .008532f /* 标定超时回退的 Z 轴陀螺仪零偏，单位 rad/s。 */
#define IMU_MPU6500_GRAVITY_NORM 9.605101f /* 回退标定的重力模长，单位 m/s²，用于计算加速度比例补偿。 */

/* A 板 MPU6500 恒温目标、姿态 EKF 和运动加速度低通参数。
 * 温控 PID 参数位于 pid_config.c。 */
#define IMU_MPU6500_REF_TEMP_C 40 /* MPU6500 恒温目标，单位 ℃；温控增益在 pid_config.c 中调整。 */
#define IMU_MPU6500_EKF_PROCESS_NOISE_1 10 /* 四元数状态过程噪声系数 Q1；算法以 Q1×dt 写入协方差，按 EKF 内部尺度调参。 */
#define IMU_MPU6500_EKF_PROCESS_NOISE_2 0.001 /* 陀螺仪零偏过程噪声系数 Q2；算法以 Q2×dt 写入协方差，按 EKF 内部尺度调参。 */
#define IMU_MPU6500_EKF_MEASUREMENT_NOISE 1000000 /* 归一化加速度观测的噪声协方差 R，按 EKF 内部尺度调参，不是原始加速度幅值。 */
#define IMU_MPU6500_EKF_LAMBDA 1 /* 零偏协方差渐消因子，无量纲；默认 1 不膨胀协方差，算法用协方差除以此值。 */
#define IMU_MPU6500_EKF_ACCEL_LPF 0 /* EKF 输入加速度低通时间常数，单位秒；默认 0 直通，改为正时间常数启用低通。 */
#define IMU_MPU6500_MOTION_ACCEL_LPF_S 0.0085 /* 去重力后的运动加速度低通时间常数，单位秒；改值后影响平滑程度与响应滞后。 */

/* 可选 BMI088（旧 ins_task.c）单独保留参数，便于独立调整。 */
#define IMU_BMI088_REF_TEMP_C 40 /* BMI088 恒温目标，单位 ℃；温控增益在 pid_config.c 中调整。 */
#define IMU_BMI088_EKF_PROCESS_NOISE_1 10 /* 四元数状态过程噪声系数 Q1；算法以 Q1×dt 写入协方差，按 EKF 内部尺度调参。 */
#define IMU_BMI088_EKF_PROCESS_NOISE_2 0.001 /* 陀螺仪零偏过程噪声系数 Q2；算法以 Q2×dt 写入协方差，按 EKF 内部尺度调参。 */
#define IMU_BMI088_EKF_MEASUREMENT_NOISE 1000000 /* 归一化加速度观测的噪声协方差 R，按 EKF 内部尺度调参，不是原始加速度幅值。 */
#define IMU_BMI088_EKF_LAMBDA 1 /* 零偏协方差渐消因子，无量纲；默认 1 不膨胀协方差，算法用协方差除以此值。 */
#define IMU_BMI088_EKF_ACCEL_LPF 0 /* EKF 输入加速度低通时间常数，单位秒；默认 0 直通，改为正时间常数启用低通。 */
#define IMU_BMI088_MOTION_ACCEL_LPF_S 0.0085 /* 去重力后的运动加速度低通时间常数，单位秒；改值后影响平滑程度与响应滞后。 */

#endif
