# 整车平移速度融合观测

当前推荐采集路线是：第二块 A 板刚性固定在底盘中心附近，经 **CAN1** 将本地 IMU 送到主板，再由主板 UART7 汇总为 **一个 VOFA+ 窗口、一份 CSV**。主板保留未标定轮速观测，远端 IMU 用于记录和离线分析，尚未参与速度融合或控制。

本页后半部分保留原有的云台 IMU 融合路径及标定说明。云台杆臂实测不是 CAN1 底盘 IMU 采集的前置条件；底盘 IMU 也不能直接代入云台杆臂补偿算法。

## 推荐：CAN1 汇总到一份 CSV

主底盘/云台板构建：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/kalman-can1-main CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=1
```

第二块底盘 IMU 板构建：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/imu-can1-sender IMU_CAPTURE_ONLY=1 IMU_CAPTURE_CAN_TX_ENABLE=1 VOFA_TELEMETRY_ENABLE=0 USB_TELEMETRY_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=0
```

第二板只运行 INS 采样、温控和 CAN1 发送，不初始化电机、底盘、云台或遥控。分别将两份固件烧入对应板。沿主板现有 CAN1 总线接入第二板，连接 `CAN_H` 对 `CAN_H`、`CAN_L` 对 `CAN_L` 和参考地；按板卡丝印及实际接线图确认物理引脚。终端电阻按总线两端配置，不因增加节点盲目添加第三只终端。

当前 CAN1 为 **1 Mbps**，新增标准 ID **`0x520`** 与当前工程已有 ID 无冲突。每 20 ms 尝试发送一组（50 Hz），每组 9 个 8 字节分片，承载源四元数 4 轴、导航系去重力加速度 3 轴、传感器系角速度 3 轴、样本序号、源板时间、发送时源年龄和有效标志。接收检查严格顺序、同组 tag、CRC16/CCITT-FALSE 和 10 ms 组装期限。发送失败即中止本组，缺片、错序或过期组不更新已发布样本。

