# 单轮力控标定与高速记录工具实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 默认在当前会话逐项执行；本文件编写不代表工具代码已经实现。

**Goal:** 交付独立封装的单轮限流加载、板端高速记录和中文网页操作工具，使用户可以按已有标定方法实测、观察和导出数据。

**Architecture:** 纯 C 状态机与缓存放入 `modules/chassis_force_calibration/`，硬件所有权由独立 DJI 适配器处理，USB 协议负责慢任务命令及冻结数据读取。板级限值和应用装配分别置于 `application/board/`、`application/runtime/`，网页复用现有本地服务和串口封装。

**Tech Stack:** C11、STM32F427、FreeRTOS/CMSIS-RTOS、GNU Make/CMake、主机 GCC、原生 JavaScript、Web Serial、Node 内置测试、现有 Playwright 浏览器验证工具；不新增前端框架或电脑串口库。

日期：2026-10-26。依据 [已确认设计](../specs/2026-10-26-chassis-force-calibration-tools-design.md)，关联 [力控主体计划](2026-10-25-chassis-direct-force-control.md)。主体已实现，本计划只增加标定工具，不重做主体。

## Global Constraints

- 工具独立封装，正常力控配置的三个必测组继续留 0，不能通过设置虚假标定值运行测试。
- 工具不自动写回物理标定参数，不修改 `CHASSIS_FORCE_CALIBRATION_VALID`。
- 继续使用现有唯一 CAN 打包和发送路径，不新增第二处电流发送。
- 首版只支持 ONE_BOARD、OMNI_X、力控后端、台架固件和 USB 遥测开启的组合。
- 四轮只能接受标定状态机的受限输出，普通驾驶请求不参与混合输出。
- 每次测试只能选一个轮子；不自动循环、不自动换轮，也不在重新连接后恢复测试。
- 接收中断只做有界字段复制及缓存追加；不解析字符串、不格式化 CSV、不调用 USB 发送、不动态分配内存。
- 默认记录内存预算不超过 64 KiB；只有输出已归零、记录被冻结后才导出。
- 现有 PID 自动整定和运行时参数写入/Flash 保存与标定会话互斥。
- 不烧录或实际加载电机；工具首次上板的保护、采样率和夹具由后续硬件验收确认。
- 保留现有用户改动，提交时只选择本任务文件或明确的本任务差异，不使用 `git add .`。

## 仓库证据与实施前检查

计划核对基线为 `40eb6ef`。工作区已有 `.Doc/指针查找索引.md`、`application/board/board_chassis_force.c`、`application/chassis/chassis.c`、`docs/configuration-index.md`、`docs/structure-index.md`、`modules/algorithm/chassis_spin_config.h` 的修改，不能覆盖或顺带提交。开始实施时重新执行以下命令，保留当时实际状态：

```powershell
git status --short
git log -5 --oneline
make -C tests/chassis_force test
make -C tests/usb_telemetry test
make -C tests/pid_autotune test
node --test tools/usb-telemetry/tests/*.test.cjs
```

已确认的接入点及必须处理的问题：

| 现有代码 | 实施要求 |
| --- | --- |
| `dji_force_adapter.c` 注册唯一 `DJIMotorOutputGroup_s` | 标定分支必须由这里委托，不得重复注册另一输出组 |
| `ChassisTask()` 因 `!calibrated` 设置能源零力 | 单独构造标定许可，正常分支仍保留原锁定；不能把控制器伪标为 calibrated |
| `DJIMotorDecode()` 滤波后只保留 `speed_aps` | 在解码时保留原始 rpm、电流及接收时刻，并通知记录器 |
| `CANTransmitGuarded()` 在邮箱等待后复查输出 | 标定最终检查沿用此入口，邮箱等待期间失去许可的这一帧应为零 |
| `DWT_GetTimeline_us()` 使用共享时间状态 | ISR 记录不用这个共享累加器；用原始周期计数和本次记录原点计算相对微秒 |
| 原力控总 RAM 账目约 150 KiB，链接还预留堆栈 | 高速缓存放入现有 CCM 段，USB 发送先复制到普通 SRAM；不能直接再占约 60 KiB 普通 SRAM |
| `RuntimeTuningBeginTrial()` 绑定 PID 版本 | 增加独立临时试验所有权接口，不伪造 PID 组版本取得标定锁 |
| USB 慢任务默认 20 ms 执行 | 命令/状态在这里处理，采样在 CAN/电机路径执行；网页不将显示频率当采样率 |

工具解释器从已安装位置读取：`node`、`gcc`、`make` 已在 PATH。当前 CMake 缓存记录的程序为 `C:/Users/20802/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe`。Python 在实施时从已安装工具目录查找，不能使用会跳到 Microsoft Store 的 `python.exe` 别名。查找使用 `Get-Command` 和目标安装目录，不安装新依赖来代替已有工具。

## 文件与职责

| 文件 | 职责 |
| --- | --- |
| `modules/chassis_force_calibration/force_calibration_config.h` | 默认关闭宏和纯配置常量，不包含板级或 RTOS 头 |
| `modules/chassis_force_calibration/force_calibration.[ch]` | 输入/限值/脉冲/状态/原因结构、单轮状态机及最终输出检查 |
| `modules/chassis_force_calibration/force_calibration_capture.[ch]` | 48 字节记录编解码、有限缓存、冻结与定址读取 |
| `modules/motor/DJImotor/dji_force_calibration.[ch]` | 四轮绑定、短临界区、RX 观察、控制/提交事件与停止证明 |
| `modules/telemetry/force_calibration_protocol.[ch]` | `@FC1` 解析、邮箱、状态回执、冻结块读取与校验 |
| `application/board/board_force_calibration.[ch]` | 本车限值及构建组合检查，中文注释与单位 |
| `application/runtime/force_calibration_app.[ch]` | 将安全、操作源、能源预算及辅助电机停止条件装配为标定许可 |
| `tools/usb-telemetry/force-calibration.html`、`force-calibration.css` | 独立中文页面及与已有工具协调的样式 |
| `tools/usb-telemetry/force-calibration-protocol.js` | 逐行协议、整数与 CRC 校验、记录解码 |
| `tools/usb-telemetry/force-calibration-client.js` | 串口会话、请求匹配、心跳、停止和冻结数据读取 |
| `tools/usb-telemetry/force-calibration-report.js` | 原始 CSV/JSON、测量表和增长裕量候选分析 |
| `tools/usb-telemetry/force-calibration-ui.js` | 操作按钮、实时状态、记录曲线和下载 |
| `tests/force_calibration/` | GCC 核心/缓存/协议/真实驱动集成及构建验证 |

