# 底盘速度观测：采集、回放与板上实验

本批范围是 S0＋S1：独立原始反馈、CSC1 短窗、两状态轮速 KF 和只读影子对照。PID、原功率保护与 CAN 命令仍走原链路。三个新开关默认均为 0；没有默认的实车 q/R，没有预测接管。

对应[实施计划](superpowers/plans/2026-10-29-chassis-speed-control-improvement.md)与[验证记录](superpowers/verification/2026-10-29-chassis-speed-observation.md)。软件检查与实车验收是两个独立状态；当前 `hardware_gate=not_run`。以下板上步骤是操作规程，需要已安排的设备、场地及明确的执行请求。

## 1. 构建配置

| 宏 | 默认 | 约束 |
|---|---:|---|
| `CHASSIS_SPEED_CAPTURE_ENABLE` | 0 | 速度模式、力控标定关闭、`LOG_OUTPUT=2` 或 `3` |
| `CHASSIS_SPEED_KF_SHADOW_ENABLE` | 0 | 速度模式、力控标定关闭、`CHASSIS_PREDICTIVE_POWER_MODE=1` |
| `CHASSIS_SPEED_KF_EXPERIMENT_CONFIG` | 0 | KF 开启，构建目录有已生成的参数头 |

三个宏只接受 0/1。采集可以单独开启，不要求动态模型或 KF。实验头不存在时编译失败，不回退到猜测参数。只更改编译开关不会安装动态转矩模型，也不会改变 `osDelay(2)` / `osDelay(5)`。

在仓库根目录 PowerShell 执行，`$speedPython` 可替换为能运行标准库脚本的 Python：

```powershell
$speedPython = 'C:\Users\20802\AppData\Roaming\uv\python\cpython-3.14-windows-x86_64-none\python.exe'
make PROFILE=atype-vehicle BUILD_DIR=build/speed-base CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 -j8
make PROFILE=atype-vehicle BUILD_DIR=build/speed-capture CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 CHASSIS_SPEED_CAPTURE_ENABLE=1 LOG_OUTPUT=2 -j8
make PROFILE=atype-vehicle BUILD_DIR=build/speed-shadow CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 CHASSIS_PREDICTIVE_POWER_MODE=1 -j8
```

KF 参数 JSON 使用 `units="rotor_rad_s"`，包含 `version/max_gap_us/max_age_us/max_horizon_us/q/r/initial_p00/initial_p11/gate_squared/max_rejections/synthetic`。先训练、验证，再冻结参数与验收门槛；真实参数的 `synthetic=false` 必须有实验依据。生成头和固件放到独立目录：

```powershell
& $speedPython scripts/chassis-speed-capture.py emit-kf-header --kf-config kf-config.json --output build/speed-experiment/chassis_speed_kf_experiment.h
make PROFILE=atype-vehicle BUILD_DIR=build/speed-experiment CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 CHASSIS_PREDICTIVE_POWER_MODE=1 CHASSIS_SPEED_CAPTURE_ENABLE=1 CHASSIS_SPEED_KF_SHADOW_ENABLE=1 CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1 LOG_OUTPUT=2 -j8
```

`tests/speed_observation/synthetic-kf-config.json` 仅供主机与编译验证，不是实车参数，生成头保留合成标记。参数 CRC16 按固定 40 字节小端编码计算，不对带 padding 的结构体算 CRC。初始化安装后，在快周期切换完成时回读 `ChassisPowerShadowGetWheelKFCheckpoint()` 返回的参数；metadata 的版本和 CRC 是捕获时的安装值。`chassis_speed_observation_diagnostics.config_result`：1 成功、2 采集绑定失败、3 实验参数校验或安装失败。

## 2. 捕获与导出

每窗固定 192 条原始记录、32 条周期记录；任一流满即同时冻结。每次 CAN 接收独立保留，同毫秒的多帧不合并。在线 KF 每个控制周期只消费一致快照中的最新事件，跳过的中间事件单独计数。短窗通常只能描述局部行为，实际时长以 end 帧为准。

状态顺序：`IDLE → ARMED → RECORDING → FROZEN → DRAINING → IDLE`。录制时只复制固定字节，不做 UART 等待；CRC 和发送在低优先级任务。冻结后必须确认禁驱才开始导出，恢复驱动后暂停新增排队。最后一帧被 UART 接受后还需等待 UART idle 才能重新 arm。

采集邮箱只由电机任务在周期边界消费。先写参数，最后写新的 sequence；等 acknowledged_sequence 与 sequence 相等，再读 result（1 接受、2 拒绝）：

