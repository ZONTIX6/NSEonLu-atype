# 整车平移速度融合观测

当前推荐路线是：第二块 A 板刚性固定在底盘，经 **CAN1** 将 IMU 送到主板，与轮速历史配对后在线运行卡尔曼速度观测，再由主板 UART7 以 JustFloat 汇总到 **一个 VOFA+ 窗口**，采集后由 VOFA+ 导出一份 CSV。融合结果仅供记录与对照，没有接入电机控制、PID、功控、随动、自旋或安全门控。

远端路径不依赖本地云台 IMU 或 yaw/pitch 编码器；它使用固定安装旋转和芯片偏移。本页后半部分保留可选的本地云台融合路径，两者配置不能混用。软件接入不代表实车精度通过；操作顺序和签字条件见 [测试与验收手册](kalman-chassis-test-and-acceptance.md)。

## 推荐：CAN1 在线融合并汇总到一份 CSV

主底盘/云台板构建：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/kalman-can1-main ONE_BOARD=1 CHASSIS_CONTROL_MODE=0 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2
```

第二块底盘 IMU 板构建：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/imu-can1-sender IMU_CAPTURE_ONLY=1 IMU_CAPTURE_CAN_TX_ENABLE=1 VOFA_TELEMETRY_ENABLE=0 USB_TELEMETRY_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=0 CHASSIS_REMOTE_IMU_FUSION_ENABLE=0
```

第二板只运行 INS 采样、温控和 CAN1 发送，不初始化电机、底盘、云台或遥控。分别将两份固件烧入对应板。沿主板现有 CAN1 总线接入第二板，连接 `CAN_H` 对 `CAN_H`、`CAN_L` 对 `CAN_L` 和参考地；按板卡丝印及实际接线图确认物理引脚。终端电阻按总线两端配置，不因增加节点盲目添加第三只终端。

当前 CAN1 为 **1 Mbps**，新增标准 ID **`0x520`** 与当前工程已有 ID 无冲突。每 20 ms 尝试发送一组（50 Hz），每组 9 个 8 字节分片，承载源四元数 4 轴、导航系去重力加速度 3 轴、传感器系角速度 3 轴、样本序号、源板时间、发送时源年龄和有效标志。接收检查严格顺序、同组 tag、CRC16/CCITT-FALSE 和 10 ms 组装期限。发送失败即中止本组，缺片、错序或过期组不更新已发布样本。

