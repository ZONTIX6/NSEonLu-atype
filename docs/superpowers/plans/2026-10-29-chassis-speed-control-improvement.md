# 底盘速度控制观测与验证（S0＋S1）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 交付独立速度原始采集、可核对的最终 CAN 提交记录、时序与存储测量、纯 C 两状态轮速 KF、影子对照及可复现报告，为是否进入 S2 提供证据。

**Architecture:** 沿现有 DJI 速度链增加只读观测支路，保留原 PID、功率限制和发送主链。原始反馈在解码边界保存，KF 使用快周期一致快照中的最新原始事件；完整反馈事件另存入有界短窗，停机后导出。KF 的配置、有效性和时间水位独立于动态功率模型，离线工具复用生产 C 数值代码。

**Tech Stack:** STM32F427、C11、FreeRTOS/CMSIS-RTOS、DWT、现有 CAN/USART2 日志传输；GCC 主机测试、ARM GNU、Make/CMake、Python 标准库。

## Global Constraints

- 设计依据：[底盘速度控制优化与改进方案](../specs/2026-10-29-chassis-speed-control-improvement-design.md)，尤其第 4、5、9、10、11 节。
- “第一批控制范围为 S0 + S1，不加载猜测的动态转矩参数，不替换 PID”。
- “对象：NSEonLu - atype，`CHASSIS_CONTROL_MODE=0` 速度路径”。
- “排除：力控及其标定工具、云台和发射功能改造；F334 固件修改须核对板上版本后独立实施”。
- “CAN 未携带采样时间时明确标记‘未知’；接收时间不改名为采样时间。”
- “采样和传输解耦，满缓冲显式计数丢记录，不能阻塞电机任务。”
- “首版仅供影子功率预测和数据比较。现有 PID 和硬保护继续使用原路径；观测失效不能刷新原始数据新鲜度。”
- “首批不预先指定 `q/R` 数值，也不承诺百分比提升。”
- “原设计的 300 μs、慢层 2 ms 和 32 KiB 是待验收预算，不是已经获得的硬件保证。”
- “每项有修改前基线，主工况建议至少 10 组配对实验，交替顺序，并匹配初末储能、载荷和温度。”
- 不开启预测接管模式 `2`；不改变 `osDelay(2)` / `osDelay(5)` 调度策略；不改变力控条件分支行为。
- 项目约定：验证通过后主动创建本地提交，每项只提交本任务文件。禁止 `git add .`；保留已有未提交改动。

---

## 1. 范围、基线和已核对事实

本计划编写基线为 `f7f4398`；已有速度影子交付为 `78aefdf`，发送需求门控修复为 `e68b855`。当前工作区另有用户对 `.Doc/指针查找索引.md`、`application/board/board_chassis_force.c`、`application/chassis/chassis.c`、`docs/configuration-index.md`、`docs/structure-index.md`、`modules/algorithm/chassis_spin_config.h` 的修改。实施时重新记录状态，不将这些修改恢复或顺带提交；本计划也不要求修改这些文件。

以下是任务拆分依据，不应当作待重新实现的功能：

| 现状 | 对实施的约束 |
|---|---|
| `DJI_Motor_Measure_s` 的 `raw_rpm/raw_current/rx_cycles` 仅在力控标定宏下存在 | 扩展字段保存的条件，保留力控 observer 回调原条件 |
| `ReadMotorCycle()` 已在短临界区复制完整测量 | 复用一致快照；不要分别读取共享轮速、序号和时间 |
| `ChassisPowerCycleInput_s.raw_current` 是 PID 候选 | 新原始反馈电流命名为 `raw_feedback_current`，不复用这个字段 |
| 速度 `AdapterRecordFinal()` 折叠了发送组结果 | 新记录保留各组实际入队字节和提交结果 |
| 速度组没有注册 `validate_current`，驱动已支持 `CANTransmitGuarded()` | Task 3 先复现等待期间的许可窗口，若存在则用已有入口收口 |
| CPC1 是 96 B 记录、102 B 帧，已有旧解码器 | 新建独立 `CSC1` 格式，原 CPC1 不变 |
| 日志口实际为 USART2、115200、8N1 | 不承诺连续全速导出；首批采用 RAM 短窗、冻结后停机导出 |
| `motor_dt` 是执行耗时 | 新增起点间隔，分别命名 `period_us` 和 `execution_us` |
| 旧 `PPObserverUpdate()` 依赖非零模型版本，且位于有效动态模型分支 | KF 独立运行，不把它塞进该分支 |
| transient 使用 `abs(rpm)` 的差分和正加速度保护 | 记录其真实 `guard_rpm`；不能当有符号加速度真值 |
| 快层/慢层目前结束后才检查 300 μs / 2 ms | 本批加测量、连续超时暂停；求解内截止的升级门槛见 Task 6 |

S2a 的前馈/积分跟踪、S2b 的动态辨识及变化命令历史、S3 分配接管、S4 双储能/F334 协议均不在本批实施清单内。Task 3 如复现竞态，允许单独修正发送前许可检查；正常许可下的电流仍须与修复后的共同基线一致。

## 2. 文件责任和依赖

| 文件（均相对仓库根目录） | 操作与责任 |
|---|---|
| `modules/predictive_power/speed_observation_config.h` | 新增默认关闭的速度采集/KF 配置及非法组合检查 |
| `modules/predictive_power/speed_observation_time.{h,c}` | 新增纯 C DWT 时间展开、粗时间交叉检查 |
| `modules/predictive_power/wheel_speed_kf.{h,c}` | 新增单轮两状态 KF，无 HAL/RTOS、无堆分配 |
| `modules/motor/DJImotor/dji_motor.{h,c}` | 原始字段保存、反馈捕获入口、逐组提交快照 |
| `modules/motor/DJImotor/dji_power_adapter.{h,c}` | 装配速度观测、最终记录及采集绑定；按故障注入证据补发送前复核 |
| `modules/chassis_power/chassis_power_types.h` | 追加明确命名的观测输入和分组最终结果 |
| `modules/chassis_power/chassis_power_shadow.{h,c}` | 独立 KF 配置发布、四实例、多步诊断、超时暂停 |
| `modules/chassis_power/chassis_power_shadow_energy.c` | 慢层耗时/暂停状态，不改变策略输出 |
| `modules/telemetry/chassis_speed_capture.{h,c}` | 新增固定存储、状态机及 CSC1 编码；不依赖 HAL |
| `application/runtime/chassis_speed_observation.{h,c}` | 新增任务时间、快照发布、低优先级 drain/资源采样 |
| `application/runtime/robot_task.c` | 任务入口/出口测量与低优先级观测任务 |
| `Makefile`、`CMakeLists.txt`、`config/atype-options.txt` | 开关、源文件和禁用 fast-math 规则 |
| `scripts/chassis-speed-capture.py` | 新增二进制解码、manifest 校验、结构化导出 |
| `scripts/wheel-speed-replay.py` | 新增轮速对照报告，调用生产 C KF |
| `scripts/predictive-power-replay.py` | 原接口兼容；仅提取公共加载逻辑确有必要时修改 |
| `tests/speed_observation/*` | 新增时间、捕获、协议、manifest 和构建契约测试 |
| `tests/predictive_power/test_wheel_speed_kf.c`、`test_speed_replay.py` | 新增真值/异常/报告测试 |
| `tests/predictive_power/Makefile`、`test_shadow.c`、`verify_speed.py` | 接入并验证各开关隔离 |
| `tests/dji_chassis_power_rtt/test_dji_chassis_power_rtt.c` 及其 `Makefile` | 扩展真实驱动故障注入和字节验证 |
| `docs/chassis-speed-observation.md` | 新增操作、字段、实验及退出说明 |
| `docs/superpowers/verification/2026-10-29-chassis-speed-observation.md` | 实施后记录证据；硬件未测时明确未验收 |

依赖顺序：Task 1 → Task 2 → Task 3 → Task 4；Task 5 可与 Task 2/3 并行；Task 4＋5 → Task 6 → Task 7 → Task 8。并行实现者不得同时修改 `dji_motor.c` 或 `chassis_power_shadow.c`；相邻接口以本计划为准。

## Task 1：独立速度开关、原始反馈和可靠接收时间

