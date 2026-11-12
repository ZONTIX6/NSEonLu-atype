# 整车平移速度融合观测

当前推荐路线是：**DM-IMU-L1 刚性固定在底盘，经 CAN2** 将 IMU 送到主板，与轮速历史配对后在线运行卡尔曼速度观测，再由主板 UART7 以 JustFloat 汇总到 **一个 VOFA+ 窗口**，采集后由 VOFA+ 导出一份 CSV。融合结果仅供记录与对照，没有接入电机控制、PID、功控、随动、自旋或安全门控。

远端路径不依赖本地云台 IMU 或 yaw/pitch 编码器；它使用固定安装旋转和芯片偏移。本页后半部分保留可选的本地云台融合路径，两者配置不能混用。软件接入不代表实车精度通过；操作顺序和签字条件见 [测试与验收手册](kalman-chassis-test-and-acceptance.md)。

## 推荐：DM-IMU-L1 接 CAN2

日常运行 `scripts/build-board.ps1 -Action Show -Board 1` 核对 DM 接收与融合开关均为 `1`，再用 `-Action Build -Board 1` 构建主板。DM 模块通过厂商上位机设为 CAN 1 Mbps、主动上报 100 Hz，加速度/角速度/四元数开启（Euler 可选），`MST_ID` 与 `config/92_imu/dm_imu_config.h` 的 `DM_IMU_CAN_RX_ID` 一致，项目示例为 `0x11`。主板被动接收，不自动改写模块设置，也无需第二块 A 板。

接收完整组按至少 18 ms 首片接收间隔（名义 100 Hz 输入约发布 50 Hz，为接收抖动留余量）发布，沿用固定安装补偿、轮速历史配对及卡尔曼观测。原始加速度含重力，先按四元数旋转到导航系，再减去 `DM_IMU_GRAVITY_MPS2=9.80665f`；角速度转换为 rad/s。四元数按传感器→导航系解释，这一方向仍需实物倾斜静止检查，不能以编译通过代替确认。

schema 3/72 列和 `I64=2` 保持；`I5` bit 19（524288）为 DM 来源。`I34/I35` 序号与 `I67/I68` 时间由主板接收端生成/估计，`I66=0`，不是模块源时钟或零传感器延迟。断流超龄后清 IMU/融合有效位，四轮有效则回退轮速。接线、模块设置、首测步骤和时间限制见 [DM 集成说明](dm-imu-integration.md)。

旧第二板 CAN 采集协议与构建预设已删除，仅保留[历史验证记录](superpowers/verification/2026-11-12-remote-imu-fusion.md)。需要轮速基线时只关闭 `CHASSIS_REMOTE_IMU_FUSION_ENABLE`，保留 DM 接收与同样遥测；四轮正常时状态为 `I6=2`，bit 3/16/17/18 为 0。

## 固定底盘 DM 的融合参数与状态

### 固定底盘 IMU 参数

配置在 [chassis_remote_imu_config.h](../config/30_velocity_observation/chassis_remote_imu_config.h)，类型见 [远端融合模块](../modules/algorithm/chassis_remote_fusion.h)。修改后重新构建主板，保留配置文件、测量照片、误差说明和二进制哈希。

| 字段/标识 | 填法与边界 |
| --- | --- |
| `CHASSIS_REMOTE_IMU_SENSOR_TO_BODY` → `sensor_to_body_quaternion` | 原生传感器坐标到底盘前/左/上的安装旋转，Hamilton `[w,x,y,z]` 单位四元数；不是传感器到导航系的实时姿态 |
| `CHASSIS_REMOTE_IMU_OFFSET_BODY_M` → `imu_offset_body_m` | 底盘几何中心到 IMU 芯片的固定三维向量，底盘前/左/上，单位 m；靠近中心不等于零偏移 |
| `filter.wheel_variance`、`accel_variance`、`jerk_density`、`max_velocity_residual_mps` | 用辨识数据设定、独立验证段检查，再冻结；单位与约束见 [滤波头](../modules/algorithm/chassis_velocity_fusion.h) |
| `filter.max_age_us/max_skew_us/max_gap_us`、`max_pair_span_ms/max_wheel_skew_ms/max_timing_jitter_ms` | 分别约束滤波时间、轮速配对跨度、四轮反馈偏差和接收估计时间抖动；按实际配置记录，不通过放宽门槛隐藏传输异常 |
| `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` | 默认 `0`；安装、噪声和时间参数完成实测审核后改为 `1`，对应 bit 18。只是审核声明，不阻止算法运行，也不能代替实车验收 |