```text
set var chassis_speed_observation_request.capture_id = 1
set var chassis_speed_observation_request.action = 1
set var chassis_speed_observation_request.sequence = 1
print chassis_speed_observation_request.acknowledged_sequence
print chassis_speed_observation_request.result

set var chassis_speed_observation_request.action = 2
set var chassis_speed_observation_request.sequence = 2
print chassis_speed_observation_request.acknowledged_sequence
print chassis_speed_observation_request.result
```

action 1 为 arm，2 为手动冻结。未初始化时钟、前窗尚未导完、配置发布尚未在快周期切换等情况会拒绝 arm。每窗使用新 capture_id。arm 保存计算前的 KF checkpoint，故可以从热机状态回放；显式 arm 也恢复因超时暂停的影子任务。

日志口为 USART2，115200、8N1。按二进制保存串口流到 `capture.bin`，不要用会修改字节或行尾的文本终端保存。原始记录 60 B/帧、周期 532 B/帧、metadata 1044 B、end 68 B，传输不支持连续全速采样。停止后的导出可能持续数秒。

```powershell
& $speedPython scripts/chassis-speed-capture.py decode --input capture.bin --output capture.jsonl
```

解码成功且完整返回 0；缺帧、CRC 错误、混合编号、截断等返回 2，同时保留能解出的记录和 `complete=false`。普通参数或文件错误返回 1。检查 summary、end、计数和冻结原因后再使用该窗。正常容量冻结不算丢失；窗内 rejected_records、窗外 outside_window 和传输重试含义不同，不能互相替代。开始 drain 后导出内容保持不变，后续重试只累计会话诊断。

## 3. 时间、轮序和协议含义

轮序为 LF/RF/LB/RB；方向字段只描述几何，不改变原始电机转速的符号。原始单位是 rpm 和 DJI 反馈等效电流，KF 为转子 rad/s、rad/s²，时间为 μs，功率为 W。接收时间不是采样时间；schema 1 的 sample_source 必须为 0，采样时间显示未知。

CSC1 帧头为 `magic[4]/schema:u16/kind:u8/flags:u8/payload_len:u16/capture_id:u32/record_index:u32`，随后 payload 和小端 CRC16。schema=1，kind 1/2/3/4 分别为 metadata/raw/cycle/end；payload 为 1024/40/512/48 B。CRC 覆盖头和 payload，反射多项式 0xA001、初值 0xFFFF；`123456789 → 0x4B37`。

精确字段表见计划附录。实施中保留了一个必要的时间字段修订：周期 diagnostics 的相对 offset 64（周期 payload offset 464）是 **`observation_cycles`**，不是初版计划中的 `period_max_us`；周期 offset 0 的 `tick_ms` 是该观测时刻配对的粗时钟。MotorBegin 的起点仍在 `start_cycles`，完整周期耗时为 `execution_us`，相邻起点间隔为 `period_us`。`observation_now_us` 的整数微秒不能独自还原 DWT 小数相位，所以回放需要这对原始计数。运行时累计最大周期保留在 `chassis_speed_observation_diagnostics.period_max_us`，报告另算窗内最大值。不要混用旧实现中 MotorBegin 的 tick 与 AdapterLimit 的 cycles；旧数据跨 tick 的周期不能作为精确一致性证据。

metadata 保存时钟 epoch/fraction、KF 参数/checkpoint、运行 PID 与来源、模型 checksum。未注入构建或工作区身份时相应有效位为 0，不能把当前电脑 HEAD 当作已烧录版本。manifest 的源码身份和板上 metadata 身份须分别核对。

CAN 提交记录是实际尝试入队的字节与每组成功位，不代表电调执行确认。跨组停止不能撤销前一组已入队的命令，报告保留部分提交。`raw_feedback_current` 为测量电流，PID 候选另存，二者不能混用。

不可用耗时、资源和反馈年龄用 UINT32_MAX 编码，解码为缺测。无效时间不会进入正常耗时分布。资源每 200 ms 采样；栈高水位单位为 FreeRTOS words，堆单位为 bytes。快层连续三次超过 300 μs 暂停，慢层超过 2 ms 暂停；只在重新 arm 或发布新 KF 配置后恢复。这是结束后的超时检查，尚不是求解内部的硬截止。

## 4. 实验清单与回放