**Files:** 新增 `speed_observation_config.h`、`speed_observation_time.{h,c}`、`tests/speed_observation/Makefile`、`test_time.c`、`test_build_contracts.py`；修改 `dji_motor.{h,c}`、`config/atype-options.txt` 和 DJI 驱动测试。

**Interfaces:**

```c
/* speed_observation_time.h: <stdbool.h>, <stdint.h> only */
typedef struct {
    uint64_t now_us;
    uint32_t cycles, tick_ms, hz, source_epoch, fraction;
    bool initialized;
} PPObservationClock;
bool PPObservationClockUpdate(PPObservationClock *clock, uint32_t cycles,
                              uint32_t tick_ms, uint32_t hz);
bool PPObservationReceiveTime(const PPObservationClock *clock,
    uint32_t rx_cycles, uint32_t rx_tick_ms, uint32_t max_age_us,
    uint64_t *rx_us);
```

- [ ] **1. 写开关契约测试并运行红灯。** 新宏 `CHASSIS_SPEED_CAPTURE_ENABLE=0`、`CHASSIS_SPEED_KF_SHADOW_ENABLE=0`、`CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=0`，均只接受 0/1。KF 要求 `CHASSIS_PREDICTIVE_POWER_MODE=1`；实验参数宏要求 KF 开启；采集可以单独启用，不依赖动态模型或 KF。采集或 KF 开启要求速度模式且 `CHASSIS_FORCE_CALIBRATION_ENABLE=0`；采集要求可用 UART 日志配置。测试用 GCC 预处理实际配置头，不只搜索字符串。执行 `python tests/speed_observation/test_build_contracts.py`，预期新头尚不存在而失败。

- [ ] **2. 加入开关和字段保存条件。** `config/atype-options.txt` 添加上述三个宏；原始数据字段与解码保存用以下条件，力控回调原条件不变。`rx_sequence` 仍代表每个接收事件；测试其现有回绕约定，不因本功能改变其他控制代码对序号 0 的处理。

```c
#if CHASSIS_FORCE_CALIBRATION_ENABLE || CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
    int16_t raw_rpm, raw_current;
    uint32_t rx_cycles;
#endif
/* DecodeDJIMotor 中先解码原始整数，再运行原有滤波公式。 */
```

- [ ] **3. 增加真实解码测试。** 向真实 `DecodeDJIMotor` 输入 rpm=600、current=-123 的已知大端 CAN 字节，断言新字段精确相等，原 `speed_aps/real_current` 与关闭观测时相同；同一个 HAL tick 连续两帧改变 `rx_cycles` 和序号。用现有测试夹具验证复制期间发布新反馈不会形成拼接快照。运行 `make -B -C tests/dji_chassis_power_rtt test CC=gcc`。

- [ ] **4. 先写时间回绕测试，再实现整数展开。** 以下为真实测试主体；补频率为 0、粗 tick 回绕、重复计时、倒序、频率切换和超过一个周期计数回绕间隔的暂停案例。

```c
PPObservationClock c = {0};
assert(PPObservationClockUpdate(&c, UINT32_MAX - 999U, 100U, 1000000U));
uint64_t start = c.now_us;
assert(PPObservationClockUpdate(&c, 1000U, 102U, 1000000U));
assert(c.now_us - start == 2000U);
uint64_t rx = 0;
assert(PPObservationReceiveTime(&c, 500U, 101U, 20000U, &rx));
assert(c.now_us - rx == 500U);
assert(!PPObservationReceiveTime(&c, 500U, 1U, 20000U, &rx));
```

算法采用 `(uint32_t)(new_cycles-old_cycles)` 和 uint64 乘法换算；用粗 tick 差校验周期差是否在 ±2 ms 容差内且不可能跨多次硬件回绕。保留除法余数：`numerator=(uint64_t)delta_cycles*1000000U+fraction`，`now_us+=numerator/hz`，`fraction=numerator%hz`；长序列非整微秒步进累计误差须小于 1 μs。首次以粗 tick 建立局部原点；频率变化、倒序或不能确定的长间断返回 false、增加 `source_epoch`、重建时间原点并清除 fraction。接收时间只在粗细年龄均通过且不超过 `max_age_us` 时由 `now_us-age_us` 得到；超界不能下溢为巨大时间。记录中的 `rx_cycles/rx_tick_ms/hz/source_epoch` 始终保留供独立核查。

- [ ] **5. 绿灯并提交。** `make -B -C tests/speed_observation test CC=gcc`、开关契约和 DJI 测试均通过。Make 已 wildcard 纳入 `modules/predictive_power/*.c`，CMake 已对此目录应用 `-fno-fast-math`，不新增重复源列表。

```powershell
git add modules/predictive_power/speed_observation_config.h modules/predictive_power/speed_observation_time.h modules/predictive_power/speed_observation_time.c modules/motor/DJImotor/dji_motor.h modules/motor/DJImotor/dji_motor.c config/atype-options.txt tests/speed_observation tests/dji_chassis_power_rtt
git diff --cached --stat
git commit -m "feat(speed): add independent raw feedback observation and time base"
```

## Task 2：有界短窗 CSC1 捕获和可往返解码

**Files:** 新增 `modules/telemetry/chassis_speed_capture.{h,c}`、`scripts/chassis-speed-capture.py`、`tests/speed_observation/test_capture.c`、`test_capture_decode.py`；修改测试 Makefile、根 Makefile/CMake 数值文件规则。

**Interfaces:** 新模块只消费值对象；与任务锁、UART 隔离。固定存储由一个静态实例持有。

```c
typedef enum { CSC_IDLE, CSC_ARMED, CSC_RECORDING, CSC_FROZEN, CSC_DRAINING } CSCState;
/* 原始 payload 40 B，周期 payload 512 B；不直接发送 C struct。 */
typedef struct { uint8_t bytes[40]; } CSCRawRecord;
typedef struct { uint8_t bytes[512]; } CSCCycleRecord;
typedef struct { uint8_t bytes[1024]; } CSCMetadata;
typedef struct {
    CSCRawRecord raw[192]; CSCCycleRecord cycle[32];
    CSCMetadata metadata;
    uint8_t end_payload[48];
    uint32_t capture_id, rejected_records, outside_window, transport_retries;
    uint32_t session_outside_window;
    uint16_t raw_count, cycle_count, drain_index;
    uint8_t drain_kind, freeze_reason;
    CSCState state;
} CSCCapture;
bool CSCArm(CSCCapture *, uint32_t capture_id, const CSCMetadata *);
bool CSCAppendRaw(CSCCapture *, const CSCRawRecord *);
bool CSCAppendCycle(CSCCapture *, const CSCCycleRecord *);
void CSCFreeze(CSCCapture *);
bool CSCBeginDrain(CSCCapture *); /* 冻结本窗导出统计，后续不再修改。 */
bool CSCPeekFrame(const CSCCapture *, uint8_t *dst, uint16_t capacity, uint16_t *length);
void CSCFrameAccepted(CSCCapture *);
void CSCDrainComplete(CSCCapture *, bool transport_idle);
```

- [ ] **1. 固定 schema 和字节测试。** 线帧为 `"CSC1"[4] + schema:u16 + kind:u8 + flags:u8 + payload_len:u16 + capture_id:u32 + record_index:u32 + payload + crc16:u16`，全小端，schema=1；CRC 使用仓库 `crc_16()`，反射多项式 0xA001、初值 0xFFFF、无末尾异或，覆盖头部和 payload，末尾小端保存。共同已知向量 `"123456789" → 0x4B37`。kind 为 `1=metadata, 2=raw, 3=cycle, 4=end`，payload 长度分别为 1024/40/512/48，最大帧 1044 B；拒绝未知 schema、非法长度、坏 CRC、截断与超上限数量。浮点以 IEEE754 binary32 编码，用 memcpy 转位，不依赖编译器结构布局。

原始 40 B payload 固定为：`wheel:u8, temperature:u8, sample_source:u8, flags:u8, sequence:u32, rx_tick_ms:u32, rx_cycles:u32, raw_rpm:i16, raw_feedback_current:i16, filtered_rpm:f32, filtered_current:i16, reserved:u16, sample_time_us:u64, event_index:u32`。`sample_source=0` 表示未知，`sample_time_us=0` 不参与时间推断。已知字节用例至少包含负电流、序号回绕、同 tick 多帧及非对称四轮值。