既有文件仅改接入点：`dji_motor.[ch]`、`dji_force_adapter.c`、`usb_telemetry_stream.c`、`runtime_tuning.[ch]`、`tuning_flash.c`、`chassis_force_app.c`、`Makefile`、`CMakeLists.txt`、`config/atype-options.txt`、`tools/usb-telemetry/serve.cjs`、现有工具 README/入口链接和相关测试。不把状态机、协议或缓存实现放进 `chassis_force_app.c`。

## 共同接口与固定行为

核心公开类型前缀为 `Fc`，函数前缀为 `Fc`；驱动适配函数为 `DJIForceCalibration*`，USB 协议为 `ForceCalibrationProtocol*`，应用装配为 `ForceCalibrationApp*`。类型在所属任务中定义，后续任务沿用，不能各自另建不兼容副本。

`FcLimits` 字段：`int16_t max_current`，`uint16_t max_on_ms, max_rpm, stationary_rpm, stationary_ms, heartbeat_ms, pre_ms, tail_ms`，`uint8_t stop_temperature_c`。

`FcPulse` 字段：`uint8_t wheel`，`int16_t current`，`uint16_t rise_ms, hold_ms, fall_ms`。`wheel` 是 LF/RF/LB/RB 数组索引 0/1/2/3，不是电调 ID。

`FcInput` 字段：`uint32_t now_us, usb_generation, feedback_seq[4], feedback_age_us[4]`，`int16_t raw_rpm[4]`，`float used_rpm[4], load_w, budget_w`，`uint8_t temperature[4]`，`bool usb_open, bench_permitted, command_fresh, aux_stopped, meter_fresh, budget_valid, zero_submitted`。`now_us` 为模 2^32 微秒时钟，所有持续时间用无符号差计算；快路径最后复查时必须刷新输入。

`FcDrive` 字段：`int16_t current[4]`，`FcState state`，`FcReason reason`，`bool recording`。状态固定为 `IDLE, PREPARING, READY, PRE, RISE, HOLD, FALL, TAIL, FROZEN, ABORTED`。原因固定为 `NONE, STOP, USB, HEARTBEAT, PERMISSION, FEEDBACK, OVERSPEED, TEMPERATURE, POWER, TIMING, CAN, CAPACITY, PROTOCOL`，线上用整数，网页提供中文映射。

`FcStatus` 保存会话、测试、USB 世代、状态/原因、当前命令、预期/已有记录数量、数据完整性、四轮摘要和最后零电流提交证明；从引擎/适配器生成拷贝，不把可修改内部对象暴露给网页任务。

状态枚举在 C 中使用 `FC_IDLE`、`FC_PREPARING` 等 `FC_` 前缀，原因使用 `FC_REASON_NONE`、`FC_REASON_STOP` 等 `FC_REASON_` 前缀。`FcOperation` 和 `FcRequest` 也在核心头定义，操作为 `FC_OP_ARM, FC_OP_START, FC_OP_PING, FC_OP_STOP, FC_OP_CLEAR`；请求字段为 `op, request_id, generation, session, capture_id, pulse`，除 `pulse` 外的 ID 均为 uint32_t。`FcCommandResult` 包含原请求 ID、`FcStatus status` 和 `FcCommandError error`，错误枚举为 `FC_ERROR_NONE, FC_ERROR_BUSY, FC_ERROR_GENERATION, FC_ERROR_SESSION, FC_ERROR_GUARD, FC_ERROR_RANGE, FC_ERROR_CAPTURE, FC_ERROR_OWNERSHIP`，协议负责转换为文本。

默认限值：电流幅值 1500、通电总时长 500 ms、转子 1000 rpm、50 ℃、静止 20 rpm 连续 100 ms、心跳超时 150 ms、预记录 50 ms、尾段 100 ms。网页默认 300 命令、50/100/50 ms，心跳 50 ms；这些均为调试边界，不是物理验收值。

## 任务 1：纯 C 单轮状态机与独立测试限值

**文件：** 创建 `force_calibration_config.h`、`force_calibration.[ch]`、`board_force_calibration.[ch]`、`tests/force_calibration/Makefile`、`test_engine.c`、`test_support.h`。`test_support.h` 只放测试夹具。

**接口：** `FcEngine` 由 `force_calibration.h` 定义，包含上述限值、会话状态、时间基准、静止确认和最后电流；只保存一个测试。公开原型：

```c
void FcInit(FcEngine *engine, const FcLimits *limits);
bool FcArm(FcEngine *engine, const FcInput *input, uint32_t *session);
bool FcStart(FcEngine *engine, uint32_t session,
             const FcPulse *pulse, const FcInput *input);
bool FcHeartbeat(FcEngine *engine, uint32_t session,
                 uint32_t generation, uint32_t now_us);
void FcCancel(FcEngine *engine, FcReason reason, uint32_t now_us);
FcDrive FcStep(FcEngine *engine, const FcInput *input);
bool FcValidateFinal(FcEngine *engine, const FcInput *input,
                     const int16_t current[4]);
void FcRecordSubmission(FcEngine *engine, uint32_t now_us,
                         const int16_t current[4], bool submitted);
void FcGetStatus(const FcEngine *engine, FcStatus *status);
FcLimits BoardForceCalibrationLimits(void);
```

- [ ] **1.1 写核心行为测试并运行红灯。** 测试夹具 `ready_engine()` 先按真实接口生成 100 ms 连续静止输入并取得会话；它不直接写 READY 状态。`permitted_input()` 给出新鲜四轮反馈、在线 USB、台架许可、辅助电机已停止、有效 50 W 预算和 2 W 测量。第一条测试的关键断言如下：

```c
FcEngine e; FcInput in; uint32_t session;
ready_engine(&e, &in, &session);
const FcPulse pulse = {.wheel=1, .current=300,
    .rise_ms=50, .hold_ms=100, .fall_ms=50};
assert(FcStart(&e, session, &pulse, &in));
const uint32_t start = in.now_us;
FcDrive d = advance_ms(&e, &in, session, 75); /* 50 ms 预记录 + 25 ms 上升 */
assert(d.current[0]==0 && d.current[1]==150);
assert(d.current[2]==0 && d.current[3]==0);
d = advance_ms(&e, &in, session, 175); /* 共计 250 ms，预记录 + 完整脉冲 */
assert(in.now_us-start==250000);
assert(d.current[1]==0 && d.state==FC_TAIL);
```

