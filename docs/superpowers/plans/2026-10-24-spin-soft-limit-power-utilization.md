# 自转软上限扩展实施计划

**Goal:** 实施已批准的软/硬上限方案，先利用基础功率，保留实际能力和保护边界。

**执行方式：** 在当前会话逐项实施；requesting-code-review 要求的独立审查使用专门审查代理，修复发现后复审。

**Architecture:** 保留 chassis_spin 的功率误差控制器，在其输入提供当前平移下正解算的仿射轮速及逆解整车速度，统一限制候选 Wz。预算模块独立处理能力锁存；CSV v5 追加诊断并兼容旧版本。

**Tech Stack:** C11 主机测试、STM32 ARM GCC、JavaScript node:test、现有遥测网页。

## 全局约束

- max_wz 继续表示软上限。扩展默认关闭，未标定硬指令上限跟随软上限，物理边界未配置禁止扩展。
- 首版扩展参数仅编译配置，不改变 Flash / 16 项自转调参布局；恢复旧字段时重新应用编译配置并校验。
- 200 ms 扩展确认、100 ms 持续跟随回收、5% 硬边界滞回、500 ms 能力锁存释放均按新样本。
- 固定/正弦/超级电容路径保持既有策略；不修改电流、PID 输出和功率保护。
- 保留任务开始时用户的 Kp、adaptive、fixed/max Wz 工作区改动，提交时不包含这些原有差异。

## Task 1: 自转控制和候选边界

Files: modules/algorithm/chassis_spin.{c,h}, chassis_spin_config.h; tests/chassis_spin/Makefile, test_chassis_spin_limits.c

- [x] 添加行为测试：健康软上限确认并以 600 Wz/s 上升、功率到达保持资格、重复功率/单轮帧不确认、单轮落后禁止扩展、物理硬上限、下降/退出/热更新。
- [x] 运行 `make -C tests/chassis_spin test_chassis_spin_limits.exe`，确认新接口缺失导致失败。
- [x] 配置验证、独立上限状态、同步 applied/target，约束完整候选轮速与整车速度；持续故障按 fall 回收。
- [x] 运行 `make -C tests/chassis_spin test`。

## Task 2: 预算锁存与底盘集成

Files: modules/algorithm/chassis_power_budget.{c,h}, modules/chassis_power/chassis_power{.h,_policy.c}, application/chassis/chassis.c; tests/chassis_buffer_budget/test_spin_priority.c

- [x] 先添加能力撤销后不重复准入、旧帧不释放、500 ms 新功率及四轮反馈且 5% 余量才释放的测试并观察失败。
- [x] 底盘由同一功控反馈快照计算实际整车速度，当前平移正解提供 wheel offset/gain 和 body offset/gain；按真实硬边界/持续跟随判断能力。
- [x] 更新预算输入和锁存输出；故障只清确认，退出自转清锁存。释放后重新完成基础功率确认。
- [x] 运行自转、预算、功率策略、运动/速度回归。

## Task 3: CSV v5 及网页诊断

Files: application/chassis/chassis.h; modules/telemetry/usb_telemetry*; tools/usb-telemetry/{protocol.js,app.js,index.html}; tests/usb_telemetry; tools/usb-telemetry/tests

- [x] 添加 v5 前 111 列稳定、旧版本缺失字段、扩大原因位、最大行长、导出/演示测试并观察失败。
- [x] 追加生效软/硬/物理上限、状态/确认/资格、功率缺口/跟随误差、缓冲锁存诊断；网页速度图附近显示状态。
- [x] 运行 C 格式测试和所有 Node 测试，浏览器核对演示状态及旧版本缺失显示。

## Task 4: 验证和提交

- [x] 主机相关测试、固件直供/电容和模式构建，检查新字段不进入 Flash。
- [x] 使用 requesting-code-review 技能请求独立审查，修复重要发现并回归。
- [x] 更新实施说明与标定步骤；实车标定/同工况采集明确留给连接硬件的验证。
- [x] `git diff --check`，只暂存本任务差异，创建本地 Git 提交并报告编号。

## 软件验证记录

- 自转主机测试：固定 / 自适应 / 正弦、模式覆盖、功率及缓冲回归；新增软上限确认、600 Wz/s 限斜率、功率到达保留资格、旧帧 / 单轮重复禁止确认、单轮落后回收至可跟随需求、负向轮速、物理硬边界、瞬时越界峰值、平移、故障和热更新。
- 预算主机测试：基础优先、低能量保护、100 ms 撤销和锁存、500 ms 新功率及四轮反馈释放、重新基础确认。
- 功控模块 32 种 source / meter / buffer / stage 组合以及生产策略联动通过；运动、速度、运动学、Flash gate/restore/retry 回归通过。
- USB 格式 / 传输 / 参数存储主机测试通过，检查精确表头、127 列顺序、整数最坏长度、任意 USB 字节分片；网页 Node 测试 54 项通过。
- Playwright：上限状态 / 原因 / 锁存 / 旧 v4 缺失 / 演示 / 375 px，以及整体速度、暂停、过期、旧 v3、放大和 320–1440 px 布局通过。截图位于忽略的 `output/playwright/spin-limits-*.png`。
- 固件构建使用独立目录：`build/spin-limits-direct`、`fixed`、`sine`、`supercap`、`calibrated`、`no-buffer`、`mecanum`；calibrated 使用的 10000 / 2 / 12000 仅为编译验证输入。
- 独立审查发现并修复默认关闭扩展时负功率降速被阻止、持续跟随失败只退至软上限等问题；复审无重要问题。
- 实车尚未烧录或测试；扩展开关保持 0。实际硬边界标定、带载稳定性、基础 / 缓冲分阶段持续功率利用率对照需实车完成。