周期 512 B payload 的精确字段和 offset 见文末协议附录；C 与 Python 分别按该协议实现，不复制 C 内存。布局如下；未使用保留字节固定为 0：

| 块 | 字节 | 必须包含 |
|---|---:|---|
| 周期头 | 80 | 起点/结束 cycle、粗 tick、period/execution、control/config/model/strategy/KF 版本、source epoch、有效掩码、提交/成功组掩码、丢失累计 |
| 每轮块 ×4 | 80×4 | 使用的反馈序号与 rx cycles、raw rpm/current、滤波 rpm/current、目标、PID 候选、限流值、最终 i16、温度、sender group、KF omega/alpha/P00/P01/P11/innovation/reason/valid |
| 功率、提交与资源诊断 | 112 | 输入/负载瓦特及各自序号/年龄/有效标志、旧 guard_rpm[4]、KF/求解耗时、控制结束至提交完成时间、最小栈/堆、原始时间来源及必要保留位 |

metadata 除上述信息外，还保存起窗前四轮完整 KF checkpoint、时间展开状态和实际 PID 参数；具体布局见协议附录。end 带冻结原因、两流长度、丢记录/窗外/传输重试计数。版本变化时冻结当前窗，不能在一个未声明的新配置下继续采样。

- [ ] **2. 写满窗、重试、重启用例并运行红灯。** 测试主体：

```c
static CSCCapture c;
CSCRawRecord r = {{0}};
CSCMetadata metadata = {{0}};
assert(CSCArm(&c, 7U, &metadata));
for (unsigned i = 0; i < 192; ++i) assert(CSCAppendRaw(&c, &r));
assert(c.state == CSC_FROZEN);
assert(!CSCAppendRaw(&c, &r));
assert(c.raw_count == 192 && c.outside_window == 1);
assert(!CSCArm(&c, 8U, &metadata)); /* 未导完不能覆盖实验。 */
```

两流任一达到容量立即共同冻结；正常满窗不是“丢记录”，另记 freeze reason，冻结后事件记 `outside_window`。在录制窗内因竞争/格式错误未记录的事件才增加 `rejected_records`；计数不可静默丢失。`CSCBeginDrain` 将计数冻结到 end payload；之后窗外事件/传输重试只增加会话诊断，不改变当前窗导出字节。传输未接收完整帧时，重复 Peek 返回同样字节且游标不动；增加 end 等待重试时注入反馈仍逐字节一致的测试。

- [ ] **3. 实现状态机和显式序列化。** 首版固定选择“临界区内完成一条记录复制及计数发布”：周期 payload 在临界区外准备，关中断区只做有界 512 B memcpy、计数更新和冻结标志发布；ISR 原始记录最多复制 40 B。实测临界区耗时纳入预算。ISR/任务均不得等待对方完成，不使用自旋或阻塞锁；不在临界区计算 KF、CRC 或执行 UART。Arm 先在未录制的存储中准备 metadata，再在周期边界原子发布状态；头中写明调用方锁契约，不能只加 `volatile` 充当原子保证。

```c
_Static_assert(sizeof(((CSCCapture *)0)->raw) +
               sizeof(((CSCCapture *)0)->cycle) == 24064U,
               "capture payload memory budget changed");
```

禁用时不实例化这块 RAM。32 KiB 预算还必须计入其他新增状态、快照及任务堆栈，不能只看上式。

- [ ] **4. 实现解码 CLI 并做双向独立验证。** 命令契约：`python scripts/chassis-speed-capture.py decode --input capture.bin --output capture.jsonl`。输出逐条保留 kind/capture_id/event_index、时间来源和单位；原始帧、周期帧分别导出，不把相同毫秒事件合并。C 生成黄金文件由 Python 验证，Python 构造已知数据由 C 验证 CRC/字段；日志垃圾前后可重同步，缺 metadata/end 或丢帧时标记 `complete=false`，不得填补缺测。

- [ ] **5. 验证旧格式兼容并提交。** 执行新 capture C/Python 测试，另执行 `make -B -C tests/chassis_power_capture test CC=gcc` 和 `powershell -NoProfile -File tests/chassis_power_capture/test_decoder.ps1`。Make 显式加入新 telemetry 源；CMake 的 glob 自动包含，二者保持数值检查行为一致。

```powershell
git add modules/telemetry/chassis_speed_capture.h modules/telemetry/chassis_speed_capture.c scripts/chassis-speed-capture.py tests/speed_observation Makefile CMakeLists.txt
git diff --cached --stat
git commit -m "feat(telemetry): add bounded versioned speed capture"
```

## Task 3：最终发送字节、分组结果及许可窗口核验

**Files:** 修改 `dji_motor.{h,c}`、`dji_power_adapter.c`、`chassis_power_types.h`、`tests/dji_chassis_power_rtt/test_dji_chassis_power_rtt.c`。

**Interfaces:** 在现有 `DJIMotorCycle_s` 中增加只读提交诊断指针；不改变 `record_final` 参数顺序或力控 hook 的控制语义。

```c
typedef struct {
    uint8_t bytes[6][8]; /* 现有两路 CAN 各三个 sender group。 */
    uint32_t begin_cycles[6], end_cycles[6];
    uint8_t submitted_groups, successful_groups;
    bool enabled_at_submit[DJI_MOTOR_CNT];
} DJIMotorSubmission_s;
/* DJIMotorCycle_s 追加：const DJIMotorSubmission_s *submission; */
```

使用真实 sender 数量常量替代重复裸数字，并在构建时断言与 `sender_assignment` 长度一致。`ChassisPowerFinalInput_s` 追加 `submitted_groups/successful_groups`，原 `can_submitted` 保留兼容语义。

- [ ] **1. 写故障注入并确认基线结果。** 扩展现有 `stop_in_can_wait` 到速度组；分别注入计算期间停止、等待邮箱期间停止、反馈过期、配置版本变化、单组失败及四轮跨两组第二次提交前禁用。断言测试捕获的真实 `tx_buff[8]` 与最终诊断逐字节相等。若基线已通过某项，保留绿灯回归，不为了制造失败改变期望。

- [ ] **2. 在每组提交后立即保存实际提交字节。** 在下一组 guard 可能修改全局缓冲前完成快照；失败也保存 attempted bytes，但成功掩码为 0。伪代码中的所有对象均为驱动现有变量或上述新类型：

```c
submission.begin_cycles[i] = DWT_GetCycleCount();
/* 原 CANTransmit/Guarded 调用保持在这里；success 是其真实返回值。 */
submission.end_cycles[i] = DWT_GetCycleCount();
memcpy(submission.bytes[i], sender_assignment[i].tx_buff, 8);
submission.submitted_groups |= (uint8_t)(1U << i);
if (success) submission.successful_groups |= (uint8_t)(1U << i);
```

同步保存该组电机使能。最终记录从 per-group bytes 还原各轮已尝试提交值；不能在后续 guard 清零后，用全局 `final_commands` 冒充早先已成功入队的命令。未尝试发送的组单独标记；“提交成功”仍不是电机执行确认。不要修改前一组已经提交的历史来制造全零的一致假象。

- [ ] **3. 按复现结果接入速度 `validate_current`。** 如邮箱等待期间撤销许可用例失败，新增 `AdapterValidateCurrent`，签名严格匹配 `DJIMotorOutputHooks_s.validate_current`，注册到速度 hooks。仅作 O(4) 的当前许可、命令/config 版本、控制 TTL、轮反馈帧龄、在线/停止状态及现有功率许可检查，不在关中断区重跑控制器；调用现有门控依据，不能凭 KF 有效标志放行。返回 false 时复用现有 `ValidateBeforeEnqueue` 清零当前尚未提交的输出。版本/权限稳定时不改变候选字节。

```c
static bool AdapterValidateCurrent(void *context, const DJIMotorCycle_s *cycle,
    const int16_t commands[DJI_MOTOR_CNT], const bool enabled[DJI_MOTOR_CNT],
    uint32_t now_ms);
/* hooks 中仅在故障注入证明需要时新增： */
/* .validate_current = AdapterValidateCurrent */
```

完整 predicate 必须由现有速度发送门控逐项抽取并覆盖上述测试，不复制力控的 torque/calibration 规则。四轮跨组无法原子撤销已发送组，应准确记录发生的部分提交并令需求增长失效，不声称已实现跨 CAN 组原子发送。