`advance_ms()` 每次增加 1 ms，刷新反馈和许可、每 50 ms 心跳一次，调用真实 `FcStep()` 与成功/失败可控的 `FcRecordSubmission()`，返回最后一次输出；不能一次跳过 75 ms 而绕过周期检查。预期先因为缺少实现不能构建；补齐空接口后必须看到断言失败，再编写状态机。执行：`make -C tests/force_calibration test_engine`。

- [ ] **1.2 实现最小状态机，通过单轮/正负/时序测试。** 输入越界、非法轮号、零电流、零上升/保持/下降段或总通电超过 500 ms 均拒绝；整段电流函数以本次开始时间计算，不按假定固定周期累加。时钟基准未建立时保持零输出；每次实际周期必须不超过 5000 us。
- [ ] **1.3 逐项补红灯再实现终止。** 表驱动改变 USB 世代/开关、台架许可、操作源年龄、辅助停止证明、反馈年龄 5001 us、温度 50 ℃、转速 1000 rpm、心跳 150001 us、CAN 失败；均立即归零。预算无效/降低、实测超预算、任意转速/功率 NaN 或无穷也终止，不等待下降斜率。验证四轮任何非选定输出、过大输出、过期输入或已到脉冲截止时刻仍非零的命令在 `FcValidateFinal()` 被拒绝。
- [ ] **1.4 验证尾段与重新开始。** 正常结束记录 100 ms 尾段；异常归零后尽可能记录尾段，原因不可被后续错误覆盖。没有成功提交零电流时不能标为可导出。冻结/异常不会自动回 READY，旧会话/旧世代不能启动；下一次显式 ARM 重新确认静止，再显式 START。时钟、会话和反馈序号回绕各写测试。
- [ ] **1.5 验证板级限值与锁定独立。** 通过实际板级配置函数取得限值，拒绝非法限值；普通 `BoardChassisForceConfig()` 三组待测项仍为 0，`VALID=1` 时仍无法驱动正常力控。运行 `make -C tests/chassis_force test`。
- [ ] **1.6 提交本任务。** 只暂存上述新文件及测试，提交 `feat: add bounded single-wheel calibration engine`，记录实际哈希。

## 任务 2：原始事件记录、CCM 缓存与冻结读取

**文件：** 创建 `force_calibration_capture.[ch]`、`tests/force_calibration/test_capture.c`；在任务 1 的状态机增加容量终止调用。实际 CCM 存储对象由任务 3 的适配器持有。

**接口：** `FcCapture` 为调用方传入存储区的描述对象，纯 C 模块不自行分配 64 KiB 静态全局。`FcRecord` 以语义字段表示事件，编码为固定 48 字节小端线格式；不传输编译器结构体内存布局。

```c
void FcCaptureInit(FcCapture *capture, uint8_t *storage, uint16_t capacity);
void FcCaptureBegin(FcCapture *capture, const FcCaptureMeta *metadata);
bool FcCaptureAppend(FcCapture *capture, const FcRecord *record);
void FcCaptureFreeze(FcCapture *capture, FcReason reason, bool complete);
bool FcCaptureRead(const FcCapture *capture, uint16_t first,
                    uint8_t count, uint8_t *bytes, size_t capacity);
bool FcEncodeRecord(const FcRecord *record, uint8_t bytes[48]);
```

| 字节偏移 | 类型/内容 |
| --- | --- |
| 0/1/2/3 | u8 事件类型 RX=1、CONTROL=2、MARKER=3 / 轮号 / 状态 / 标志 |
| 4 | u32 反馈序号或控制周期编号 |
| 8 | u32 本次记录开始后的事件时刻/us |
| 12 | i32 本周期使用的反馈接收时刻/us，相对记录原点；开始前的反馈允许为负 |
| 16/18 | i16 原始转子 rpm / 原始反馈电流 |
| 20 | float32 本次使用的滤波后 rpm |
| 24 | i16 单轮电流请求 |
| 26、28、30、32 | i16 四轮最终提交电流，固定 LF/RF/LB/RB |
| 34/35 | u8 温度/启用掩码 |
| 36 | u32 实际控制周期/us |
| 40/42 | u16 准备至最终复查耗时 / 最终复查至提交返回耗时/us |
| 44/46/47 | u16 原因 / u8 尝试提交组掩码 / u8 成功提交组掩码 |

RX 不适用的字段为 0；CONTROL 同时保留当次使用的反馈、准备时刻派生时差及最终结果，不能用提交之后更新的反馈覆盖当次快照。时差超过 u16 范围必须标记溢出/质量失败，不能静默截断。

`FcCaptureMeta` 在缓存头定义，包含 `uint32_t capture_id, session, generation`、`FcPulse pulse`、`FcLimits limits` 和滤波设置。它在成功 START 时复制，此后冻结记录的身份不随新 ARM 或 USB 重连改变；CAP 中当前连接 generation 与 DATA 中冻结记录 session/capture_id 必须分开验证。

- [ ] **2.1 红灯验证原始编码。** 使用人工固定值（负转速 -123、负电流 -300、时间戳 `0x01020304`、滤波值 12.5）断言确切字节，包括 `04 03 02 01`、`85 ff`、`d4 fe`、float32 `00 00 48 41`；反例改一个字节应被 JS/协议侧解码测试识别。命令：`make -C tests/force_calibration test_capture`。

```c
/* FcRecord 语义字段名与网页对象一致，不使用 packed 结构体发送。 */
FcRecord record = {.type=1, .time_us=0x01020304,
    .raw_rpm=-123, .raw_current=-300, .used_rpm=12.5f};
uint8_t bytes[48]; assert(FcEncodeRecord(&record,bytes));
assert(bytes[8]==4 && bytes[9]==3 && bytes[10]==2 && bytes[11]==1);
assert(bytes[16]==0x85 && bytes[17]==0xff);
assert(bytes[18]==0xd4 && bytes[19]==0xfe);
assert(bytes[20]==0 && bytes[21]==0 && bytes[22]==0x48 && bytes[23]==0x41);
```

