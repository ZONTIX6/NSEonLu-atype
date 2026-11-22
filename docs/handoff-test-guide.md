# NSEonLu A 板工程交接与测试说明

评估日期：2026-11-22
用途：交给下一位测试人员接手软件回归、A 板联调和底盘低速测试。

## 1. 当前结论

工程的软件实现和自动化验证已达到可进入受控硬件测试的阶段。STM32F427IIH6 A 板固件、主机侧模块测试、USB 遥测工具、板卡构建脚本、配置矩阵和链接布局检查均已覆盖。最新一次完整软件验证执行了 **113 条检查命令，113 条通过**，记录在 `build/validation/results.json` 和 `build/validation/provenance.json`。

这不等于整车验收通过。远端 IMU 安装/噪声审核标记仍为 `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED=0`；底盘速度反馈试验和新增防滑主动控制没有当前固件的完整实车验收记录。转交后应先核对板间 CAN 口和主板速度闭环开关，再按观测、低速、主动干预的顺序测试。

| 子系统 | 软件状态 | 接手测试时的判断 |
| --- | --- | --- |
| A 板整车固件与构建 | Make 16 种配置和 CMake 3 种 profile 均构建、链接检查通过 | 可做板级启动和接口验证；实际电机、串口和 CAN 行为仍须上板核对 |
| 云台、底盘、发射、遥控、裁判与图传 | 已集成到固件，相关主机测试通过 | 逐路检查实机接线、方向、单位和启停门控 |
| 功率控制、功率计、超级电容、Flash 参数保存 | 模块与故障路径有主机回归 | 尚需实车功率、掉电、温升及参数恢复验收 |
| 双 A 板 IMU 与速度融合 | 采集协议、接收、融合、遥测和离线重放已有覆盖 | 轴向、CAN、偏移和安装参数尚未完成最终实测审核 |
| 底盘平移速度反馈试验 | 有限幅闭环已实现，主机集成测试通过 | 当前工作树的板 1 预设已打开此试验，必须确认是否为本轮目标 |
| 防滑模式 0–3 | 已接入轮速 PID、电流/功率/停车门控链路；算法和 CAN 集成测试通过 | 模式 1 只观察；模式 2/3 尚无实车收益验收，模式 3 应最后测试 |
| USB 仪表盘、在线调参和自动整定 | 前端及协议测试通过 | 仍需在实际 USB CDC、浏览器、遥控和电机状态下操作验收 |

## 2. 代码与交接快照

工程根目录为 `NSEonLu - atype/`。本次评估时功能基线是分支 `codex/github-main-06efa7d` 上的提交 `a98b29d`（`feat: add staged chassis traction and braking slip control`）。该分支当时比 `github/main` 超前 23 个提交。

评估开始前工作树已有未提交修改和未跟踪文件，包含主板配置、底盘自旋参数、构建脚本、说明文档、`.agents/` 工具文件及 `output/` 记录。测试与交接修订只涉及本次标明的测试夹具、验证脚本和本文档；它们不会把原有工作树改动自动变成可复现的正式版本。测试人员应接收**完整当前工程快照**，或先由项目负责人整理并记录全部本地修改。只检出 `a98b29d` 不会得到当前所有本地配置。

`build/validation/`、`build/traction-mode*/` 和 `output/` 是本地生成/实验产物，不应假定已随 Git 提交。交接可复现证据时，应一并保存本轮 `results.json`、`provenance.json`、必要日志、最终构建选项和每块板实际烧录固件的 SHA-256。

## 3. 交接前必须确认的两项配置

### 板间 CAN 口不一致

当前有效源码默认值在 `config/90_board/board_config.h`：`CHASSIS_IMU_CAN_PORT=1`，即 `BOARD_CHASSIS_IMU_CAN` 使用 CAN1。主板预设没有覆盖这个宏。当前未提交的 README 却写着下方 IMU 板经 **CAN2** 通信；`docs/dual-a-imu-integration.md` 和代码仍写 CAN1 为默认值。

因此，在烧录前由测试人员查看实际线束和两块板接口，确认接的是 CAN1 还是 CAN2。两块板必须使用相同的 `CHASSIS_IMU_CAN_PORT`，并且都重新构建、烧录；不要只改其中一块。链路其余约定是 1 Mbps、标准 ID `0x180`、50 Hz。旧 ID `0x520` 固件不能与当前 ID 混用。实物轴向、CAN 接线和 116 mm 杆臂补偿见 [双 A 板 IMU 接入说明](dual-a-imu-integration.md)。