- [ ] **4. 运行回归并独立提交修复。** `make -B -C tests/dji_chassis_power_rtt test CC=gcc`、`make -B -C tests/chassis_power_module test CC=gcc`。先保存“仅 guard 修复”的基线；后续采集/KF 开关对这个共同基线比较。若没有复现许可窗口，只提交诊断与回归，报告核验结果。

```powershell
git add modules/motor/DJImotor/dji_motor.h modules/motor/DJImotor/dji_motor.c modules/motor/DJImotor/dji_power_adapter.c modules/chassis_power/chassis_power_types.h tests/dji_chassis_power_rtt
git diff --cached --stat
git commit -m "fix(speed): verify final submission permissions and preserve per-group bytes"
```

## Task 4：接入短窗采集、周期和资源测量

**Files:** 新增 `application/runtime/chassis_speed_observation.{h,c}`、`tests/speed_observation/test_runtime.c`；修改 `dji_motor.c`、`dji_power_adapter.{h,c}`、`chassis_power_types.h`、`robot_task.c`、Makefile。

**Interfaces:** 新 runtime header 只使用标准类型和观测值对象，底层模块不得 include 应用层 header。驱动通过新增注册函数 `DJIMotorSetSpeedFeedbackObserver(void (*observer)(void *, unsigned, const DJI_Motor_Measure_s *), void *context)` 发布值；注册/轮索引映射在初始化停机时完成，回调仅复制原始记录。

```c
/* runtime 提供给 robot_task.c；不由驱动直接调用应用层。 */
void ChassisSpeedObservationInit(void);
void ChassisSpeedObservationMotorBegin(uint32_t cycles, uint32_t tick_ms);
void ChassisSpeedObservationMotorEnd(uint32_t cycles, uint32_t tick_ms);
void ChassisSpeedObservationTask(void);
bool ChassisSpeedObservationArm(uint32_t capture_id);
void ChassisSpeedObservationFreeze(void);
```

runtime 持有静态 `CSCCapture`，初始化调用新增 `bool DJIPowerAdapterSetSpeedCapture(CSCCapture *)`，适配器保存指针并通过上述 observer 注册四轮反馈映射。适配器内部 raw callback 编码并调用 `CSCAppendRaw`；`AdapterRecordFinal` 生成一份静态 pending 周期记录。runtime 在 MotorEnd 调用新增 `bool DJIPowerAdapterTakeSpeedCycle(CSCCycleRecord *)` 取走当前周期唯一记录，写 period/execution 后调用 `CSCAppendCycle`，不得重复取上一周期。另提供 `bool DJIPowerAdapterDescribeSpeedCapture(CSCMetadata *)`，在 Arm 前填充已注册四轮的方向、运行 PID 和适配器时间状态。以上 API 声明在 `dji_power_adapter.h`；纯 telemetry 模块不依赖 DJI 或 application。实现前以 `tests/architecture/test_layers.py --strict` 检查依赖。

采集操作使用调试器请求邮箱，不扩 USB/CAN 协议。runtime 导出 `volatile ChassisSpeedObservationRequest chassis_speed_observation_request`，字段为 `uint32_t sequence, action, capture_id, acknowledged_sequence, result`，action 为 `1=arm, 2=freeze`。操作员先写 action/capture_id，最后写递增 sequence；MotorBegin 只消费未确认的完整请求，完成或拒绝后发布 result 与 acknowledged_sequence。禁止在 ISR 里响应请求。Arm 在该周期 KF 计算之前，通过新增 `bool ChassisPowerShadowGetWheelKFCheckpoint(PPWheelKF out[4], PPWheelKFConfig *config)` 复制已完成周期状态以及 `PPObservationClock`，编码 metadata 后才开放录制；关闭 KF 时 checkpoint 标记无效。由此回放可从热机初态逐周期重现。

- [ ] **1. 写假时钟与 UART 拥塞测试。** 连续周期起点为 0/2500/5200 μs、耗时为 400/500/600 μs，期望 `period_us=2500/2700`、`execution_us=400/500/600`，不能得到 2 ms 常量。让 UART 永远返回 0，验证 MotorControl 调用次数、CAN 字节及调度调用不变，capture 冻结且重试计数增加；不会在快任务内等待 drain。

- [ ] **2. 接入起点、出口与提交时间。** 只用整数 cycles 测量。临界区复制四轮 raw/filter/sequence/time 和控制版本；`AdapterLimit` 前后分别保存 PID 候选与限流结果，最终字段来自 Task 3。完整周期耗时在 `MotorEnd` 后才写入周期记录，不用上一周期的耗时冒充本周期。第一周期 period 标记无效，日志里保留 valid 位。

```c
const uint32_t begin = DWT_GetCycleCount();
ChassisSpeedObservationMotorBegin(begin, HAL_GetTick());
MotorControlTask();
ChassisSpeedObservationMotorEnd(DWT_GetCycleCount(), HAL_GetTick());
/* 原 osDelay(2) 保持原位。 */
```

同时记录反馈接收间隔、接收至使用年龄、KF/求解耗时、最后计算结束至每组提交开始/结束的差；“提交”是 CAN 入队，不是线上的实际发送完成。总周期与执行时间各有 histogram/最大值/超预算次数，分位数由完整窗或累计 histogram 输出。

展开时钟由速度适配器单独持有并在 `AdapterLimit` 使用当前 DWT/HAL tick 更新，再从一致测量快照的 rx cycles 计算原始帧时间。不能直接拿更早的 MotorBegin 时间作为“当前观测时刻”，因为快照可能包含其后到达的反馈。Task 4 的起点/出口计时仅用于周期测量。采集、metadata、drain frame 与 pending 周期大对象均放静态受控存储，不在 512 words 电机任务栈上临时分配 1 KiB 缓冲；新增 scratch 也计入 RAM。

- [ ] **3. 完成两类功率来源的映射。** 负载功率取 `PowerMeterStatus_s.power_w/rx_sequence/last_rx_tick_ms`；输入功率取 `SuperCapSnapshot_s.input_power` 及 `input_received_frame_count`。保留两者各自有效性、时间和传感器边界声明，缺失则无效；不将 `snapshot_tick_ms` 当新测量事件。慢任务原子发布来源快照，快任务只复制，禁止逐周期新增 CAN 查询。

- [ ] **4. 实现低优先级 drain 和资源采样。** 扩展/复用现有低优先级采集任务，按 200 ms 间隔测电机/应用/观测任务栈高水位、`xPortGetFreeHeapSize()` 和 `xPortGetMinimumEverFreeHeapSize()`。每轮先 `BSPUARTLogPoll()`；仅在当前确认禁驱且全窗冻结后 `CSCBeginDrain`，然后 `CSCPeekFrame → BSPUARTLogWrite → CSCFrameAccepted`；返回长度不等于完整帧时不推进。恢复驱动时暂停新增排队；已排队日志可继续发送。最后一帧被接收后仍持续 Poll，UART idle 后才能 `CSCDrainComplete`。测试最后一帧首次启动遇 HAL_BUSY/HAL_ERROR 后无新 Write 仍能导完。不改波特率、不占用 DBUS、不增加 CAN 帧。

- [ ] **5. 验证原始流覆盖和窗长限制。** 同一快周期两帧均进入原始流；周期记录只引用实际被控制使用的最新事件，统计中间未被 KF 消费的事件数。默认 192 条原始＋32 条周期记录，窗长由实际反馈/控制频率决定，约几十毫秒只是容量推算，最终记录实测起止时间。不得把多个窗直接拼成连续轨迹，也不得凭短窗生成长阶跃恢复时间。

- [ ] **6. 绿灯并提交。** 新 runtime 测试、真实驱动测试、架构严格检查、CPC1 回归通过；检查关闭两开关时没有新增采集实例/任务。提交本任务文件，消息 `feat(speed): capture coherent cycles and timing diagnostics`。

## Task 5：纯 C 两状态 KF 与异常恢复

**Files:** 新增 `modules/predictive_power/wheel_speed_kf.{h,c}`、`tests/predictive_power/test_wheel_speed_kf.c`；修改 `tests/predictive_power/Makefile`。

**Interfaces:**