- [ ] **2.2 实现编码与有界追加。** 默认 1280 条 × 48 字节 = 61440 字节；状态元数据与缓存合计必须小于等于 65536 字节，用 `_Static_assert` 验证。每次先验证空间再写整条；冻结后拒绝写入，越界读取拒绝，无动态分配。
- [ ] **2.3 红灯验证容量与历史保留。** 用 4 条小缓存测试第 5 条追加失败、既有字节不变、完整性为 false；重复块读取字节一致，未冻结时不可导出。新的 START 只有参数与许可全部通过后才清掉旧记录；失败的 START 不丢失历史。
- [ ] **2.4 实现容量控制。** 预记录 50 ms、最大加载 500 ms、尾段 100 ms；一次仅记录所选轮 RX 和每个控制周期一次 CONTROL。保留至少 8 个终止/零提交标记槽，在可用槽接近下限时原子置容量取消标志；任务/最终复查看到标志后归零，已有数据完整性为 false。硬满时后续事件计入丢弃计数，不能覆盖旧记录或等待腾空。
- [ ] **2.5 频率与质量统计。** 从时间戳计算实际 RX/控制间隔、反馈序号间隙和剩余空间；理论容量可按 `1280/(RX每秒+CONTROL每秒)` 显示，但不得保证 1 kHz 或固定可记录秒数。计数器回绕、缺反馈、同序号重复、尾段不足都须留下质量信息。
- [ ] **2.6 提交。** 验证核心与缓存测试通过，提交 `feat: capture calibration feedback and final control events`。

## 任务 3：四轮驱动接入、许可与能源预算

**文件：** 创建 `dji_force_calibration.[ch]`、`force_calibration_app.[ch]`、`tests/force_calibration/test_adapter.c`、`test_power_permission.c`；修改 `dji_motor.[ch]`、`dji_force_adapter.c`、`chassis_force_app.c`、`runtime_tuning.[ch]`、`tuning_flash.c`。

**接口：** `FcPermit` 由 `dji_force_calibration.h` 定义，包含现有 `ChassisPowerControlSnapshot_s power`、命令接收时刻、操作源年龄、yaw 年龄、安全 epoch、台架中位许可和辅助电机停止证明。它与 `ChassisForceActuationSnapshot_s` 分开，不改普通力控的 calibrated 状态。

```c
bool DJIForceCalibrationInit(DJIMotorInstance *const motors[4],
    const FcLimits *limits, void (*read_link)(bool *open, uint32_t *generation));
void DJIForceCalibrationPublish(const FcPermit *permit);
void DJIForceCalibrationReadControl(void);
void DJIForceCalibrationPrepare(const DJIMotorCycle_s *cycle);
bool DJIForceCalibrationValidate(const DJIMotorCycle_s *cycle,
    const int16_t current[DJI_MOTOR_CNT], const bool enabled[DJI_MOTOR_CNT],
    uint32_t now_ms);
void DJIForceCalibrationRecord(const DJIMotorCycle_s *cycle,
    const int16_t current[DJI_MOTOR_CNT], uint8_t attempted, uint8_t successful);
void DJIForceCalibrationFeedback(const DJIMotorInstance *motor,
    int16_t raw_rpm, int16_t raw_current, uint32_t rx_cycles);
void DJIForceCalibrationCancel(FcReason reason);
void DJIForceCalibrationGetStatus(FcStatus *status);
bool DJIForceCalibrationSubmit(const FcRequest *request);
bool DJIForceCalibrationReply(FcCommandResult *reply);
void DJIForceCalibrationReplyAccepted(uint32_t request_id);
bool DJIForceCalibrationRead(uint32_t capture_id, uint16_t first,
                             uint8_t count, uint8_t *bytes, size_t capacity);
bool ForceCalibrationAppPermit(const ChassisForceIntentReceiver_s *command,
                               uint32_t now_ms);
void ForceCalibrationAppPublish(const ChassisForceIntentReceiver_s *command,
    const ChassisPowerControlSnapshot_s *power, uint32_t now_ms);
bool RuntimeTuningBeginCalibration(void);
void RuntimeTuningEndCalibration(void);
```

引擎和命令邮箱由电机适配器持有；ARM/START/PING/CLEAR 经 `Submit()` 入队，在 `Prepare()` 中处理，不让慢任务直接改引擎。结果由 `Reply()` 拷贝，USB 接受后才调用 `ReplyAccepted()` 释放；停止/USB 世代变化/缓存满使用单独取消标志，最终复查必须看到最新值。这样任务 3 可以通过真实 Submit/Reply 接口独立测试，不依赖任务 4 尚未实现的协议代码。

- [ ] **3.1 用真实驱动测试支架写红灯。** 复用 `tests/chassis_force/test_adapter.c` 中已有 HAL/CAN 支架，实际执行 `DJIMotorControl()`。普通配置仍未标定；标定输入允许时只 RF 非零、四轮 PID 调用为 0，四轮 CAN ID 1/4/2/3 的字节与最终记录一致；第五个辅助电机保持停用。不存在真实第二次发送调用。

```c
/* 已通过公开 Submit 接口开始 RF 测试并推进到保持段后检查。 */
test_pid_calls=0; DJIMotorControl(); assert(test_pid_calls==0);
int16_t packed[4];
for(unsigned w=0;w<4;++w) {
    const unsigned slot=adapter_motors[w]->message_num;
    packed[w]=(int16_t)((uint16_t)sender_assignment[1].tx_buff[2*slot]<<8 |
                        sender_assignment[1].tx_buff[2*slot+1]);
}
assert(packed[0]==0 && packed[1]==300 && packed[2]==0 && packed[3]==0);
```

