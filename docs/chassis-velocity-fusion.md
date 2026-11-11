# 整车平移速度融合观测

适用于单 A 板、速度控制模式，MPU6500 安装在随 yaw 和 pitch 转动的云台头上。整车观测将四轮原始转子 RPM 反算为底盘速度，用同步的 yaw、pitch 编码器角度与原生 INS 四元数恢复底盘姿态，再把 IMU 去重力加速度换算到底盘中心。在固定导航系对每个水平轴估计速度和加速度，最后将速度转回底盘坐标系。观测结果供记录与对照使用。

## 构建与读取

| 构建选项 | 效果 |
| --- | --- |
| 默认配置 | 关闭本观测 |
| `CHASSIS_VELOCITY_FUSION_ENABLE=1` | 发布原始四轮反算，状态为 `CHASSIS_VELOCITY_UNCALIBRATED` |
| 再加 `CHASSIS_VELOCITY_FUSION_CALIBRATED=1` | 从构建目录读取 `chassis_velocity_fusion_calibration.h` 并运行融合 |

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-observation CHASSIS_VELOCITY_FUSION_ENABLE=1
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-calibrated CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=1
```

第二条命令需要先把实测配置头放入对应的 `BUILD_DIR`。CMake 的同名选项使用对应的二进制目录。配置头定义 `static const ChassisVelocityFusionCalibration_s chassis_velocity_fusion_calibration`，字段如下。

| 字段 | 填法 |
| --- | --- |
| `geometry.yaw_origin_b_m` | 底盘几何中心到 yaw 转轴，底盘系，m |
| `geometry.pitch_origin_yaw_m` | yaw 转轴到 pitch 转轴，yaw 转动后的坐标系，m |
| `geometry.imu_origin_head_m` | pitch 转轴到 MPU6500 芯片，云台头坐标系，m |
| `geometry.sensor_to_head_quaternion` | INS 原生传感器轴到云台头坐标系的安装旋转，`[w,x,y,z]` 单位四元数 |
| `joint_zero_ecd[2]`、`joint_sign[2]` | yaw、pitch 顺序，编码器零位 `0…8191` 和方向 `±1`。坐标系为前、左、上，正 yaw 绕上轴，正 pitch 绕左轴低头 |
| `filter` | 按实测辨识 `wheel_variance`、`accel_variance`、`jerk_density`、`max_velocity_residual_mps`。输入时间配置可用 `max_age_us=45000`、`max_skew_us=3000`、`max_gap_us=40000` 作为软件验证起点，覆盖中点校正的延迟和输入偏斜 |

调试器可读取 `chassis_fused_velocity`，固件代码通过 `ChassisGetFusedVelocitySnapshot()` 取得一致快照。

- `valid` 表示速度可用。只有 `status == CHASSIS_VELOCITY_FUSED` 才表示用了 IMU。
- `vx_mps`、`vy_mps` 为底盘系前向、右向速度，单位 m/s。融合时，`paired_wheel_mps` 是与融合配对的轮速反算，适合做同时间对照。`wheel` 另外保留最新的四轮反算和自身时间戳。
- `tick_ms` 是估计的时间戳。中心加速度校正用三点居中差分，每侧间隔 15–30 ms，融合结果有至少一侧采样间隔的延迟。`center_accel_n_mps2` 为融合所用的导航系中心平移加速度。
- IMU、云台编码器、时间对齐或滤波异常时，状态标出原因，估计退回当前轮速反算。四轮反馈不可用时，`valid` 为假。

## 实车验证

1. 校准实际轮径、轮向、两轴编码器零位和方向、转轴偏置及 IMU 安装姿态。[合成配置头](../tests/chassis_velocity_observer/stubs/chassis_velocity_fusion_calibration.h)仅供主机和固件构建验证。
2. 底盘保持静止，先记录云台静止基线，再分别转 yaw、pitch，最后做两轴组合动作。同步检查中心加速度和融合速度是否随云台运动产生偏差。
3. 采集低速直行、横移、起停、换向及自旋。按时间戳对比融合值和配对轮速反算，使用独立速度参考评价实际误差。CAN 和 SPI 时间戳记录的是 MCU 接收或读回时刻，电调和 IMU 内部的采样及滤波延迟需要实测评估。
4. 检查传感器失效时的降级、恢复时的稳定性，以及启用观测后的任务最大耗时。当前标定和实车精度尚未验证。