默认单位四元数、零偏移是假定芯片轴已对齐、芯片在底盘中心的**名义安装**，噪声也是试运行初值。实物不符合这些假定时，不能把默认值生成的 FUSED 曲线用作定量验收。远端路径没有云台关节参数，不加载 `chassis_velocity_fusion_calibration.h`。

当前配对跨度、四轮反馈偏差、时间抖动的初值分别为 15 ms、3 ms、10 ms，对应 `CHASSIS_REMOTE_IMU_MAX_PAIR_SPAN_MS/MAX_WHEEL_SKEW_MS/MAX_TIMING_JITTER_MS`。滤波输入在配对后使用同一个估计样本时刻，因此初始化器中的 `filter.max_skew_us=0` 不表示真实传感器零偏斜；`max_age_us/max_gap_us=60000` 也不取消中心算法每侧 15～30 ms 的约束。

### 时间和有效性

DM 协议没有源时钟或序号，适配器将首片接收时刻作为样本时间估计，并在发布完整组时生成序号，`source_age_ms=0`。`I33=主板当前时间−首片接收时间` 是已知年龄下界，要求不超过 60 ms；`I65` 为完整组接收年龄，`I69` 为组装跨度。该方法不包含模块内部采样/滤波与 CAN 排队延迟，bit 17 和 `timing_estimated` 提醒它不是硬件同步时间。

32 项轮速历史按估计采样时刻有界配对/插值，缺少合格历史时等待或降级，不把当前轮速冒充同刻轮速。中心差分和卡尔曼 dt 使用接收端估计时刻；输出保存三点中间样本时刻。`I67/I68` 属于主板时钟，但不能据此宣称知道模块真实采样时刻。

同一已发布序号最多推进一次，重复读取不重复校正或刷新输出时间。DM 按至少 18 ms 首片接收间隔（名义 100 Hz 输入约发布 50 Hz，为接收抖动留余量）发布，三点中心补偿要求每侧 15～30 ms；异常间隔、无效姿态、超龄和时间倒退会拒绝或重置。协议没有组序号，无法识别在线重传的一整组相同测量；静止时相同量化值也不意味着重传。

DM 不可用且四轮有效时退回当前轮速；四轮不可用时整体估计无效，不自动切回本地云台 IMU。模块断电后没有源重启字段，只能通过断流和时间间隔触发重置，因此实测必须保留停流/恢复证据。

### 融合状态与诊断

等待有效历史后，`I6=9` 且 `I5` bit 3 为 1 才说明本行真正使用 IMU 融合。`I6=4` 表示 IMU 不可用，`6` 未对齐，`7` 预热，`8` 输入/中心补偿/滤波拒绝，`10` 配置无效；这些状态不能按融合通过统计。`I6=2` 对应纯采集或未启用本地标定的轮速观测，并不是远端配置“未审核”的状态：bit 18 为 0 时算法也可以输出 FUSED。

调试器可观察 `chassis_fused_velocity`；程序应调用 `ChassisGetFusedVelocitySnapshot()` 读取一致快照并检查 `valid/status`。远端路径还发布 `remote` 诊断：

| 成员 | 含义 |
| --- | --- |
| `remote.tick_ms` | 融合输出对应的中间样本在主板时钟下的估计时刻；没有新样本时不刷新 |
| `remote.source_sequence/source_tick_ms/estimated_sample_ms` | 最近检查的接收端生成序号、样本估计时刻及主板配对时刻；不一定是当前中间输出对应的源包 |
| `remote.source_step_ms/receive_step_ms/pair_span_ms` | 样本估计时刻步长、首片接收步长、轮速配对跨度，用于检查漏组、抖动和历史配对 |
| `remote.updates/resets` | 饱和累计更新/重置计数；更新计数用于检查同一源包是否被重复处理，不等于 CSV 行数 |
| `remote.timing_estimated` | 采用接收端估计时间；不是同步成功标志 |

这些详细诊断不增加观测通道。需要无停核日志或额外采集才能逐次验证计数、重置和延迟；调试器 halt 的记录不能用于实时性验收。`I27/I28` 和 `I29/I30` 是同一融合输出的速度与配对轮速，`I31/I32` 是导航系中心加速度。主板时间减 `I11` 可还原该输出的主板估计时刻，但不能消除采样时间不确定度；`I33`–`I45` 仍是最新原始远端输入，与中间输出并不同刻。

### 采集工况与交付