- [ ] **3.2 实现编译分支与所有权委托。** `ENABLE=1` 时 `dji_force_adapter.c` 的 read/prepare/validate/record 四钩子分别委托独立适配器，`Publish()` 接收独立许可并设定标定所需 stop 状态。`ENABLE=0` 完全保留原行为。普通驾驶请求不能进入标定电流路径，空闲/异常/退出会话归零。
- [ ] **3.3 加入原始 RX 观察。** `dji_motor.[ch]` 在标定编译分支提供 `bool DJIMotorRegisterFeedbackObserver(void (*observer)(const DJIMotorInstance *, int16_t, int16_t, uint32_t))`，只注册一次，参数依次是电机、原始 rpm、原始电流、接收周期。解码时在低通前取得原始 rpm/电流，完成原有滤波和序号更新后通知记录器。仅接纳所选电机；保留当前控制快照所用的原始值/接收周期以关联 CONTROL，不能在控制完成时取最新 ISR 数据冒充旧快照。
- [ ] **3.4 实现统一相对时钟与短临界区。** 会话记录开始时保存 `DWT_GetCycleCount()` 原点，记录事件以无符号周期差除以 `DWT_GetFrequencyHz()/1000000` 换算相对 us，检查频率非零且记录期限短于一个周期计数回绕。RX/控制的槽位分配、完整写入和取消标志使用保存/恢复 PRIMASK 的短临界区，不修改原始 DWT 共享累计器，不在锁中格式化或遍历整缓存。
- [ ] **3.5 接入 CCM。** 仅标定固件定义 `static uint8_t storage[1280][48] __attribute__((section(".ccmram"), aligned(4)));`；CCM 不作为 USB 端点发送缓冲。初始化只清有效数量/元数据，读取前验证所有槽已经写满；不依赖启动代码自动初始化该段。核对链接 map 的 CCM 容量、普通 RAM、堆栈预留和镜像大小，禁止为塞下缓存缩减原任务栈/堆。
- [ ] **3.6 单独装配标定许可。** `ForceCalibrationAppPermit()` 保留新鲜命令、源/yaw 累计 10 ms、`VehicleSafetyAllowsAutotune()`、当前安全 epoch 和辅助电机静止证明；`force_calibration_app.c` 负责遍历辅助电机状态，chassis 只调用。读 USB 端口/世代的快照通过适配器初始化时注入的回调取得，最终复查重新读取，不能只相信慢任务 20 ms 前的缓存。
- [ ] **3.7 红灯覆盖预算引导与掉额。** 现有 `!calibrated` 会把 `policy_input.zero_force` 置真；仅 `ENABLE=1` 的装配分支改用独立标定许可，普通分支保持原式。标定分支关闭自适应申请与正向模型学习，只使用已批准的基础能源预算，附加 boost/offer 为零；现有台架功率宏默认 50 W，最终标定预算取该宏与有效裁判/政策限额的较小者。要求负载功率计有效且不超过 20 ms，政策快照不超过 10 ms；实际功率超预算、预算下降、裁判禁用或电容确认失效时撤力。不得用合成 `ChassisForceCycleOutput_s.valid/calibrated` 去触发旧增补流程。
- [ ] **3.8 红灯覆盖发送前变化。** 在 CAN 邮箱等待支架中注入急停、USB 关闭/世代变更或取消标志，当前帧四轮字节都应归零。CAN 返回失败时记录尝试命令与失败结果，不将其计为已生效；至少一帧零电流成功提交之前，状态不能允许导出。提交成功表示进入发送邮箱，不声称电调已经收到/执行。
- [ ] **3.9 实现试验与 Flash 互斥。** 用临时试验 owner 区分 PID 和标定；`RuntimeTuningBeginCalibration()` 要求无 PID 试调、无 QUEUED/APPLYING 控制修改和无进行中的 Flash 操作。校准会话期间阻止 SET/SAVE 及 Flash poll，GET 仍可读；原 PID `BeginTrial/EndTrial` 不能释放标定 owner。准备失败和所有退出路径释放本会话锁，已冻结数据保留不占驱动会话。
- [ ] **3.10 回归并提交。** 运行 `tests/force_calibration`、`tests/chassis_force`、`tests/dji_chassis_power_rtt`、`tests/pid_autotune`、`tests/tuning_flash`；提交 `feat: integrate isolated calibration with motor and power permissions`。此提交不能宣称网页或 USB 命令已经可用。

## 任务 4：USB 命令、取消优先和冻结数据协议

**文件：** 创建 `force_calibration_protocol.[ch]`、`tests/force_calibration/test_protocol.c`；修改 `usb_telemetry_stream.c` 和现有 `tests/usb_telemetry` 支架，接通任务 3 的命令消费点。

**接口：** 沿用任务 1 的 `FcRequest` 和任务 3 的适配器邮箱。CAP/STATUS/READ 是慢任务只读操作；ARM/START/PING/STOP/CLEAR 调用 `DJIForceCalibrationSubmit()`，结果从 `DJIForceCalibrationReply()` 取得。邮箱仅一条普通待执行命令，STOP/取消另存，不受已占用邮箱阻塞。

```c
void ForceCalibrationProtocolInit(void);
void ForceCalibrationProtocolSubmit(const char *line, uint32_t generation);
size_t ForceCalibrationProtocolReply(char *buffer, size_t capacity);
void ForceCalibrationProtocolReplySent(void);
void ForceCalibrationProtocolCancel(uint32_t generation, FcReason reason);
```

`Submit/Reply/ReplySent` 在 USB 慢任务调用；最终控制许可仍由电机任务检查。共享状态只在短临界区内复制。协议通过任务 3 的 `DJIForceCalibrationGetStatus()` 获取状态，READ 通过 `DJIForceCalibrationRead()` 读取冻结数据。协议回执成功提交 USB 后才释放相同 request_id 的适配器结果。

线协议采用十进制整数、ASCII、CRLF；所有请求带请求 ID，除 CAP 外都带当前 generation。STATE/REASON 按前述枚举顺序从 0 编号；布尔值只接受 0/1。每个请求严格检查字段数量，拒绝符号异常、溢出、空字段、NaN 文本和尾随字段。

```text
@FC1,id,CAP
@FC1,id,STATUS,generation
@FC1,id,ARM,generation
@FC1,id,START,generation,session,wheel,current,rise_ms,hold_ms,fall_ms
@FC1,id,PING,generation,session
@FC1,id,STOP,generation,session
@FC1,id,CLEAR,generation,capture_id
@FC1,id,READ,generation,capture_id,first,count
```

CAP 返回字段固定为：

```text
@FC1,id,CAP,generation,enabled,format,record_bytes,record_capacity,max_current,max_on_ms,max_rpm,stop_temperature_c,stationary_rpm,stationary_ms,heartbeat_ms,pre_ms,tail_ms,source_mode,power_stage,bench_mode,normal_calibrated,lf_id,rf_id,lb_id,rb_id,speed_lpf_pm,current_lpf_pm
```

STATUS、ARM、START、PING、STOP、CLEAR 的成功回复统一为：

```text
@FC1,id,OK,generation,session,capture_id,capture_session,capture_generation,state,reason,records,complete,zero_submitted,requested,final_lf,final_rf,final_lb,final_rb,raw_rpm,used_rpm_milli,temperature,feedback_age_us,load_mw,budget_mw
@FC1,id,ERR,generation,error
@FC1,id,DATA,generation,session,capture_id,first,count,total,hex,crc16
```

`error` 只使用 `disabled,format,busy,generation,session,guard,range,not_frozen,not_zero,capture,index,ownership`。只有 PING 续租；STATUS/CAP/READ 不续租、不申请输出。STOP 的 session=0 表示停止当前连接的任何标定会话；generation 必须匹配。旧 START/ARM 不得在重连后落入新会话。