```c
typedef enum {
    PP_KF_OK, PP_KF_INITIALIZED, PP_KF_DUPLICATE, PP_KF_TIME,
    PP_KF_VALUE, PP_KF_INNOVATION, PP_KF_STALE, PP_KF_CONFIG, PP_KF_NUMERIC
} PPWheelKFReason;
typedef struct {
    uint32_t version, max_gap_us, max_age_us, max_horizon_us;
    float q, r, initial_p00, initial_p11, gate_squared;
    uint8_t max_rejections;
} PPWheelKFConfig;
typedef struct {
    uint64_t rx_time_us;
    uint32_t sequence, source_epoch;
    float omega_rad_s;
    bool online;
} PPWheelSample;
typedef struct {
    float omega, alpha, p00, p01, p11, innovation, innovation_variance;
    uint64_t corrected_time_us, output_time_us;
    uint32_t sequence, source_epoch, config_version;
    bool valid;
    PPWheelKFReason reason;
} PPWheelEstimate;
typedef struct {
    PPWheelEstimate corrected;
    uint64_t seen_time_us;
    uint32_t seen_sequence, seen_source_epoch;
    uint8_t rejected_consecutive;
    bool seen, initialized;
} PPWheelKF;
bool PPWheelKFConfigValid(const PPWheelKFConfig *);
void PPWheelKFReset(PPWheelKF *);
PPWheelKFReason PPWheelKFCorrect(PPWheelKF *, const PPWheelKFConfig *, const PPWheelSample *);
bool PPWheelKFPredict(const PPWheelKF *, const PPWheelKFConfig *,
                     uint64_t now_us, uint32_t horizon_us, PPWheelEstimate *);
```

- [ ] **1. 写最小真值和只读预测测试，运行红灯。** 以下参数仅为合成测试夹具，禁止拷入板级默认值。

```c
const PPWheelKFConfig cfg = {.version=1, .max_gap_us=20000, .max_age_us=20000, .max_horizon_us=20000,
    .q=1, .r=0.25f, .initial_p00=1, .initial_p11=100,
    .gate_squared=25, .max_rejections=3};
PPWheelKF k = {0};
PPWheelSample s = {.rx_time_us=100000, .sequence=1, .source_epoch=1,
                   .omega_rad_s=10, .online=true};
assert(PPWheelKFCorrect(&k, &cfg, &s) == PP_KF_INITIALIZED);
PPWheelKF before = k;
PPWheelEstimate out;
assert(PPWheelKFPredict(&k, &cfg, 100000, 8000, &out));
assert(fabsf(out.omega - 10) < 1e-6f);
assert(out.p00 > k.corrected.p00);
assert(memcmp(&before, &k, sizeof k) == 0);
assert(PPWheelKFCorrect(&k, &cfg, &s) == PP_KF_DUPLICATE);
assert(k.corrected.corrected_time_us == 100000);
```

运行 `make -B -C tests/predictive_power test CC=gcc`，预期缺少新 API 而失败；Makefile 将此测试作为独立 executable 纳入 `test`。

- [ ] **2. 实现预测和 Joseph 校正。** 转速单位 rad/s，角加速度 rad/s²。以校正状态为唯一持久状态，预测输出不回写。`q>=0, r>0, initial_p00>0, initial_p11>0`，其他门限须有限且为正、版本非零；不提供猜测的板级默认 `q/r`。

```c
/* 预测，a/b/d 分别对应 P00/P01/P11。 */
float a = p00 + 2.0f*dt*p01 + dt*dt*p11 + q*dt*dt*dt/3.0f;
float b = p01 + dt*p11 + q*dt*dt/2.0f;
float d = p11 + q*dt;
float w = omega + dt*alpha;
float residual = measurement - w;
float variance = a + r;
float k0 = a/variance, k1 = b/variance;
omega = w + k0*residual;
alpha += k1*residual;
/* Joseph: (I-KH)P(I-KH)' + K R K'。 */
p00 = (1-k0)*(1-k0)*a + k0*k0*r;
p01 = (1-k0)*(b-k1*a) + k0*k1*r;
p11 = d - 2*k1*b + k1*k1*a + k1*k1*r;
```

每次预测/校正后验证所有状态有限、对角非负且 `p00*p11-p01*p01` 在与尺度相关的浮点容差内；微小舍入误差可修复到 PSD，明显失效返回 `PP_KF_NUMERIC` 并撤销状态。测试不得用与实现完全相同的公式作为唯一 oracle：合成恒速/恒加速度真值和独立 double 计算同时覆盖。

- [ ] **3. 实现双水位和时间异常。** `seen_*` 防止同一被拒绝事件反复更新拒绝次数；`corrected.*` 仅在接受测量后刷新。序号用 uint32 半区间判新：`advance != 0 && advance < 0x80000000U`，允许回绕；倒序不得降低任何水位。同微秒的新序号记时间拒绝，不做二次校正；同毫秒不同微秒正常更新。来源 epoch 变化、配置变化、在线恢复、可信新帧长间断时从新测量重建；重复或无效帧不能触发“恢复”。离线立即撤销有效性。

- [ ] **4. 实现有限创新拒绝和新鲜度。** 超 gate 的新事件先记录残差、拒绝；连续达到 `max_rejections` 撤销 KF 有效性，下一条时间/数值可信的新事件以测量值重新初始化并标记原因。不能把重复残差累计成多个拒绝。`Predict` 分别检查 `now_us-corrected_time_us <= max_age_us` 和 `horizon_us <= max_horizon_us`；now 早于校正时刻、目标加法溢出均无效。实际传播 dt 为 `now_us+horizon_us-corrected_time_us`。未来预测不刷新测量年龄。增加“帧龄 1 ms、horizon 20 ms、max_age/max_horizon 均 20 ms 仍有效”及“帧龄 21 ms 时 horizon=0 也无效”的测试。

- [ ] **5. 扩充覆盖并绿灯。** 恒速、恒加速度、正反向阶跃、停车、量化噪声、不同 dt、丢帧、dt=0、序号/时间倒序、回绕、NaN/Inf、非法配置、PSD、长间断、epoch/配置切换、真实阶跃恢复全部有断言。至少验证长序列无非法数值和重复预测不改变校正状态。只对合成真值声明精度。

- [ ] **6. 提交。** 消息 `feat(speed): add bounded two-state wheel speed Kalman filter`，只暂存新 KF 与测试/测试 Makefile。

## Task 6：四实例影子、对照输出和超时暂停

**Files:** 修改 `chassis_power_types.h`、`chassis_power_shadow.{h,c}`、`chassis_power_shadow_energy.c`、`dji_power_adapter.c`、`test_shadow.c`、`verify_speed.py`。

**Interfaces:** `ChassisPowerCycleInput_s` 追加 `PPWheelSample raw_speed[4]`、`uint64_t observation_now_us` 和时间有效掩码；配置仅在完整通过校验后原子发布，于下一快周期切换。新增以下函数与诊断对象：

```c
bool ChassisPowerShadowSetWheelKFConfig(const PPWheelKFConfig *);
bool ChassisPowerShadowGetWheelKFCheckpoint(PPWheelKF out[4], PPWheelKFConfig *config);
typedef struct {
    PPWheelEstimate corrected[4], current[4];
    float future_omega[3][4], future_p00[3][4]; /* horizon: 2/8/20 ms */
    uint32_t accepted_age_us[4];
    uint8_t future_valid_mask[3];
    uint32_t corrected_count[4], skipped_events[4], kf_elapsed_us;
    uint32_t consecutive_overruns;
    bool configured, paused;
} ChassisWheelKFDiagnostics;
/* ChassisPowerShadowSnapshot 追加 ChassisWheelKFDiagnostics wheel_kf; */
```

- [ ] **1. 写未装模型仍运行 KF 的红灯测试。** 对 `test_shadow.c` 送入配置、四轮原始事件，无 `PPModel`；断言 KF 可初始化，`dynamic_model=false`，`legacy` 输入及最终命令不变。再次送同序号原始帧，只允许当前时刻预测，不增加 `corrected_count`。

- [ ] **2. 在动态模型分支之前更新四实例。** raw rpm → `rpm*(2*pi/60)`，不乘车辆几何方向，不再次滤波；PID/filter/old transient 保持原字段。每轮只校正本周期最新一致快照；中间被覆盖帧计入 `skipped_events`，不得宣称在线使用了全部 CAN 事件。无配置时 `configured=false`，只采集，不代入任何猜测噪声参数。