主板与所选外部 IMU 上电后保持静止，等待姿态与远端有效位稳定；DM 先完成倾斜静止核对。每个工况保存一份 CSV：静止 30–60 s、低速前后起停/换向 30–60 s、低速左右起停/换向 30–60 s，以及两个方向旋转各 20–30 s。旋转按 [手册 T7](kalman-chassis-test-and-acceptance.md#6-t1t8按表执行实测) 先零输出手推，有动力时仅用已验证的受限配置；默认自旋档不能直接当作低速档。各动作前后保留约 5 s 静止，重复 3–5 次，保留起停与换向过程。

提交完整 `I0`–`I71`、原有表头和 VOFA+ 时间列，并附主板固件身份、完整构建命令、外部 IMU 安装轴向和芯片相对底盘中心的偏移；附模块设置截图和 bit 19、实际动作方向、地面与载荷，以及拒绝计数增长或抖动情况。靠近几何中心不等于偏移严格为零。

平移速度使用 mm/s，加速度使用 mm/s²，IMU 角速度使用 mrad/s。`I23`、`I26` 的车体旋转速度为 mrev/s（`1000×转/秒`），由 `wz_rpm×1000/60` 得到。模式 `2` 的 20 ms CSV 记录最新快照（旧模式 `1` 为 80 ms），不会提高 IMU 或融合器的内部更新率，也不能保证捕获每个输入样本、逐次重放高频滤波器或完整辨识 Q/R；实际速度误差和打滑表现还需独立速度参考。

本地 IMU 采样与 UART 诊断仍可独立使用，见 [VOFA 本地诊断](vofa-telemetry.md#本地-imu-uart-诊断)；它不替代 DM 融合或另建第二板 CAN 链路。

## 现有云台 IMU 融合路径：构建与读取

以下是可选的本地路径，适用于单 A 板、速度控制模式，MPU6500 随云台 yaw 和 pitch 转动；验证时关闭 `CHASSIS_IMU_CAN_RX_ENABLE` 和 `CHASSIS_REMOTE_IMU_FUSION_ENABLE`。观测器用四轮原始转子 RPM 反算底盘速度，结合时间对齐的 yaw/pitch 编码器与本地 INS 四元数恢复底盘姿态，再将 IMU 去重力加速度补偿到底盘中心，在导航系估计速度后转回底盘坐标系。结果用于记录与对照。

| 构建选项 | 效果 |
| --- | --- |
| 默认配置 | 关闭本观测 |
| `CHASSIS_VELOCITY_FUSION_ENABLE=1` | 发布原始四轮反算，以及四轮/INS/云台原始诊断；四轮有效时状态为 `CHASSIS_VELOCITY_UNCALIBRATED` |
| 再加 `CHASSIS_VELOCITY_FUSION_CALIBRATED=1` | 从构建目录读取 `chassis_velocity_fusion_calibration.h` 并运行融合 |

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-observation CHASSIS_IMU_CAN_RX_ENABLE=0 CHASSIS_REMOTE_IMU_FUSION_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=1
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-calibrated CHASSIS_IMU_CAN_RX_ENABLE=0 CHASSIS_REMOTE_IMU_FUSION_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=1
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

本地融合输出可用模式 `2` JustFloat 或模式 `1` FireWater 记录，均为 schema 2。只有 `I5` bit 3 为 1 时，`I27`/`I28` 才是融合结果，`I29`/`I30` 为对应时刻的配对轮速，估计年龄为 `I11`。最新轮速和本地 IMU 另有年龄，不能把整行当成同时采样。估计减配对轮速是后验差，不是校正前 innovation，不能直接用来拟合 R。

## 现有云台 IMU 融合路径的实车验证

以下针对本页已有的云台 IMU 融合路径；使用底盘 DM 模块时，不把该路径的杆臂参数直接移作中心 IMU 配置。

1. 校准实际轮径、轮向、两轴编码器零位和方向、转轴偏置及 IMU 安装姿态。[合成配置头](../tests/chassis_velocity_observer/stubs/chassis_velocity_fusion_calibration.h)仅供主机和固件构建验证。
2. 底盘保持静止，先记录云台静止基线，再分别转 yaw、pitch，最后做两轴组合动作。同步检查中心加速度和融合速度是否随云台运动产生偏差。
3. 采集低速直行、横移、起停、换向及自旋。按时间戳对比融合值和配对轮速反算，使用独立速度参考评价实际误差。CAN 和 SPI 时间戳记录的是 MCU 接收或读回时刻，电调和 IMU 内部的采样及滤波延迟需要实测评估。
4. 检查传感器失效时的降级、恢复时的稳定性，以及启用观测后的任务最大耗时。当前标定和实车精度尚未验证。