`generation/session` 表示当前连接/会话，`capture_session/capture_generation` 表示冻结数据产生时的身份；`FcStatus` 增加这两个元数据字段。重连后 READ 请求使用当前 generation，返回 DATA 的 session 必须匹配 capture_session，不能错误要求等于新 ARM 的 session。

每块最多 2 条记录，即最多 96 字节二进制、192 个十六进制字符；整行必须小于现有 512 字节命令/回执容量。`crc16` 是四位十六进制，计算范围为从 `@FC1` 到 `hex` 末字符的 ASCII 字节，不包括最后的逗号、CRC 和 CRLF；算法沿用工程已有 CRC16：初值 0xffff、反射多项式 0xa001。格式版本来自 CAP 的 format=1，记录类型在每条字节 0 中区分。

- [ ] **4.1 写红灯协议用例。** `@FC1,7,START,9,3,1,-300,50,100,50` 解码为 RF/负 300/200 ms；多一字段、wheel=4、current=1501、`4294967296`、NaN、空字段均拒绝。旧 generation、错误 session 和普通邮箱忙返回明确错误，不能产生电机动作。执行 `make -C tests/force_calibration test_protocol`。

```c
ForceCalibrationProtocolInit();
ForceCalibrationProtocolSubmit("@FC1,7,START,9,3,1,4294967296,50,100,50",9);
char reply[512];
assert(ForceCalibrationProtocolReply(reply,sizeof reply)>0);
assert(strcmp(reply,"@FC1,7,ERR,9,format\r\n")==0);
/* 十进制字段超出解析整数范围，不能截断为合法电流命令。 */
```

- [ ] **4.2 接入慢任务分发。** 在 `ReadCommands()` 识别 `@FC1,`，其余既有 `@A1/@A2/@T1` 路径保持兼容；标定编译关闭时 CAP 回复 enabled=0，变更命令回复 disabled，不引入缓存。新协议回复拥有独立已发送确认，不消耗或误清旧协议回执。
- [ ] **4.3 测试取消与忙重试。** 先排队 START，再提交同 generation 的 STOP，输出仍为零，START 不得延后重放。USB 关闭/世代变化、RX 溢出、丢弃超长/含 NUL 的 `@FC1` 命令都置取消标志；仅保留旧数据。端点忙时 `Reply()` 重复得到相同内容，只有 USB 接受后 `ReplySent()` 才释放；不能在端点仍占用数组时改写回执缓冲。
- [ ] **4.4 检查导出门槛。** READ 仅在缓存冻结且最终零电流已成功提交时成功；不能把“stop_flag 为 STOP”当作成功提交证明。按固定 capture_id、索引读取，允许重读；错误 CRC、错误索引或不同 capture 的块在电脑端拒绝。固件不因读取或重连恢复驱动。
- [ ] **4.5 完成集成并提交。** 验证 `make -C tests/force_calibration test`、`make -C tests/usb_telemetry test`、`make -C tests/pid_autotune test`；提交 `feat: expose calibration control and frozen capture over USB`。

## 任务 5：网页协议、数据分析和可导出报告

**文件：** 创建 `force-calibration-protocol.js`、`force-calibration-client.js`、`force-calibration-report.js`、`tools/usb-telemetry/tests/force-calibration.test.cjs`；复用已有 `serial.js` 的 `TelemetrySerial.SerialSession`，不改其公共接口。

**接口：** 与现有脚本一致，浏览器导出全局对象，Node 导出 `module.exports`。以下 API 的参数/返回结构固定，UI 只调用这些接口：

```javascript
// ForceCalibrationProtocol
createParser({onReply, onError}); // 返回 {push(bytes), reset()}
decodeRecord(bytes);             // bytes.length=48，返回带单位的原始事件对象
// ForceCalibrationClient
new Client({serialSession, onStatus, onError});
client.capabilities();           // Promise<Caps>，只读取
client.arm();                    // Promise<Status>，显式调用
client.start({wheel,current,rise_ms,hold_ms,fall_ms}); // Promise<Status>
client.stop();                   // Promise<Status>，优先级最高
client.readCapture();            // Promise<{caps,status,records,quality}>
client.clear();                  // Promise<Status>，明确清除冻结数据
client.disconnect();             // 停止本地任务，尽力 STOP 后关闭端口
// ForceCalibrationReport
analyzeGrowth({records,window_us,margin_rpm,max_gap_us});
// -> {valid,max_growth_rpm,candidate_rpm,windows,excluded,issues}
torqueRows({measurements}); // -> 含方向、质量/力、力臂、电流和误差的表
toCsv(report);              // -> string，包含原始事件和单位列名
toJson(report);             // -> string，原始值与元数据，无自动参数采用
```

`Caps/Status` 对应任务 4 固定字段，client 校验 format/record_bytes/轮 ID、真实限值和权限。`records` 的对象字段与任务 2 的表一一对应；缺失值为 null/质量错误，不用 0 冒充。时序分析全部以记录的相对 us 为准，不使用浏览器收到数据的时间。

- [ ] **5.1 独立字节/协议红灯。** 用手写 48 字节固定向量验证负数、float32、四轮顺序和字段单位；用完整 DATA 行的固定 CRC 断言，通过任意字节分片输入 parser。CRC 算法另用 ASCII `123456789` 的独立已知结果 0x4b37 验证；不使用被测编码器来生成同一个测试的预期结果。
- [ ] **5.2 会话与心跳红灯。** 串口测试替身仅替代硬件字节流，运行真实 client/parser：连接不 ARM、不 START；只有明确点击/调用才开始；50 ms PING，不并发堆积；START 超时返回结果未确认，不重试 START。STOP 取消未发送的普通请求并优先发送，断开后停止心跳、清除未确认请求，重新连接只 CAP/STATUS；迟到旧 generation/session/id 的回执不完成新请求。
- [ ] **5.3 实现冻结数据读取。** 只在状态允许时按 2 条一块顺序读取；每块核对 capture/session、first/count/total、CRC 和返回字节数。READ 超时可重读相同索引，重复数据不追加两次；未读完只允许导出明确标注不完整的原始报告，不给标定完成结论。
- [ ] **5.4 红灯验证增长分析。** 手工记录：CONTROL 在 t=0 使用 100 rpm，RX 在 5 ms 为 140、10 ms 为 180，窗口 10 ms、额外裕量 20，候选应为 100 rpm；覆盖负转速、反转过零、重复序号、时间无序、5 ms 中间缺口、尾部不满窗口。分析用绝对转速，排除窗口边界之外的数据；无足够窗口返回 valid=false，不能返回“0 rpm 已标定”。

