# 整车平移速度融合观测

适用于单 A 板、速度控制模式，MPU6500 安装在随 yaw 和 pitch 转动的云台头上。整车观测将四轮原始转子 RPM 反算为底盘速度，用同步的 yaw、pitch 编码器角度与原生 INS 四元数恢复底盘姿态，再把 IMU 去重力加速度换算到底盘中心。在固定导航系对每个水平轴估计速度和加速度，最后将速度转回底盘坐标系。观测结果供记录与对照使用。

这是当前已实现的云台 IMU 观测路径。同一工程另外提供第二块 A 板的仅 IMU 采集模式：将其刚性固定在底盘、尽量靠近几何中心，独立记录 IMU 后与主板 CSV 离线配对。当前没有两板通信或硬件同步，第二块 IMU 尚未实时接入本观测器。云台杆臂实测不是这条双板采集路线的前置条件。

## 构建与读取

| 构建选项 | 效果 |
| --- | --- |
| 默认配置 | 关闭本观测 |
| `CHASSIS_VELOCITY_FUSION_ENABLE=1` | 发布原始四轮反算，以及四轮/INS/云台原始诊断；四轮有效时状态为 `CHASSIS_VELOCITY_UNCALIBRATED` |
| 再加 `CHASSIS_VELOCITY_FUSION_CALIBRATED=1` | 从构建目录读取 `chassis_velocity_fusion_calibration.h` 并运行融合 |

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-observation CHASSIS_VELOCITY_FUSION_ENABLE=1
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-calibrated CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=1
```

第二条命令需要先把目标车实测配置头放入对应的 `BUILD_DIR`。不能用 `tests/chassis_velocity_observer/stubs` 的合成参数代替实车标定。CMake 的同名选项使用对应的二进制目录。配置头定义 `static const ChassisVelocityFusionCalibration_s chassis_velocity_fusion_calibration`，字段如下。

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
- `diagnostics` 在同一采集临界区保存四轮 raw rpm/接收时间、INS 四元数/去重力加速度/角速度/时间/序号，以及 yaw/pitch 原始编码器/转速/时间。未标定构建也发布这些输入。`available` 表示已采集过，调用方还需检查各来源有效位和年龄；getter 判定速度过期时不会擦除这些诊断。

## FireWater 与 CSV

首次采集可直接使用未标定模式：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/kalman-firewater VOFA_TELEMETRY_MODE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0
```

第二块底盘 IMU 板使用同工程的独立采集构建：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/imu-firewater IMU_CAPTURE_ONLY=1 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=1 USB_TELEMETRY_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=0
```

该模式保留 INS 采样、加热和遥测，不初始化电机、底盘、云台或遥控，不运行底盘速度融合。分别将两份固件烧入对应 A 板，通过各自的 UART7 记录，不并接两个 TX。

通过 UART7 以 **115200、8N1** 连接 VOFA+ 并选择 FireWater。schema 2 固定 64 列，间隔至少 80 ms（标称 12.5 Hz），替换原有 38 列 FireWater 测量格式；mode 0 的 19 路 JustFloat 保持兼容。版本库默认 mode 0，本地 `board_config.h` 或构建参数可以覆盖；上面命令明确选择 mode 1。[完整列、有效位、时间和采集步骤](vofa-telemetry.md#firewater-卡尔曼观测通道表schema-2)

主板记录的 `I63=0`。未完成实测标定时，四轮有效的 `I6=2` 表示 `UNCALIBRATED`，`I27`/`I28` 仍是轮速反算，`I29`–`I32` 尚不可用；该记录的 `I36`–`I45` 来自云台 IMU。若另行验证本页的云台 IMU 融合路径，只有对应的实测标定头就绪后才开启 `CHASSIS_VELOCITY_FUSION_CALIBRATED=1`。

独立中心 IMU 板记录的 `I63=1`、`I6=-2`、`I5` bit 14 为 1；测量有效位只使用 bit 0 和 bit 4。该板提供 `I1`–`I4` 时间/发送序号、`I10` 采集年龄和 `I33`–`I45` IMU 时间/序号/四元数/去重力加速度/角速度，其他控制或观测值不可用。`I5` bit 13 表示供电配置（1 超电，0 直供），`I63` 专用于区分板角色。

FireWater 的平移速度使用 mm/s，加速度使用 mm/s²，IMU 角速度使用 mrad/s。`I23` 和 `I26` 的车体旋转速度使用 mrev/s（`1000 × 转/秒`），由 `wz_rpm × 1000/60` 得到，不能按 rpm 读取。`I27`/`I28` 与配对轮速 `I29`/`I30` 用于融合对应时刻的比较，估计年龄为 `I11`；最新轮速和原始 IMU 另有年龄，不能把同一 CSV 行视为所有传感器同时采样。两组速度之差是后验差，不能当成 innovation 来直接拟合 R。

每个工况同时保存主板与中心 IMU 板两份 CSV：静止 30–60 s、低速前后起停/换向 30–60 s、低速左右起停/换向 30–60 s，以及两个方向自旋各 20–30 s。可选记录底盘静止时仅 yaw 或仅 pitch 往复各 20–30 s，核对现有 IMU 来源。各动作前后保留约 5 s 静止，重复 3–5 次。保留 `I0`–`I63` 和 VOFA+ 时间列，并附两块板的固件身份、构建参数、安装轴向/位置、标定头（若有）和实际动作方向。

两端均开始录制后，同场记录明显的“静止—低速起步—停车—静止”重复序列，供轮速与中心加速度粗配准。两块板独立计时，不能直接按开机毫秒、CSV 行号或电脑接收时间配对；上述序列也不能实现毫秒级同步。离线分析还需考虑时钟漂移、传感器延迟与两板各自导航系的方向。第二板能输出数据不代表已经参与主板实时融合。

80 ms 降采样可支持初始参数分析，不能逐次重放滤波器或完整辨识 Q/R。静止和慢动作可帮助区分噪声、残余偏置、杆臂影响和响应滞后；评价实际车速误差及打滑表现还需要独立速度参考。

## 现有云台 IMU 融合路径的实车验证

以下针对本页已有的云台 IMU 融合路径；使用第二块底盘中心采集板时，不把该路径的杆臂参数直接移作中心 IMU 配置。

1. 校准实际轮径、轮向、两轴编码器零位和方向、转轴偏置及 IMU 安装姿态。[合成配置头](../tests/chassis_velocity_observer/stubs/chassis_velocity_fusion_calibration.h)仅供主机和固件构建验证。
2. 底盘保持静止，先记录云台静止基线，再分别转 yaw、pitch，最后做两轴组合动作。同步检查中心加速度和融合速度是否随云台运动产生偏差。
3. 采集低速直行、横移、起停、换向及自旋。按时间戳对比融合值和配对轮速反算，使用独立速度参考评价实际误差。CAN 和 SPI 时间戳记录的是 MCU 接收或读回时刻，电调和 IMU 内部的采样及滤波延迟需要实测评估。
4. 检查传感器失效时的降级、恢复时的稳定性，以及启用观测后的任务最大耗时。当前标定和实车精度尚未验证。