板上参数安装固定采用独立实验构建：Task 7 的 `emit-kf-header` 子命令生成 `chassis_speed_kf_experiment.h`，包含 `static const PPWheelKFConfig chassis_speed_experiment_kf_config` 及其 canonical CRC16。Make 添加 `-I$(BUILD_DIR)`，CMake 添加 binary directory include；仅 `CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1` 时 include 此头，缺文件直接构建失败。`ChassisSpeedObservationInit()` 在任务启动前校验 CRC、调用 `ChassisPowerShadowSetWheelKFConfig()`，快周期切换后 getter 回读 version/checksum；失败保持未配置并在邮箱 result/诊断中暴露原因。默认配置不生成或安装任何 q/R。Task 4 的 metadata checkpoint 在本任务接通，增加“先运行 100 帧后 arm，回放逐周期一致”测试。

```c
for (unsigned w = 0; w < 4; ++w) {
    PPWheelKFCorrect(&wheel_kf[w], &active_kf_config, &input->raw_speed[w]);
    PPWheelKFPredict(&wheel_kf[w], &active_kf_config, input->observation_now_us, 0,
                    &working.wheel_kf.current[w]);
}
```

`observation_now_us` 由 Task 1 的同源时钟输入，不调用 HAL。跨模型/控制配置版本在诊断中分段；仅改变功率模型不应隐式重置同参数 KF，改变测量方向/单位/来源必须增加 epoch 并重建。

- [ ] **3. 生成多步轮速对照。** 用校正状态向 `now+2/8/20 ms` 外推，getter 提供 horizon 的 omega/P00 和起始反馈年龄；超过预测上限明确无效。CSC1 周期帧保存 corrected 态，不混入 current/future 态，完整未来协方差用该状态和冻结配置离线重建，并以主机测试核对 getter。不要将未来预测写回 PID、transient、功率额度、旧 `PPObserver.state` 或动态求解输入。记录真实旧 `guard_rpm` 并保留其绝对值语义；本批 KF 的功率影响在离线对齐数据上评价。

- [ ] **4. 加连续超时暂停。** 新观测成本纳入快层总预算；用可注入的时钟测试 299/300/301 μs 边界、连续 3 次超时及恢复。连续 3 次超预算后，暂停快层影子求解和 KF，保持旧控制/采集/最终记录；配置重新发布或显式重新 arm 实验才恢复，记录暂停原因与时间。慢层超过 2 ms 同样失效并暂停慢层计划，保留 TTL/原通信策略。板上 DWT 频率无效时不报告 0 μs。

本批不增加求解复杂度，允许保留既有“单次结束后检测”的限制，但不得把它称作求解内硬截止。如果单次最坏耗时已经使发送周期越界，S1 停止启用；先降频/关闭既有影子求解，只保留经验证可运行的观测，或另行实现带时钟回调的 `PPAllocate` 检查点。求解内截止最迟在增加复杂求解或进入 S3 前完成；没有余量证据不能默认启用。

- [ ] **5. 验证隔离并提交。** `test_shadow.c` 覆盖配置切换、无效原始帧、失联、超时、发送失败且最终原字节可追溯。扩展 `verify_speed.py` 为 default/capture-only/shadow/shadow+capture+KF 四类配置，保留原三组 7,000 条功控轨迹比较及非法接管宏拒绝；实际驱动 CAN 字节隔离仍由 DJI 测试证明，不能用模块轨迹替代。提交消息 `feat(speed): integrate wheel KF diagnostics without control takeover`。

## Task 7：manifest、无泄漏回放及报告

**Files:** 修改 `scripts/chassis-speed-capture.py`；新增 `scripts/wheel-speed-replay.py`、`tests/predictive_power/test_speed_replay.py`、`tests/speed_observation/test_manifest.py`、`tests/speed_observation/synthetic-manifest.json`、`synthetic-kf-config.json`。

**Interfaces:**

```text
python scripts/chassis-speed-capture.py manifest --metadata experiment-metadata.json --output build/speed-experiment/manifest.json
python scripts/chassis-speed-capture.py emit-kf-header --kf-config kf-config.json --output build/speed-kf/chassis_speed_kf_experiment.h
python scripts/wheel-speed-replay.py --capture capture.jsonl --manifest manifest.json --kf-config kf-config.json --output report.json
```

manifest 子命令保存当前提交、二进制/源补丁哈希、编译选项文件、工具链、实际 PID/Flash 导出及来源、静态功率/动态模型 checksum、KF 参数、传感器接线/板版本、轮序方向/单位/几何、轨迹编号、初末能量/温度/负载、捕获完整性、时间对齐方法、数据段 `train/validation/test`、冻结门槛及冻结时间。`--metadata` 文件提供无法从仓库推导的实测项；缺项用 `{value:null,status:"unmeasured"}`，不以源码默认值补实际 PID 或初始能量。合成 fixture 必须 `synthetic:true`。

- [ ] **1. 写拒绝伪完整实验的测试。** 没有实际 PID/Flash 来源、没有接线边界、未知采样延迟、缺冻结门槛时，报告允许生成但 `hardware_gate="insufficient_evidence"`。输入/负载未分别确认时功率项 `available=false`，不能填 0；manifest 哈希不匹配、非法单位或无有效轮速样本则命令非零退出。

```python
report = json.loads(output_path.read_text(encoding="utf-8"))
assert report["synthetic"] is True
assert report["hardware_gate"] == "insufficient_evidence"
assert report["power"]["available"] is False
assert report["power"]["reason"] == "unverified_measurement_boundary"
assert report["windows"]["stitched"] is False
```

- [ ] **2. 调用生产 C KF 并保留旧回放入口。** 采用现有回放脚本 `gcc -shared -fPIC -std=c11 -O2 -fno-fast-math`＋ctypes 的模式，声明 Task 5 的准确 ABI；启动时比较 C 导出的 sizeof/offset 或用小 C bridge 避免静默布局错配。若采用 bridge，新增 `tests/predictive_power/wheel_speed_replay_bridge.c` 并只暴露固定标量/数组，不复制 KF 算法。旧 `predictive-power-replay.py --model --capture --output` 与原测试保持可运行。`emit-kf-header` 严格校验 JSON 数值/版本/单位，按附录 40 B 参数编码计算 CRC16，输出可编译的常量；用主机编译读取生成头，逐字段与 JSON 往返比较。校验和验证完整性，参数是否标定仍由 manifest 证明。

- [ ] **3. 实现四条因果对照。** 原始 rpm 直接保持；已捕获的旧滤波/真实 transient 输出；按真实 dt 的一阶低通 `beta=dt/(tau+dt)`；KF。低通的 tau、KF 的 q/R 从训练段确定，验证段选择，测试段不再调整；配置哈希与冻结门槛写报告。本批提供参数 JSON 的反复回放，不声称自动辨识 q/R；训练人员记录每个候选和选择指标，然后用选定 JSON 生成实验头。在线一致性回放从 metadata 的 clock/KF checkpoint 恢复，只消费周期引用的 sequence；缺少窗内 raw 事件时，使用周期帧携带的完整原始快照，不能填造历史。全原始事件离线回放另列 `all_events`，不能把额外反馈带来的收益归给滤波器本身。实际 dt 低通从首可信样本开始并标记热机段，热机段不混入稳态分数。

- [ ] **4. 写未来数据泄漏与独立真值测试。** 固定种子产生恒速、恒加速度、阶跃、停车、换向、非均匀时间和噪声。先用共同前缀生成预测，再替换后缀测量，断言既有预测完全不变；仅评分改变。2/8/20 ms 目标采用对应窗口内未来观测，评分匹配容差写配置；断窗、丢失、跨版本、跨 train/test 段不连线。合成真值 RMSE 与真实未来原始观测 RMSE 分开命名。

- [ ] **5. 输出完整指标和计数。** 每轮/工况/horizon 输出样本数、有效率、拒绝原因、稳态噪声、轮速预测 RMSE、加速段低估尾部、创新分布、响应延迟、周期/执行/求解/提交的 p50/p95/p99/max 与超预算次数。记录旧模型对齐功率和 KF 轮速输入下同一模型的差异，仅在边界/延迟/模型证据齐备时计算 `max(P_measured-P_predicted,0)`；不把这个值升级为硬保护上界。