### 主板速度闭环预设已打开

当前工作树的 `config/00_flash/board_flash_config.h` 将板 1 的 `CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE` 设为 `1`；执行 `scripts/build-board.ps1 -Action Show -Board 1` 已确认构建参数会传入 `1`。而 README 和 [速度闭环试验说明](chassis-velocity-control-trial.md) 描述的是默认关闭、仅观测。与此同时，`CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` 仍为 `0`。

测试负责人应先决定本轮是“只观察融合”、单独测速度闭环，还是测防滑。若只做 IMU/融合观察，应使用 `CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE=0`；若测试速度闭环，则将 `1` 视为真实控制试验，先做低速起停、数据过期、IMU 断流及恢复检查，并保存板 1、板 2 的固件身份。当前 `scripts/build-traction.ps1 -Mode 1` 会沿用主板预设中的速度闭环开关，因此它不能自动隔离防滑观察与速度反馈；运行前必须查看生成选项。

## 4. 自动化验证证据

2026-11-22 的正式验证命令为：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/validate-atype.ps1 `
  -Python "<Python 3 executable>" -FirmwareMatrix -CMake
```

运行环境为 Windows PowerShell 5、Python 3.14、Arm GNU Toolchain 14.3.1+st.2、GNU Make 4.4.1、CMake 4.2.1、Node.js 24.16.0 和 MinGW GCC。Arm GNU Toolchain 须在 PATH 中；`scripts/build-board.ps1` 能从 STM32Cube bundles 自动寻找它，但验证脚本中的 Make 直接读取 PATH。

本轮 **113/113** 检查通过，包含：

- 严格架构、应用头文件和电机层边界检查。
- 所有 `tests/` 主机 C 测试目录、便携模块测试、功控阶段 0–3 及无功率计回归。
- 板卡配置和板卡构建/烧录入口测试，以及 USB 遥测和波形查看器的 Node 测试。
- 16 套 Make 固件配置及每套 ELF、Flash/RAM、栈与可选模块链接布局检查。
- `atype-vehicle`、`atype-bench`、`atype-direct` 三套 CMake 配置、构建和产物检查。

本次还校正了三处测试基础设施过期问题：profile 3 已是合法防滑诊断档但配置单测仍拒绝；直接力控测试目标缺少其必需的功率控制宏；验证脚本对 PowerShell 5 的原生命令 stderr 处理及旧路径 CMake 缓存不健壮。修订后的验证脚本在 Windows PowerShell 5 下重新执行并通过。其 CMake 验证目录现位于 `build/validation/cmake-*`，每次带独立运行标识。

需要查本轮逐项日志时，打开 `build/validation/results.json`、`build/validation/provenance.json` 和同目录 `.log` 文件。仓库内较早的 [A 板软件验证记录](atype-validation.md) 日期为 2026-10-22，不能替代本轮 113 项记录，也不能作为最新实机验收证明。

## 5. 构建与测试入口

查看两块板当前预设，不会构建或烧录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Show -Board 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Show -Board 2
```

确认预设和 CAN 口后，分别构建主板及 IMU 采集板。`Build` 只生成文件，不会烧录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Build -Board 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Build -Board 2
```

板 1 与板 2 的输出目录默认分别为 `build/board-1-main/` 和 `build/board-2-imu/`。每次上板前记录 `basic_framework.elf` 或 HEX 的 SHA-256、构建参数、CAN 口、VOFA profile 和固件角色。现有 build 文件的时间戳不能证明它与当前工作树完全一致；测试使用前应重新构建并核对。

防滑各模式有独立目录和身份文件：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-traction.ps1 -Action Show -Mode 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-traction.ps1 -Mode 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-traction.ps1 -Mode 0
```

脚本生成的 `build/traction-modeN/traction-build.json` 记录模式、标定声明、预设路径和 ELF SHA-256。`-Calibrated` 只是构建声明，不代表实车标定完成或验收通过。模式 2/3 要求显式传入该开关；使用前先完成模式 1、方向/阈值检查和项目负责人批准。

USB 网页工具入口为：

```powershell
node tools/usb-telemetry/serve.cjs
```

本机页面为 `http://127.0.0.1:4173`。浏览器端连接、串口、遥测协议、调参保护和运行说明见 [USB 仪表盘手册](../tools/usb-telemetry/README.md)。不要把在线整定或 Flash 保存当作只读操作；先断开电机动力或使用明确批准的台架状态。

## 6. 建议实机测试顺序

每轮开始前确认急停可用、底盘周边清空、操作者能立即断电；测试由熟悉机器人电气和遥控门控的人员执行。第一轮不以自旋、高速、急加速或模式 3 开始。