```powershell
& $speedPython scripts/chassis-speed-capture.py manifest --metadata experiment-metadata.json --output manifest.json
& $speedPython scripts/wheel-speed-replay.py --capture capture.jsonl --manifest manifest.json --kf-config kf-config.json --output report.json --build-dir build/speed-replay-experiment
```

回放用主机 GCC 编译生产 C KF，并校验 ctypes 大小与字段偏移；不复制一套 Python KF。不同进程使用不同 build-dir，以免覆盖已加载的 DLL。manifest 篡改、非法单位、错误捕获哈希、没有有效轮速等情况非零退出。

metadata 须填写实际固件/二进制身份、构建选项与工具链、PID 和 Flash 实际导出及 `source`、模型与 checksum、传感器接线/板版本、单位/几何、轨迹编号、初末能量/温度/载荷、时间对齐与采样延迟、参数、分段和冻结门槛/时间。无法测得的字段使用 `{"value":null,"status":"unmeasured"}`；空对象或仅声明已测量不能代替来源。`artifacts` 可将名称映射到文件路径，生成器记录各文件 SHA256；捕获 JSONL 使用名称 `capture`，或显式填写 `capture_sha256`。

训练、验证、测试使用互不重叠的半开区间，例如：

```json
{
  "synthetic": false,
  "splits": [
    {"capture_id": 1, "split": "train", "start_us": 100000, "end_us": 150000, "condition": "steady"},
    {"capture_id": 2, "split": "validation", "start_us": 200000, "end_us": 250000, "condition": "ramp"},
    {"capture_id": 3, "split": "test", "start_us": 300000, "end_us": 350000, "condition": "ramp"}
  ],
  "steady_segments": [{"capture_id": 1, "start_us": 110000, "end_us": 150000}],
  "frozen_at": "实验实际冻结时间（含时区）"
}
```

这只是分段格式片段，不是完整实测清单。时间必须与捕获 `observation_now_us` 同源；不拼接多个短窗。低通 `comparison_lpf_tau_s` 与评分 `score_tolerance_us` 写入 KF JSON 并与 q/R 一同冻结；未提供 tau 时不猜测低通参数。首次可信样本启动低通，前 5τ 热机段排除稳态分数。

报告分开列出 raw hold、捕获的旧滤波/guard、实际 dt 的低通差分外推和 KF 的 2/8/20 ms 预测。`online_latest` 从周期实际引用事件重现，`all_events` 另用全部原始事件，其收益不能归为在线滤波收益。原始未来观测误差与合成真值误差分列；创新拒绝样本不能从评分真值中偷偷删掉。

功率评价需要输入与负载边界、时间对齐、模型证据分别确认；缺证据时 `available=false`，不补 0 W。当前比较要求已核实 `timing_alignment.sampling_delay_us=0`；未知或非零延迟会明确拒绝评分，须先取得实际对齐的测量数据。完整轨迹的 IAE、响应、过冲/恢复、电流变化等只在数据覆盖时给出；短窗显示 `not_observable_in_window`。任何报告都不自动放行硬保护或控制接管。

## 5. 板上验收顺序与退出

1. 记录修复后的共同基线、Flash/PID 导出、固件和补丁哈希，先核实四轮对应关系与传感器接线。
2. 顺序执行静止、架空低速、稳定轮速、受限阶跃/斜坡、停车/换向，再地面直行/横移/自旋/组合。每窗 arm 前核对 KF version/checksum，冻结后停机导出；不使用长时硬堵转。
3. 训练/验证段选择 q/R/τ 与创新门限，按基线噪声和仪器分辨率冻结主指标、允许退化量及栈/堆余量。测试段不再调参。每个主工况至少 10 组配对试次，交替顺序并匹配初末储能、载荷和温度。
4. 单独测试日志拥塞、时钟长运行/回绕、部分离线、组提交失败、许可切换、最坏求解和任务干扰。保留所有失败、超时、丢样本、暂停/恢复及最差试次。无停核读写优先；必须 halt 的试次标记干扰，不能用来验收实时预算。
5. 核对完整控制周期、临界区耗时、300 μs/2 ms 预算、总新增 RAM、实际栈高水位与堆余量。短窗不覆盖长响应时，补独立连续测量后再评响应延迟。

参数无证据、任一预算不满足或控制周期越界时保持关闭；不能因为平均耗时较小放行。退出时使用全关闭固件配置，保留实验文件与 manifest。S1 结论只可为“软件完成，实车未验收”“S1 不放行或无可测收益”或“独立实测验收通过，可规划 S2”。本批不自动启用 Ki、前馈、动态辨识、预测分配或 F334 新协议。