完整轨迹才输出速度 IAE、稳态误差、过冲/恢复时间、电流总变化与峰值；输入、负载分别积分超限能量。短窗不足覆盖响应时明确 `not_observable_in_window`。保留失败/超时/丢样本计数、全部有效试次和最差值；不按结果好坏删样本。

- [ ] **6. 验证并提交。** 新 replay/manifest 测试、`python tests/predictive_power/test_replay.py` 通过。使用合成输入生成一份可检查 JSON 报告，明确“工具链验证，非硬件收益”。提交消息 `feat(tools): replay raw wheel observations with reproducible comparison reports`。

## Task 8：软件矩阵、实车规程和阶段结论

**Files:** 新增 `docs/chassis-speed-observation.md`、`docs/superpowers/verification/2026-10-29-chassis-speed-observation.md`；按验证结果补上游任务的回归。

**Interfaces:** 本任务只接收前七项产物，产出两种独立状态：`software_verified`、`hardware_gate`，不能用主机通过代替 S1 实车放行。

- [ ] **1. 运行必要主机测试并保留输出。** 仓库根目录 PowerShell，Python 商店别名不可用时用已知解释器：

```powershell
$speedPython = 'C:\Users\20802\AppData\Roaming\uv\python\cpython-3.14-windows-x86_64-none\python.exe'
make -B -C tests/speed_observation test CC=gcc
make -B -C tests/predictive_power test CC=gcc
make -B -C tests/dji_chassis_power_rtt test CC=gcc
make -B -C tests/chassis_power_module test CC=gcc
make -B -C tests/chassis_power_capture test CC=gcc
& $speedPython tests/speed_observation/test_build_contracts.py
& $speedPython tests/speed_observation/test_capture_decode.py
& $speedPython tests/speed_observation/test_manifest.py
& $speedPython tests/predictive_power/test_speed_replay.py
& $speedPython tests/predictive_power/test_replay.py
& $speedPython tests/predictive_power/verify_speed.py
& $speedPython tests/architecture/test_layers.py --strict
powershell -NoProfile -File tests/chassis_power_capture/test_decoder.ps1
```

实现新 Makefile 时使上述 `test` 目标包括 time/capture/runtime；Python 测试是独立入口。测试二进制、真实实验数据和 build 输出不进入提交。

- [ ] **2. 四配置固件构建，使用独立目录。**

```powershell
make PROFILE=atype-vehicle BUILD_DIR=build/speed-base CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 -j8
make PROFILE=atype-vehicle BUILD_DIR=build/speed-capture CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 CHASSIS_SPEED_CAPTURE_ENABLE=1 LOG_OUTPUT=2 -j8
make PROFILE=atype-vehicle BUILD_DIR=build/speed-shadow CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 CHASSIS_PREDICTIVE_POWER_MODE=1 -j8
& $speedPython scripts/chassis-speed-capture.py emit-kf-header --kf-config tests/speed_observation/synthetic-kf-config.json --output build/speed-kf/chassis_speed_kf_experiment.h
make PROFILE=atype-vehicle BUILD_DIR=build/speed-kf CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 CHASSIS_PREDICTIVE_POWER_MODE=1 CHASSIS_SPEED_CAPTURE_ENABLE=1 CHASSIS_SPEED_KF_SHADOW_ENABLE=1 CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1 LOG_OUTPUT=2 -j8
& $speedPython scripts/chassis-speed-capture.py emit-kf-header --kf-config tests/speed_observation/synthetic-kf-config.json --output build/speed-kf-cmake/chassis_speed_kf_experiment.h
cmake -S . -B build/speed-kf-cmake -G Ninja -DATYPE_PROFILE=atype-vehicle -DCHASSIS_CONTROL_MODE=0 -DCHASSIS_FORCE_CALIBRATION_ENABLE=0 -DCHASSIS_PREDICTIVE_POWER_MODE=1 -DCHASSIS_SPEED_CAPTURE_ENABLE=1 -DCHASSIS_SPEED_KF_SHADOW_ENABLE=1 -DCHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1 -DLOG_OUTPUT=2
cmake --build build/speed-kf-cmake --parallel 8
```

上述 synthetic 参数只用于编译/主机管线验证，生成头保留 `synthetic=true` 标记，不能作为实车参数烧录；板上实验改用已冻结且 manifest 匹配的真实参数 JSON，构建目录也独立。检查两套编译命令中数值源 `-fno-fast-math` 在优化选项后生效。收集 map、`.su`、静态 RAM/FLASH；捕获状态与 KF 的总新增常驻内存目标 ≤32 KiB，核查任务堆栈从 FreeRTOS 堆分配造成的额外消耗。若工具缺失，记录实际限制，不把未运行的 CMake 写成通过。

- [ ] **3. 生成板上操作规程。** 默认关闭所有新开关。准备带 manifest 的独立实验固件；不把“执行软件计划”视为自动烧录或自动驱动车辆授权。板上阶段需具备用户安排的设备/场地和明确执行请求；在此之前状态写 `hardware_gate=not_run`。规程按以下顺序执行：静止→架空低速→稳定轮速→受限阶跃/斜坡→停车/换向→地面直行/横移/自旋/组合。每次 arm 前回读安装的 KF version/checksum 和窗口编号，冻结后停机导出，确认 end/CRC/丢失计数，再恢复；不使用长时硬堵转。调试器按以下写入顺序操作，最后读取 acknowledged_sequence/result 确认；freeze 使用 action=2 和新的 sequence。优先使用调试器无停核内存访问；若工具必须 halt，标记干扰并使该试次不能作为实时预算证据。

```text
set var chassis_speed_observation_request.capture_id = 1
set var chassis_speed_observation_request.action = 1
set var chassis_speed_observation_request.sequence = 1
print chassis_speed_observation_request.acknowledged_sequence
print chassis_speed_observation_request.result
```

- [ ] **4. 冻结训练和验收参数，再做配对实验。** 先用训练/验证段确定 q/R/tau 和创新门限；以基线噪声、仪器分辨率设定主指标与允许退化量，记录冻结时间。每主工况至少 10 组配对，交替顺序并匹配初末能量、温度、载荷。短窗只评局部噪声/预测；不能覆盖长阶跃时，延长受控采集能力或采用经时间对齐的独立连续测量，未取得这些数据前不给“响应延迟通过”结论。

- [ ] **5. 验收板上预算与故障行为。** 测最坏求解、日志拥塞、任务干扰、时钟长运行/回绕、部分离线、提交失败、许可切换；记录完整周期而非只看 shadow elapsed。电机/应用/采集任务高水位和剩余堆须满足事先冻结的余量门槛；300 μs/2 ms/32 KiB 中任何一项不满足均保持关闭。超时暂停和恢复需有独立试次，不能隐藏其样本。

- [ ] **6. 写阶段结论并提交。** 结论只可为以下三类：

| 结论 | 需要的证据与后续 |
|---|---|
| 软件完成，实车未验收 | 主机/构建/隔离通过；明确硬件、参数、实时性证据缺项，默认关闭 |
| S1 不放行或无可测收益 | 保留/关闭 KF，依据实测将后续工作转向 PID 或模型；不接管输出 |
| S1 验收通过，可规划 S2 | 独立实验预测收益、低估尾部、响应延迟和板上预算均满足冻结门槛；分别制定 S2a/S2b 计划 |

不在本计划内自动启用 Ki、前馈、变化命令模型、预测分配或 F334 新协议。提交消息 `docs(speed): record observation validation and staged release criteria`。

## 3. CSC1 精确字段协议

以下 offset 均为 payload 内字节偏移；`u/i/f` 分别为无符号整数、有符号整数、IEEE754 浮点。数组紧密连续。每个未显式命名的保留字节必须为 0。CRC 在低优先级编码帧时计算，不能在反馈 ISR 计算整帧 CRC。

### 3.1 周期 payload：512 B

| 起始偏移 | 类型、数量 | 字段（按顺序） |
|---:|---|---|
| 0 | u32 ×12 | tick_ms, start_cycles, end_cycles, period_us, execution_us, config_version, control_version, model_version, strategy_version, kf_config_version, source_epoch, cycle_index |
| 48 | u64 | observation_now_us |
| 56 | u32 ×4 | first_submit_cycles, last_submit_cycles, flags, skipped_event_total |
| 72 | u8 ×4 | enabled_mask, online_mask, submitted_groups, successful_groups |
| 76 | u32 | capture_rejected_records |
| 80 | 80 B ×4 | 下面的 wheel block，依次 LF/RF/LB/RB |
| 400 | 112 B | 下面的 diagnostics block |