仅主板 UART7 接 VOFA+，选择 **FireWater、115200、8N1**。schema 3 固定 72 列，发送间隔至少 80 ms（标称 12.5 Hz）；导出完整 **`I0`–`I71`**。启动先静止，确认 `I0=3`、`I63=0`、`I64=2`、`I5` bit 15 为 1，再等待 bit 4 持续有效且 `I65` 接收年龄保持较小。来源标记本身不表示远端数据已经有效。[完整字段与采集步骤](vofa-telemetry.md#firewater-卡尔曼观测通道表)

schema 3 保留 schema 2 前 64 列的布局，主要区别如下：

| 字段 | CAN1 路线含义 |
| --- | --- |
| `I0` | schema 版本 `3` |
| `I5` bit 4 / bit 15 | 远端 IMU 当前有效 / 已选择远端 CAN IMU 模式 |
| `I33`–`I45` | 第二板的样本年龄下界、源序号、四元数、去重力加速度和角速度；远端失效时标无效，不退回本地云台 IMU |
| `I63` / `I64` | 记录板为主板 `0` / IMU 来源为 CAN1 `2` |
| `I65` / `I66` | 主板完整组接收年龄 / 第二板发送时源样本年龄，ms |
| `I67`、`I68` | 源板样本毫秒时间的高、低 16 位，仅诊断 |
| `I69` | 本组首末片主板接收间隔，ms |
| `I70`、`I71` | 累计拒绝组数的高、低 16 位，达到 `UINT32_MAX` 后饱和 |

四轮有效时 `I6=2`（`UNCALIBRATED`），`I27`/`I28` 仍来自轮速，bit 3 为 0，`I29`–`I32` 为 0。收到第二板 IMU 不会启用卡尔曼速度融合，也不会改变底盘控制。

### 时间和有效性

源有效且完整组校验通过后，远端可用还要求发送时源年龄 `I66≤20 ms`、已知年龄下界 `I33≤60 ms`。`I33 = 发送时源年龄 + (主板当前时间 − 本组首片接收时间)`；它没有包含首片排队、总线仲裁前等待或传感器内部延迟，因此不是精确端到端年龄，也不是硬件同步后的采样时刻。`I65` 从完整组接收时刻算起，`I69` 表示分片到达跨度。无效测量填 0，未知或不可表示的年龄填 `-1`，使用数据必须检查 bit 4。

高低位按 `高位×65536+低位` 还原。时钟、发送序号和源 INS 序号按 uint32 回绕；累计拒绝组数达到 `UINT32_MAX` 后饱和，不回绕。两板独立计时，不能用主板 `I1/I2` 直接减源板 `I67/I68`，也不能把主板时间减 `I33` 当成精确的跨板采样时刻。单份 CSV 只是汇总不同来源的最新观测；离线比较仍须考虑各自年龄、传感器延迟及第二板导航系方向。

### 采集工况与交付

两板上电后保持静止，等待启动标定与远端有效位稳定。每个工况保存一份 CSV：静止 30–60 s、低速前后起停/换向 30–60 s、低速左右起停/换向 30–60 s，以及两个方向自旋各 20–30 s。各动作前后保留约 5 s 静止，重复 3–5 次，保留起停与换向过程。

提交完整 `I0`–`I71`、原有表头和 VOFA+ 时间列，并附两板的固件身份、完整构建命令、第二板安装轴向、MPU6500 芯片相对底盘中心的偏移、实际动作方向、地面与载荷，以及拒绝计数增长或抖动情况。靠近几何中心不等于偏移严格为零。

平移速度使用 mm/s，加速度使用 mm/s²，IMU 角速度使用 mrad/s。`I23`、`I26` 的车体旋转速度为 mrev/s（`1000×转/秒`），由 `wz_rpm×1000/60` 得到。80 ms CSV 是降采样，不能逐次重放高频滤波器或完整辨识 Q/R；实际速度误差和打滑表现还需独立速度参考。

## 可选：独立双 CSV 采集

不接 CAN1 时仍可分别记录两板 UART7。关闭主板 `CHASSIS_IMU_CAN_RX_ENABLE` 和第二板 `IMU_CAPTURE_CAN_TX_ENABLE`，第二板保持 `IMU_CAPTURE_ONLY=1` 并开启 FireWater 输出。该路线使用 schema 2 的 64 列，每个工况保存主板与第二板各一份 CSV，需要两个独立接收通道，不并接两个 TX。[两份完整构建命令与配对步骤](vofa-telemetry.md#可选独立双-csv-采集)

schema 2 主板 `I63=0`，IMU 列来自本地云台；独立第二板 `I63=1`、`I6=-2`、`I5` bit 14 为 1，IMU 列来自底盘。第二板仅有 bit 0 和 bit 4 对应的测量有效，保留时间、源序号及 IMU 数据，其余控制或观测测量不可用。保存各自完整 `I0`–`I63`，再离线处理时间偏移、时钟漂移和坐标方向。不能直接按 CSV 行号或两板开机毫秒配对。

## 现有云台 IMU 融合路径：构建与读取

以下适用于原有单 A 板、速度控制模式，MPU6500 随云台 yaw 和 pitch 转动；验证此路径时关闭 `CHASSIS_IMU_CAN_RX_ENABLE`。观测器用四轮原始转子 RPM 反算底盘速度，结合时间对齐的 yaw/pitch 编码器与本地 INS 四元数恢复底盘姿态，再将 IMU 去重力加速度补偿到底盘中心，在导航系估计速度后转回底盘坐标系。结果用于记录与对照。

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

本地融合输出可用 schema 2 FireWater 记录。只有 `I5` bit 3 为 1 时，`I27`/`I28` 才是融合结果，`I29`/`I30` 为对应时刻的配对轮速，估计年龄为 `I11`。最新轮速和本地 IMU 另有年龄，不能把整行当成同时采样。估计减配对轮速是后验差，不是校正前 innovation，不能直接用来拟合 R。

## 现有云台 IMU 融合路径的实车验证

以下针对本页已有的云台 IMU 融合路径；使用第二块底盘中心采集板时，不把该路径的杆臂参数直接移作中心 IMU 配置。

1. 校准实际轮径、轮向、两轴编码器零位和方向、转轴偏置及 IMU 安装姿态。[合成配置头](../tests/chassis_velocity_observer/stubs/chassis_velocity_fusion_calibration.h)仅供主机和固件构建验证。
2. 底盘保持静止，先记录云台静止基线，再分别转 yaw、pitch，最后做两轴组合动作。同步检查中心加速度和融合速度是否随云台运动产生偏差。
3. 采集低速直行、横移、起停、换向及自旋。按时间戳对比融合值和配对轮速反算，使用独立速度参考评价实际误差。CAN 和 SPI 时间戳记录的是 MCU 接收或读回时刻，电调和 IMU 内部的采样及滤波延迟需要实测评估。
4. 检查传感器失效时的降级、恢复时的稳定性，以及启用观测后的任务最大耗时。当前标定和实车精度尚未验证。