```javascript
const result = R.analyzeGrowth({records: [
  {type:'CONTROL',time_us:0,used_rpm:100,cycle_seq:1},
  {type:'RX',time_us:0,raw_rpm:100,feedback_seq:1},
  {type:'RX',time_us:5000,raw_rpm:140,feedback_seq:2},
  {type:'RX',time_us:10000,raw_rpm:180,feedback_seq:3}
],window_us:10000,margin_rpm:20,max_gap_us:5000});
assert.equal(result.valid,true);
assert.equal(result.max_growth_rpm,80);
assert.equal(result.candidate_rpm,100);
```

- [ ] **5.5 记录质量与计算范围。** 实际样本间隔、反馈序号缺口、容量终止、未成功提交命令、时间饱和都在报告出现。`max_gap_us` 是分析输入，不能用插值弥补；候选明确绑定所选轮、测试电流/转速/电压观察范围及 T。多份四轮记录导入后取有效候选的共同包络，缺一轮时不标记四轮完成。
- [ ] **5.6 测量表与导出。** 质量含容器/挂件，力臂用 m；示例 `m=0.5,L=0.1` 得 0.4905 N·m，CAN=8192 得 10 A 指令等效值。用户输入上下阈值、重复测量和方向后生成待复核表；摩擦/误差大的结果标注，不能由轮速自动推算 K_out。JSON 包含 CAP、脉冲、原始记录、完整性、失败原因、T、裕量、用户输入条件、源码提交号（未填写则 null）及未知电压 null，导出不调用 `/api/tuning` 或任何参数写入。
- [ ] **5.7 执行并提交。** `node --test tools/usb-telemetry/tests/force-calibration.test.cjs` 后运行全部既有 `*.test.cjs`；提交 `feat: decode and analyze force calibration captures`。

## 任务 6：独立中文页面与可运行使用入口

**文件：** 创建 `force-calibration.html`、`force-calibration.css`、`force-calibration-ui.js`、`tools/usb-telemetry/tests/force-calibration-check.js`；修改 `serve.cjs`、`index.html` 的入口链接、`README.md` 和服务测试文件。

**接口：** 页面通过任务 5 的三个模块和现有 `SerialSession` 工作；本地服务增加精确允许的 `/force-calibration.html`，保持只监听 localhost。页面样式复用既有工具的色彩/按钮/卡片约定，避免引入第二套应用框架。

- [ ] **6.1 页面启动与能力检查测试。** 使用当前可用 UI/Playwright 技能说明进行浏览器验证。入口是 `node tools/usb-telemetry/serve.cjs` 后访问 `http://127.0.0.1:4173/force-calibration.html`；无 Web Serial、串口被其他页面占用、旧固件和 disabled 能力各显示明确中文原因。用服务测试验证精确 HTML/JS/CSS 路径 200，任意新 HTML、路径穿越和现有 `/api/tuning` 同源约束不被放宽。
- [ ] **6.2 写模拟串口操作检查后实现页面。** 布局顺序：连接与固件状态、准备条件/真实限值、选轮和脉冲输入、手动准备/开始/停止、实时读数、冻结记录曲线、测量条件/窗口/导出。开始前明确显示轮名与 CAN ID、命令及等效 A、持续时间和夹具确认，按钮不能仅凭已连接启用。

```javascript
const check=(ok,message)=>{if(!ok)throw new Error(message);};
await page.goto('http://127.0.0.1:4176/force-calibration.html');
check(await page.locator('#fc-start').isDisabled(),'连接前禁止开始');
// 串口替身预先以 addInitScript 注入，window.__fcCommands 记录真实 client 发出的行。
await page.locator('#fc-connect').click();
await page.waitForFunction(()=>document.querySelector('#fc-status').textContent.includes('已连接'));
check(await page.evaluate(()=>!window.__fcCommands.some(x=>x.includes(',START,'))),
      '连接不能启动电机');
```

- [ ] **6.3 显示实时监测与原始数据区别。** 测试中显示所选轮、请求和最终电流、原始/滤波转速、温度、负载功率、状态与停止原因；统计来自 STATUS 的读数标注监测更新率。记录读取完成后，曲线横轴使用板端相对时间，分别显示原始 rpm、控制使用 rpm、请求/最终电流和失败提交标记，能缩放/查看原始点；曲线不能由网页收包时间重采样制造高速细节。
- [ ] **6.4 停止和重连浏览器检查。** 覆盖 RUNNING 时按停止、断连、重新连接、页面刷新、隐到后台和关闭页面；本地尽力发 STOP，但板端心跳/端口许可是最终保护。重连不 ARM/START，旧数据可手动读取。选择新轮/改脉冲、清数据和开始下一次均需明确操作；不能存在自动换轮/自动重试 START 的代码路径。
- [ ] **6.5 导出与离线检查。** 下载实际 CSV/JSON 并验证内容和完整性；历史文件导入仅分析，不能向串口发送 START。离线示例用明显 synthetic 标志，不能采用参数、不能解除锁定。记录不完整时可保存证据但标定计算结论禁用。T 和额外裕量初始为空，不能默认套用文档演示的 10 ms/300 rpm。
- [ ] **6.6 可读性检查。** 浏览器测试记录 console 错误、键盘可达、运行时始终可见的停止按钮、390 px 和桌面截图、无横向溢出、图表单位和颜色以外的状态文字。模拟串口测试不能枚举/打开真实板端口，不实发电流指令。
- [ ] **6.7 提交。** 页面、服务和下载测试通过，提交 `feat: add standalone Chinese force calibration page`。

## 任务 7：构建隔离、整体回归与交付说明

**文件：** 修改 `Makefile`、`CMakeLists.txt`、`config/atype-options.txt`、`board_force_calibration.h`、`tests/architecture/test_layers.py`；创建 `tests/force_calibration/test_build_contracts.py`、`docs/superpowers/verification/2026-10-26-chassis-force-calibration-tools.md`；更新 `docs/chassis-direct-force.md`、`docs/chassis-force-calibration.md`、工具 README。