wheel block：

| 相对偏移 | 类型、数量 | 字段（按顺序） |
|---:|---|---|
| 0 | u32 ×3 | sequence, rx_tick_ms, rx_cycles |
| 12 | i16 ×2 | raw_rpm, raw_feedback_current |
| 16 | f32 | filtered_rpm |
| 20 | i16 ×2 | filtered_current, final_command |
| 24 | f32 ×3 | target_rpm, pid_candidate, limited_command |
| 36 | u8 ×4 | temperature, sender_group, kf_reason, flags |
| 40 | f32 ×7 | corrected omega, alpha, P00, P01, P11, innovation, innovation_variance |
| 68 | u32 ×3 | accepted_age_us, corrected_count, skipped_events |

`accepted_age_us` 相对本周期 `observation_now_us`；无效时 UINT32_MAX。flags 的 bit0=raw_time_valid、bit1=kf_valid、bit2=sample_time_known、bit3=final_attempted、bit4=final_submitted_successfully，其余置零。KF reason 按 Task 5 枚举数值。周期 flags：bit0=period_valid、bit1=clock_valid、bit2=kf_configured、bit3=shadow_paused、bit4=synthetic_config；其余置零。unknown sample time 的值不参与计算。raw 流若未来出现非零 sample_source，必须另增 schema 定义其时基，本版只接受 0。

diagnostics block：

| 相对偏移 | 类型、数量 | 字段（按顺序） |
|---:|---|---|
| 0 | f32, u32, u32, u32 | input_power_w, input_sequence, input_tick_ms, input_flags |
| 16 | f32, u32, u32, u32 | load_power_w, load_sequence, load_tick_ms, load_flags |
| 32 | f32 ×4 | old_guard_rpm[4]，绝对转速语义 |
| 48 | u32 ×4 | kf_us, allocator_us, fast_shadow_us, slow_shadow_us |
| 64 | u32 ×3 | period_max_us, execution_max_us, control_to_submit_end_us |
| 76 | u32 ×5 | motor_stack_free_words, robot_stack_free_words, observation_stack_free_words, heap_free_bytes, heap_min_bytes |
| 96 | f32 ×3 | old_candidate_power_w, old_guard_power_w, physical_budget_w |
| 108 | u32 | shadow_reason |

input/load flags：bit0=present、bit1=fresh、bit2=verified_boundary、bit3=verified_time_alignment。后两位只根据实验 manifest 的已确认声明设置，不能从值看起来合理推断。功率无效时值清零且 valid 位为 0；解码器将其显示为缺测，不以 0 W 评分。资源/耗时不可用时用 UINT32_MAX，不显示零耗时。

### 3.2 metadata payload：1024 B

| 偏移 | 类型、数量 | 字段 |
|---:|---|---|
| 0 | u32 ×3 | metadata_version=1, capture_id, dwt_hz |
| 12 | u16 ×4 | raw_capacity=192, cycle_capacity=32, raw_bytes=40, cycle_bytes=512 |
| 20 | u8 ×4 | wheel_order，0/1/2/3 对应 LF/RF/LB/RB |
| 24 | i8 ×4 | 几何方向，±1，仅描述，不能改变电机原始观测符号 |
| 28 | u32 | unit_schema=1：rpm、原始等效电流、转子 rad/s、rad/s²、W、μs |
| 32 | u32 ×4 | clock source_epoch, cycles, tick_ms, flags |
| 48 | u64 | clock now_us |
| 56 | u32 ×2 | clock fraction, reserved=0 |
| 64 | 40 B | KF 参数块（见下） |
| 104 | 80 B ×4 | 起窗前 PPWheelKF checkpoint（见下） |
| 424 | 32 B ×4 | 四轮实际 PID：Kp/Ki/Kd/MaxOut/IntegralLimit/Output_LPF_RC/Derivative_LPF_RC 七个 f32＋Improve u32 |
| 552 | u16 ×2 | KF 参数块 CRC16，PID 来源标志（0=未知，1=已读取运行参数） |
| 556 | u32 | dynamic_model_checksum，未安装为 0 |
| 560 | u8 ×32 | manifest 的 build identity SHA256，可无效但不能伪造 |
| 592 | u8 ×32 | 工作区补丁 SHA256，可无效但不能伪造 |
| 624 | u32 | 实际 PID 配置版本 |
| 628 | u8 ×396 | 保留，全部为 0 |

metadata flags：bit0=clock initialized、bit1=KF configured、bit2=checkpoint valid、bit3=build identity available、bit4=patch identity available、bit5=synthetic config。本地构建 identity 的注入来自生成头/manifest，未注入时对应位为 0，不能从当前电脑 HEAD 冒充板上版本。

KF 参数 40 B：相对 offset 0 为四个 u32 `version/max_gap_us/max_age_us/max_horizon_us`；offset 16 为五个 f32 `q/r/initial_p00/initial_p11/gate_squared`；offset 36 为 `max_rejections:u8`，37–39 清零。生成头和运行时以此 canonical 编码计算 CRC，而非对含 padding 的 struct 直接做 CRC。

每轮 checkpoint 80 B：offset 0 为七个 f32 `omega/alpha/p00/p01/p11/innovation/innovation_variance`；28、36 为 `corrected_time_us/output_time_us:u64`；44/48/52 为 `sequence/source_epoch/config_version:u32`；56–59 为 `valid/reason/initialized/seen:u8`；60 为 `seen_time_us:u64`；68/72 为 `seen_sequence/seen_source_epoch:u32`；76 为 `rejected_consecutive:u8`，77–79 清零。它描述 Arm 所在周期计算之前的状态，周期帧描述该周期校正后的状态。离线恢复时先验证 checkpoint 有限性/PSD/版本，再装入生产 C 状态。

### 3.3 end payload：48 B

12 个连续 u32：`raw_count, cycle_count, freeze_reason, rejected_records, outside_window_at_drain, transport_retries_at_drain, start_tick_ms, end_tick_ms, start_cycles, end_cycles, source_epoch, flags`。freeze_reason：1=原始流满、2=周期流满、3=手动、4=配置/来源变化、5=异常；flags bit0=完整冻结、bit1=窗口内有丢失。起止时间和原因在 freeze 时确定，计数在 BeginDrain 时固定，之后不改变 payload。会话层仍可继续累计窗外事件和传输错误。

实现时在 `CSCCapture` 增加与上表对应的起止时间字段，或在冻结时写入 `end_payload` 的固定 offset；两种存储选择均不改变线格式。已编码 metadata/end 属于捕获状态，纳入 map/RAM 预算。主机测试逐块核对末尾 offset 为 512/1024/48，并验证截断/错序/不同 capture_id 混合流不能生成完整实验。

## 4. 计划自检与交付检查表

- [x] 设计 §4.1 的原始/滤波、版本、最终命令、时间来源、并发和丢记录：Task 1–4。
- [x] 设计 §4.2 的源码/工作区/Flash/实验基线：Task 7–8。
- [x] 设计 §4.3 的完整周期、栈/堆、最终门控和超时：Task 3–4、6、8；求解内截止的延后条件已明示。
- [x] 设计 §5 的 F/Q/Joseph、事件水位、异常恢复、低通对照、参数冻结：Task 5–7。
- [x] 设计 §10 的首批相关故障矩阵、指标、配对实验与失败计数：Task 7–8。
- [x] 设计 §11 的五项首批包均有任务；§6–8 的控制改造保留后续阶段，不混入采集/KF 提交。
- [x] 新 API 的时间、单位、轮序和有效性在计划中保持一致；raw_current 与 raw_feedback_current 不混用。
- [x] 软件验证与实车结论分开；不把合成数据、模块轨迹一致或协方差当真实功率/实时性保证。

执行建议：按任务提交，由独立审查者核对接口、测试结果和阶段范围。若执行环境没有头部所列工作流技能，使用当前可用子代理/测试工具完成同等逐任务审查；按用户规则，任何外部技能搜索和安装都须另获明确同意，不自动进行。