仅主板 UART7 接 VOFA+，选择 **JustFloat、115200、8N1**。模式 `2` 使用 schema 3 的固定 72 路通道，发送间隔至少 40 ms（标称 25 Hz）；采集后由 VOFA+ 导出完整 **`I0`–`I71`**。启动先静止，确认 `I0=3`、`I63=0`、`I64=2`、`I5` bit 15/16/17 为 1，再等待 bit 4 持续有效、`I65` 接收年龄保持较小，以及预热后 `I6=9` 且 bit 3 为 1。来源和配置位本身不表示收到有效输入或已进入融合。[完整字段与采集步骤](vofa-telemetry.md#firewater-卡尔曼观测通道表)

schema 3 保留 schema 2 前 64 列的布局，主要区别如下：

| 字段 | CAN1 路线含义 |
| --- | --- |
| `I0` | schema 版本 `3` |
| `I5` bit 4 / bit 15 | 远端 IMU 当前有效 / 已选择远端 CAN IMU 模式 |
| `I5` bit 16 / bit 17 / bit 18 | 已配置远端融合 / 使用接收端估计时间 / 配置声明已实测审核；均不是有效位或精度结论 |
| `I27`–`I32` | 估计速度、配对轮速、导航系中心加速度；bit 3 有效时才是融合结果及其配对量 |
| `I33`–`I45` | 第二板的样本年龄下界、源序号、四元数、去重力加速度和角速度；远端失效时标无效，不退回本地云台 IMU |
| `I63` / `I64` | 记录板为主板 `0` / IMU 来源为 CAN1 `2` |
| `I65` / `I66` | 主板完整组接收年龄 / 第二板发送时源样本年龄，ms |
| `I67`、`I68` | 源板样本毫秒时间的高、低 16 位，仅诊断 |
| `I69` | 本组首末片主板接收间隔，ms |
| `I70`、`I71` | 累计拒绝组数的高、低 16 位，达到 `UINT32_MAX` 后饱和 |

`CHASSIS_REMOTE_IMU_FUSION_ENABLE` 普通构建默认 `0`，两板脚本的板 1 预设为 `1`、板 2 为 `0`。开启需要单板整车速度后端、CAN IMU 接收和整车观测同时开启；`CHASSIS_VELOCITY_FUSION_CALIBRATED` 保持 `0`，这个旧宏只选择本地云台标定路径。

需要纯采集基线时，使用独立目录并关闭新开关：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/kalman-can1-capture ONE_BOARD=1 CHASSIS_CONTROL_MODE=0 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=0 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2
```

此时四轮有效则 `I6=2`（`UNCALIBRATED`），`I27/I28` 来自轮速，bit 3/16/17/18 为 0，`I29`–`I32` 为 0，原始 CAN 采集保留。切换配置后重新核对已烧录固件身份，不混合保存两种配置的 CSV。

### 固定底盘 IMU 参数

配置在 [chassis_remote_imu_config.h](../config/30_velocity_observation/chassis_remote_imu_config.h)，类型见 [远端融合模块](../modules/algorithm/chassis_remote_fusion.h)。修改后重新构建主板，保留配置文件、测量照片、误差说明和二进制哈希。

| 字段/标识 | 填法与边界 |
| --- | --- |
| `CHASSIS_REMOTE_IMU_SENSOR_TO_BODY` → `sensor_to_body_quaternion` | 原生传感器坐标到底盘前/左/上的安装旋转，Hamilton `[w,x,y,z]` 单位四元数；不是传感器到导航系的实时姿态 |
| `CHASSIS_REMOTE_IMU_OFFSET_BODY_M` → `imu_offset_body_m` | 底盘几何中心到 IMU 芯片的固定三维向量，底盘前/左/上，单位 m；靠近中心不等于零偏移 |
| `filter.wheel_variance`、`accel_variance`、`jerk_density`、`max_velocity_residual_mps` | 用辨识数据设定、独立验证段检查，再冻结；单位与约束见 [滤波头](../modules/algorithm/chassis_velocity_fusion.h) |
| `filter.max_age_us/max_skew_us/max_gap_us`、`max_pair_span_ms/max_wheel_skew_ms/max_timing_jitter_ms` | 分别约束滤波时间、轮速配对跨度、四轮反馈偏差和跨板估计时间抖动；按实际配置记录，不通过放宽门槛隐藏传输异常 |
| `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` | 默认 `0`；安装、噪声和时间参数完成实测审核后改为 `1`，对应 bit 18。只是审核声明，不阻止算法运行，也不能代替实车验收 |

默认单位四元数、零偏移是假定芯片轴已对齐、芯片在底盘中心的**名义安装**，噪声也是试运行初值。实物不符合这些假定时，不能把默认值生成的 FUSED 曲线用作定量验收。远端路径没有云台关节参数，不加载 `chassis_velocity_fusion_calibration.h`。

当前配对跨度、四轮反馈偏差、时间抖动的初值分别为 15 ms、3 ms、10 ms，对应 `CHASSIS_REMOTE_IMU_MAX_PAIR_SPAN_MS/MAX_WHEEL_SKEW_MS/MAX_TIMING_JITTER_MS`。滤波输入在配对后使用同一个源时刻，因此初始化器中的 `filter.max_skew_us=0` 不表示真实传感器零偏斜；`max_age_us/max_gap_us=60000` 也不取消中心算法每侧 15～30 ms 的约束。

### 时间和有效性

源有效且完整组校验通过后，远端可用还要求发送时源年龄 `I66≤20 ms`、已知年龄下界 `I33≤60 ms`。`I33 = 发送时源年龄 + (主板当前时间 − 本组首片接收时间)`；它没有包含首片排队、总线仲裁前等待或传感器内部延迟，因此不是精确端到端年龄，也不是硬件同步后的采样时刻。`I65` 从完整组接收时刻算起，`I69` 表示分片到达跨度。无效测量填 0，未知或不可表示的年龄填 `-1`，使用数据必须检查 bit 4。

JustFloat 记录中的整数语义字段先四舍五入为整数，再做位运算；高低位按 `高位×65536+低位` 还原。float32 不保证所有 int32 大整数精确，分开的 16 位时间/计数仍可还原，详见[完整协议的精度说明](vofa-telemetry.md#firewater-卡尔曼观测通道表)。时钟、发送序号和源 INS 序号按 uint32 回绕；累计拒绝组数达到 `UINT32_MAX` 后饱和，不回绕。两板独立计时，不能用主板 `I1/I2` 直接减源板 `I67/I68`，也不能把主板时间减 `I33` 当成精确的跨板采样时刻。单份 CSV 只是汇总不同来源的最新观测；离线比较仍须考虑各自年龄、传感器延迟及第二板导航系方向。

远端适配器把 `first_rx_tick_ms - source_age_ms` 作为主板时钟下的**估计采样时刻**，在 32 项固定轮速历史中作有界配对/插值；缺少合格历史时等待或降级，不把当前轮速冒充同刻轮速。中心三点差分与卡尔曼 dt 使用源板时间；输出另保存中间样本在主板时钟下的估计时刻。bit 17 提醒这个时间方法没有测得单向传输排队与传感器内部延迟，不能当作同步证据。

每个新源序号最多推进一次，重复样本不会重复校正或刷新输出时间。三点中心补偿要求每侧 15～30 ms，正常 CAN 组间隔约 20 ms；漏一组导致约 40 ms 间隔时重置并重新预热。源重启、序号/时刻倒退、异常时间抖动、无效姿态或超龄也会拒绝或重置。远端不可用且四轮有效时退回当前轮速；四轮不可用时整体估计无效。不会自动切换到本地云台 IMU。

### 融合状态与诊断

等待有效历史后，`I6=9` 且 `I5` bit 3 为 1 才说明本行真正使用 IMU 融合。`I6=4` 表示 IMU 不可用，`6` 未对齐，`7` 预热，`8` 输入/中心补偿/滤波拒绝，`10` 配置无效；这些状态不能按融合通过统计。`I6=2` 对应纯采集或未启用本地标定的轮速观测，并不是远端配置“未审核”的状态：bit 18 为 0 时算法也可以输出 FUSED。

调试器可观察 `chassis_fused_velocity`；程序应调用 `ChassisGetFusedVelocitySnapshot()` 读取一致快照并检查 `valid/status`。远端路径还发布 `remote` 诊断：

| 成员 | 含义 |
| --- | --- |
| `remote.tick_ms` | 融合输出对应的中间样本在主板时钟下的估计时刻；没有新样本时不刷新 |
| `remote.source_sequence/source_tick_ms/estimated_sample_ms` | 最近检查的源身份、源时刻及主板估计采样时刻；不一定是当前中间输出对应的源包 |
| `remote.source_step_ms/receive_step_ms/pair_span_ms` | 源时刻步长、首片接收步长、轮速配对跨度，用于检查漏组、抖动和历史配对 |
| `remote.updates/resets` | 饱和累计更新/重置计数；更新计数用于检查同一源包是否被重复处理，不等于 CSV 行数 |
| `remote.timing_estimated` | 采用接收端估计时间；不是同步成功标志 |

这些详细诊断不增加观测通道。需要无停核日志或额外采集才能逐次验证计数、重置和延迟；调试器 halt 的记录不能用于实时性验收。`I27/I28` 和 `I29/I30` 是同一融合输出的速度与配对轮速，`I31/I32` 是导航系中心加速度。主板时间减 `I11` 可还原该输出的主板估计时刻，但不能消除跨板时间不确定度；`I33`–`I45` 仍是最新原始远端输入，与中间输出并不同刻。

### 采集工况与交付

两板上电后保持静止，等待启动标定与远端有效位稳定。每个工况保存一份 CSV：静止 30–60 s、低速前后起停/换向 30–60 s、低速左右起停/换向 30–60 s，以及两个方向旋转各 20–30 s。旋转按 [手册 T7](kalman-chassis-test-and-acceptance.md#6-t1t8按表执行双板实测) 先零输出手推，有动力时仅用已验证的受限配置；默认自旋档不能直接当作低速档。各动作前后保留约 5 s 静止，重复 3–5 次，保留起停与换向过程。

提交完整 `I0`–`I71`、原有表头和 VOFA+ 时间列，并附两板的固件身份、完整构建命令、第二板安装轴向、MPU6500 芯片相对底盘中心的偏移、实际动作方向、地面与载荷，以及拒绝计数增长或抖动情况。靠近几何中心不等于偏移严格为零。

平移速度使用 mm/s，加速度使用 mm/s²，IMU 角速度使用 mrad/s。`I23`、`I26` 的车体旋转速度为 mrev/s（`1000×转/秒`），由 `wz_rpm×1000/60` 得到。模式 `2` 的 40 ms CSV 仍是降采样（旧模式 `1` 为 80 ms），不能逐次重放高频滤波器或完整辨识 Q/R；实际速度误差和打滑表现还需独立速度参考。

## 可选：独立双 CSV 采集

不接 CAN1 时仍可分别记录两板 UART7。关闭主板 `CHASSIS_IMU_CAN_RX_ENABLE`、`CHASSIS_REMOTE_IMU_FUSION_ENABLE` 和第二板 `IMU_CAPTURE_CAN_TX_ENABLE`，第二板保持 `IMU_CAPTURE_ONLY=1` 并开启遥测输出。文末示例保留模式 `1` FireWater，也可把两端同时改为模式 `2` JustFloat。该路线使用 schema 2 的 64 列，每个工况保存主板与第二板各一份 CSV，需要两个独立接收通道，不并接两个 TX。[两份完整构建命令与配对步骤](vofa-telemetry.md#可选独立双-csv-采集)

schema 2 主板 `I63=0`，IMU 列来自本地云台；独立第二板 `I63=1`、`I6=-2`、`I5` bit 14 为 1，IMU 列来自底盘。第二板仅有 bit 0 和 bit 4 对应的测量有效，保留时间、源序号及 IMU 数据，其余控制或观测测量不可用。保存各自完整 `I0`–`I63`，再离线处理时间偏移、时钟漂移和坐标方向。不能直接按 CSV 行号或两板开机毫秒配对。

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

以下针对本页已有的云台 IMU 融合路径；使用第二块底盘中心采集板时，不把该路径的杆臂参数直接移作中心 IMU 配置。

1. 校准实际轮径、轮向、两轴编码器零位和方向、转轴偏置及 IMU 安装姿态。[合成配置头](../tests/chassis_velocity_observer/stubs/chassis_velocity_fusion_calibration.h)仅供主机和固件构建验证。
2. 底盘保持静止，先记录云台静止基线，再分别转 yaw、pitch，最后做两轴组合动作。同步检查中心加速度和融合速度是否随云台运动产生偏差。
3. 采集低速直行、横移、起停、换向及自旋。按时间戳对比融合值和配对轮速反算，使用独立速度参考评价实际误差。CAN 和 SPI 时间戳记录的是 MCU 接收或读回时刻，电调和 IMU 内部的采样及滤波延迟需要实测评估。
4. 检查传感器失效时的降级、恢复时的稳定性，以及启用观测后的任务最大耗时。当前标定和实车精度尚未验证。