- [ ] **7.1 默认关闭与宏校验。** `CHASSIS_FORCE_CALIBRATION_ENABLE` 只接受 0/1。Make/CMake 共用选项清单；ENABLE=0 不编入标定引擎/适配器/缓存和 ARM/START 实现，USB 只提供轻量 disabled 回复。ENABLE=1 必须同时满足 ONE_BOARD、OMNI_X、MODE=1、STAGE=3、ATYPE_BENCH_MODE=1、USB=1、POWER_METER_ENABLE=1；标定编译分支不运行普通自旋逻辑，交付构建命令统一指定 SPIN=0。非法组合包括 ENABLE=2、速控后端、双板角色、非 OMNI_X、关闭台架/USB/功率计及 STAGE!=3，逐项通过真实交叉编译器验证失败原因。
- [ ] **7.2 在实际构建清单加入隔离。** Make 不只加源码，还加纯模块 include 路径和数值源 `-fno-fast-math`；CMake 的递归 GLOB 必须显式在默认分支排除新模块、board/runtime/adapter 文件，防止 disabled 固件带入大缓存。任务 1～6 的测试 Makefile 在各自任务内创建，最终生产构建清单在本任务统一验收。
- [ ] **7.3 运行主机回归。** 执行以下命令，遇到失败先按调试技能定位原因，不能通过放宽断言或删测试交付：

```powershell
make -C tests/force_calibration -B test
make -C tests/chassis_force -B test
make -C tests/dji_chassis_power_rtt -B test
make -C tests/usb_telemetry -B test
make -C tests/pid_autotune -B test
make -C tests/tuning_flash -B test
node --test tools/usb-telemetry/tests/*.test.cjs
```

纯 C 新测试再以 `-Ofast -fno-fast-math` 运行一次，确认 NaN/有限数及边界行为不因 Release 优化消失。架构扫描将 `modules/chassis_force_calibration/` 纳入纯 C 范围，并用临时反例验证能抓到 RTOS/硬件/application 反向依赖。

- [ ] **7.4 独立目录构建。** 从下述 Make 命令开始，不共用已有 `.d` 和对象文件；每条都在完成前检查退出码：

```powershell
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/fc-tool-speed CHASSIS_CONTROL_MODE=0
make -j4 PROFILE=atype-direct BUILD_DIR=build/fc-tool-normal-force CHASSIS_CONTROL_MODE=1
make -j4 PROFILE=atype-bench BUILD_DIR=build/fc-tool-bench CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_SPIN_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=1 CHASSIS_FORCE_CALIBRATION_VALID=0 POWER_METER_ENABLE=1 USB_TELEMETRY_ENABLE=1
make -j4 PROFILE=atype-bench BUILD_DIR=build/fc-tool-cap CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_SPIN_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=1 CHASSIS_POWER_SOURCE_MODE=1 CHASSIS_BUFFER_POWER_ENABLE=1 POWER_METER_ENABLE=1 USB_TELEMETRY_ENABLE=1
```

使用已安装 CMake 的实际绝对路径执行同样的 3 个后端，至少包括 Release 标定版本：

```powershell
$fcCmake = 'C:/Users/20802/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe'
& $fcCmake -S . -B build/cmake-fc-speed -G "MinGW Makefiles" -DCHASSIS_CONTROL_MODE=0
& $fcCmake --build build/cmake-fc-speed -j4
& $fcCmake -S . -B build/cmake-fc-normal -G "MinGW Makefiles" -DATYPE_PROFILE=atype-direct -DCHASSIS_CONTROL_MODE=1
& $fcCmake --build build/cmake-fc-normal -j4
& $fcCmake -S . -B build/cmake-fc-bench -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DATYPE_PROFILE=atype-bench -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_SPIN_MODE=0 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -DCHASSIS_FORCE_CALIBRATION_VALID=0 -DPOWER_METER_ENABLE=1 -DUSB_TELEMETRY_ENABLE=1
& $fcCmake --build build/cmake-fc-bench -j4
```

- [ ] **7.5 核对产物与内存。** 从实际 `compile_commands.json`、ELF 和 map 检查：默认两个后端不含标定存储/驱动入口；标定只保留一条 CAN 路径、无轮速 PID 替代；新数值源最后的选项为 `-fno-fast-math`；CCM 缓存约 60 KiB、整个采集对象不超过 64 KiB，USB 缓冲位于普通 RAM，现有堆/栈预留未缩减。标定功能关闭后不能仅靠链接 gc 的偶然结果隐藏编译分支错误。
- [ ] **7.6 写实际工具使用步骤。** 将旧文档“工具尚未提供”替换为真实构建命令、网页地址、遥控器准备、选轮/脉冲、停止、读取和导出步骤；实测参数仍为 0。写明 1500/500 ms 只覆盖低幅短时测试，实际器材稳定时间不够时先改进测量装置，不能延长加载绕过限制。辅助电机、功率计/裁判条件不满足时按明确故障提示处理，不建议关闭保护。
- [ ] **7.7 记录首次上板验收项。** 明确区分已通过的软件检查和待测硬件项：急停与端口关闭响应、最终零提交、真实采样间隔/漏帧、ISR 与控制耗时、任务栈余量、负载功率测量、CCM/USB 行为以及夹具。首次使用先最小电流、短脉冲，确认方向和归零；不自动开展实机验收。
- [ ] **7.8 独立审查与提交。** 使用代码审查技能，提供实际范围和已通过测试，重点审查默认关闭、最终发送保护、锁/中断、内存和协议恢复；修正重要问题后再完成对应回归。最终文档列出每项实际结果、截图/报告位置和提交号；只提交本次相关改动，报告最后提交编号。

## 完成判据与执行交接

| 要求 | 对应任务 | 可检查的交付 |
| --- | --- | --- |
| 单轮正负脉冲、所有终止条件、正常力控仍锁定 | 1、3 | 引擎和真实 CAN 字节测试 |
| 每帧原始反馈与最终控制记录、有限缓存 | 2、3 | 固定字节向量、容量/冻结/中断测试、CCM map |
| USB 会话、停止优先、重连不启动、可重读导出 | 4、5 | 协议/模拟串口分片、超时和世代测试 |
| 独立中文网页、实时监视、历史曲线与下载 | 5、6 | 浏览器截图、操作检查、实际下载文件 |
| 无测力计记录表及转速包络候选 | 5、7 | 手工算例、缺口拒绝和数据范围说明 |
| 宏隔离、默认无缓存、两种供电、旧功能兼容 | 7 | Make/CMake、符号/内存检查、完整回归 |

执行顺序固定为 1→2→3→4→5→6→7；每项先失败测试，再最小实现、验证和本地提交。当前只完成设计及实施计划，以上实施复选框保持未勾选。默认在当前会话逐项执行；若另行选择子代理方式，仍由主代理统一处理共享驱动/构建文件和提交，避免多个任务同时移动或覆盖文件。