1. **固件身份与静态通信**：记下来源提交、工作树状态、两板角色、两份 ELF SHA-256、CAN 口和 VOFA profile。先不驱动车轮，检查第二板 50 Hz IMU 帧、主板接收新鲜度、UART7 遥测连接和超时状态。
2. **IMU 轴向与安装**：按手册断驱动手推前后左右及正反转，核对轴向和符号；静止记录至少 30–60 秒。检查 `(0,+0.116,0) m` 偏移、安装四元数和公共前/右接口映射。不要仅凭状态显示 `FUSED` 宣称速度精度通过。
3. **融合只读基线**：确认速度试验开关为 `0`。做前进、后退、横移、起停与原地旋转，保存完整 VOFA CSV；测试 CAN 断流、恢复、样本过期和停车，确认融合失效时不保留旧修正。
4. **速度反馈单独测试**：只在负责人确认 `CHASSIS_VELOCITY_CONTROL_TRIAL_ENABLE=1` 后执行。先在可控低速做阶跃、松杆停止和方向切换，再做 IMU/CAN 断流与恢复、样本年龄和控制周期检查。profile 2 是卡尔曼闭环遥测 schema 9；按 [通道说明](vofa-kalman-control.md) 核对有效位、修正量、年龄和控制/CAN 版本，不以 CAN 邮箱接受等同于电机实际执行。
5. **防滑观察模式 1**：在没有主动扭矩修改的模式 1 下做正常滚动、起步、制动和换向，观察错误判定、进入/退出阈值、反馈年龄和轮序。为隔离防滑效果，单独决定速度闭环开关；当前构建脚本会继承它。profile 3 为 schema 10、72 路，细节见 [防滑试验手册](chassis-traction-control.md)。
6. **防滑模式 2**：只有模式 1 的方向、轮径、陀螺符号、正常工况误判和故障门控都通过后，才在低速、低抓地可控条件下比较模式 0 与模式 2。每种条件至少 5 次，固定地面、载荷、轮胎、电池/电容状态和遥控命令，并用固定机位视频或其他独立地速参考确认真实滑动。
7. **防滑模式 3 与扩展工况**：模式 2 通过、机械/电控负责人同意后才测试新增制动，再逐步覆盖换向、载荷和自旋。记录母线电压、电机温度、实际电流、功率、车身轨迹、任务耗时和栈高水位。任何方向反常、门控不退出、旧制动保持、温升异常、任务超时或遥测数据无效都立即停止并回退到模式 0。

详细步骤和判据以 [卡尔曼底盘实车测试与验收手册](kalman-chassis-test-and-acceptance.md)、[速度闭环试验说明](chassis-velocity-control-trial.md) 和 [防滑试验手册](chassis-traction-control.md) 为准。不要将速度闭环和防滑参数在首次试验中同时改变，否则无法区分问题来源。

## 7. 每次测试必须回传的材料

- 测试日期、操作者、机器人/底盘编号、板卡型号、接线图或 CAN 口照片。
- 来源提交号、是否含本地修改、实际构建参数、两块板的固件 SHA-256，以及板 1/2 的角色。
- `CHASSIS_IMU_CAN_PORT`、IMU ID/频率、速度闭环开关、防滑模式/标定声明、VOFA profile 和对应 schema。
- 原始 VOFA CSV、USB 工具日志、关键通道截图、试验视频；保留原始文件，不只交分析后的图。
- 地面材质、轮胎/载荷、电池与电容状态、命令档位、每次试验的方向、速度、距离、起停时间和异常现象。
- 对照条件及重复次数、故障注入与恢复结果、停车和急停行为、任务周期/堆栈观察值。
- 对每项分别填写“通过 / 失败 / 未测”，并注明证据文件名；不能把软件单测通过代填实机结果。

## 8. 交接参考

- [A 板接线与安全说明](atype-integration.md)
- [双 A 板 IMU 接入说明](dual-a-imu-integration.md)
- [卡尔曼底盘实车测试与验收手册](kalman-chassis-test-and-acceptance.md)
- [速度闭环试验说明](chassis-velocity-control-trial.md)
- [防滑控制与实车试验手册](chassis-traction-control.md)
- [VOFA 卡尔曼闭环遥测通道](vofa-kalman-control.md)
- [USB 仪表盘与在线调参说明](../tools/usb-telemetry/README.md)
- [本轮自动化验证记录](../build/validation/provenance.json)（本地生成；打包交接时另附）
