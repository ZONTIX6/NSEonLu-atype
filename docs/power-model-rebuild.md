# 功率模型离线重建：第一批使用说明

日期：2026-10-29。对应[实施设计](superpowers/specs/2026-10-29-chassis-power-model-rebuild-design.md)和[实施计划](superpowers/plans/2026-10-29-power-model-rebuild-phase1.md)。

已执行的测试、构建和边界说明见[第一批验证记录](superpowers/verification/2026-10-29-power-model-rebuild-phase1.md)。

本批提供现役速度功控修正、共享生产参数和离线静态辨识工具。**生产六项系数没有重拟合；工具输出不能直接作为本车新标定或实车性能提升的证据。**本批没有新增动态原始数据采集、动态参数包、预测接管或 F334 协议功能，也不执行烧录。

## 1. 固件与测试改变

- `ChassisPowerRecordFinal()` 只在整组 CAN 提交成功时更新增长限制使用的命令历史。失败记录仍保留实际尝试提交的字节和失败结果，连续失败不能凭失败命令累积新的增长基准。CAN 成功入队不等于电调已执行。
- 速度应用和相关测试引用 `modules/algorithm/chassis_power_model_config.h` 中的 `CHASSIS_SPEED_POWER_MODEL_INITIALIZER`。原系数和 `current_conversion=1000` 不变；该除数是原拟合的命令归一化，不是经过测量核实的 mA/A 换算。
- 原历史模型用例保留；生产参数另有数值范围、低速非单调、不可行预算和量化限流回归。用户已有 PID、自旋配置没有包含在本批模型调整中。

当前生产模型仍为：

```text
x_j = abs(final_command_j) / 1000
n_j = abs(rotor_rpm_j)
P_j = k0 + k1*x_j + k2*n_j + k3*x_j*n_j + k4*x_j² + k5*n_j²
```

它不能区分驱动与制动，低速负线性项也意味着“降低电流命令必然降低预测功率”并非全域成立。本批保留这些现有行为，以独立数据比较候选模型。

## 2. 安装离线依赖

在仓库根目录的 PowerShell 中运行。需要 `uv`、Python 3.12 和可执行的主机 `gcc`；主机 GCC 用于编译生产 C 预测器与限流保护，ARM 编译器不用于离线 DLL。

```powershell
uv venv --python 3.12 build/power-model-venv
uv pip install --python build/power-model-venv/Scripts/python.exe -r scripts/power-model-requirements.txt
gcc --version
```

依赖固定为 NumPy 2.2.6、SciPy 1.15.3，安装在忽略的 `build` 目录。已存在此虚拟环境时直接使用，不必重复创建。工具不修改全局 Python，也不自动下载标定数据。

## 3. 运行合成示例

以下示例会生成三个独立的确定性合成实验，用来验证数据解析、参数恢复及生产 C 检查。输入和结果目录都选一个尚不存在的路径。

```powershell
& ".\build\power-model-venv\Scripts\python.exe" tests/power_model_fit/fixtures.py build/power-model-example
& ".\build\power-model-venv\Scripts\python.exe" scripts/rebuild-power-model.py `
    --manifest build/power-model-example/manifest.json `
    --output-dir build/power-model-report-001 `
    --emit-c
$LASTEXITCODE
```

生成器的已知参数为 `P0=5, R=0.08, Km=0.02, K1=0.003, K2=0.00008`，全部是工具测试值。`synthetic=true` 和 `hardware_release=false` 必须保留；即使误差极小，也不能拿它们替换当前固件参数。

默认主机编译目录为 `build/static-power-replay`。可用 `--build-dir` 指定另一目录；并行运行多个分析进程时使用独立编译目录，避免覆盖正在加载的动态库。

## 4. 实验清单 manifest

每个 CSV 代表一个完整实验文件。路径相对于 manifest 所在目录解析，也可使用绝对路径。所有实验必须来自同一计量边界和单位约定。

```json
{
  "schema": 1,
  "boundary": "chassis_load",
  "units": {
    "command": "dji_command_raw",
    "speed": "rotor_rpm",
    "power": "W"
  },
  "instrument_error_w": 0.1,
  "steady": {
    "window_ms": 40,
    "max_gap_ms": 20,
    "command_span": 0,
    "rpm_span": 0,
    "max_meter_age_ms": 20,
    "max_motor_age_ms": 20
  },
  "experiments": [
    {"id": "train", "session": "synthetic-train", "split": "train", "path": "train.csv", "format": "aligned_static", "synthetic": true},
    {"id": "calibration", "session": "synthetic-calibration", "split": "calibration", "path": "calibration.csv", "format": "aligned_static", "synthetic": true},
    {"id": "test", "session": "synthetic-test", "split": "test", "path": "test.csv", "format": "aligned_static", "synthetic": true}
  ]
}
```

这里的 `0.1 W`、40 ms、零变化容差仅适用于合成示例。实测时按仪表误差、实际采样间隔和稳定工况定义选择数值，并在分析测试集前冻结，不把示例值当成标定值。

| 字段 | 约定 |
|---|---|
| `schema` | 整数 1 |
| `boundary` | `chassis_load` 表示已确认的底盘负载侧；`unknown` 允许进行分析，但不能通过验证。电池输入、超电输入与负载侧不可混用 |
| `units` | 必须与示例完全一致；原始命令不是 A，转速是电机转子 rpm |
| `instrument_error_w` | 非负有限数，作为独立仪表误差加入校准余量；不能为通过验收而事后调大 |
| `steady.window_ms` | 稳定窗口持续时间，必须大于零 |
| `steady.max_gap_ms` | 相邻有效记录允许的最大间隔，大于零且不超过窗口时长 |
| `command_span`、`rpm_span` | 窗口内每轮最大值减最小值的上限，分别用原始命令和 rpm |
| `max_meter_age_ms` | 功率计最大帧龄 |
| `max_motor_age_ms` | CPC1 有逐轮帧龄时使用；其他格式没有这些字段，不能由此推断已核实电机测量年龄 |
| `id` | 非空且唯一的实验名称 |
| `session` | 完整独立实验批次。一个 session 不能跨训练、校准与测试集合 |
| `split` | `train`、`calibration`、`test`；三类均必需，可各有多个文件 |
| `format` | `aligned_static`、`legacy26` 或 `cpc1` |
| `synthetic` | 必须显式填写布尔值；真实数据才填 `false`，不能省略或用字符串代替 |

同一路径或同一 SHA-256 内容不能重复使用，即使文件被复制改名。工具能检查路径、内容和所声明的 session；无法自动发现人为切割、改写后冒充独立实验的日志。因此分组必须按完整采集批次完成，不能把相邻行随机分给训练和测试。

## 5. 三种 CSV 输入

轮序统一为左前、右前、左后、右后。CSV 必须有唯一表头；额外字段不会参与拟合。命令与转速保留符号，使用同一轮向坐标。

### aligned_static

这是离线输入契约，**不是默认固件已经提供的遥测格式，也不因名称含 aligned 就证明时间已对齐**。使用前需独立确认计量边界、时序和命令来源。

```text
tick_ms,meter_sequence,submitted,meter_online,meter_age_ms,enabled_mask,online_mask,load_w,command0,command1,command2,command3,rpm0,rpm1,rpm2,rpm3
```

- `tick_ms`、`meter_sequence` 是 uint32；前者是统一记录时基，后者是真实功率计事件序号。
- `submitted`、`meter_online` 为 0/1；`submitted` 指对应最终命令的整组提交结果，不是推测的电调执行确认。
- 两个电机掩码为 0～15，入选要求均为 15；`load_w` 用 W。
- 命令必须是 int16 范围内整数，转速为有限数。

### legacy26

直接读取[旧标定日志](chassis-power-calibration.md)的带表头 CSV，使用以下字段，其余旧列保留但不参与拟合：

```text
tick,meter_tick_ms,meter_online,meter_age_ms,motor_enabled_mask,p_meter_cw,
i_lf_cmd,i_rf_cmd,i_lb_cmd,i_rb_cmd,n_lf_rpm,n_rf_rpm,n_lb_rpm,n_rb_rpm
```

`p_meter_cw/100` 转成 W。没有独立事件序号，只能用 `meter_tick_ms` 近似去重；没有成功提交、独立电机在线和采样时间对齐证据。工具保留缺失标志，输出只能用于分析，不会补造 `submitted=1`。

### cpc1

输入是已解码的 CSV，不是原始二进制流。先运行现有解码器：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/decode-chassis-power-capture.ps1 `
    -InputPath capture.bin -OutputPath capture.csv
```

拟合读取以下字段：

```text
tick_ms,meter_tick_ms,meter_age_ms,enabled_mask,measured_w,flags,
final_current_lf,final_current_rf,final_current_lb,final_current_rb,
actual_rpm_lf,actual_rpm_rf,actual_rpm_lb,actual_rpm_rb,
motor_age_ms_lf,motor_age_ms_rf,motor_age_ms_lb,motor_age_ms_rb
```

它使用最终命令，不使用 `raw_current_*` 需求。功率已是 W。原始格式缺少提交结果、独立电机在线掩码和功率计事件序号；功率时间仍是接收时刻。CPC1 不是完整动态辨识合同。若一个解码文件包含多个独立捕获，先按完整捕获拆分并明确 session，不能把不连续捕获拼成一个稳定窗口。

## 6. 样本筛选与拒绝

每种拒绝数量都列入 `data_quality`；已完成输入解析但拟合失败的报告也保留这些统计。坏记录会打断稳定窗口，长间断后的数据重新积累完整窗口。正常重复功率事件继续维护控制状态稳定窗，但每个事件只贡献一次拟合样本；重复行中的失败、失联、制动和命令跳变仍影响稳定准入。控制时间与计量事件分别维护高水位，uint32 正常回绕可以处理。

| 原因 | 处理 |
|---|---|
| `numeric` | 缺失必要字段、NaN/Inf、非整数命令或越界数值 |
| `duplicate` | 同一计量事件不重复加入拟合；正常控制记录保留在稳定窗中 |
| `order` | 拒绝倒序或不合法的事件/时间推进，并打断稳定窗 |
| `submission` | 已知命令提交失败 |
| `stale` | 功率计明确离线、年龄为负或超过上限 |
| `motor_health` | 四轮未全部使能、明确未全部在线，或 CPC1 电机帧龄不合格 |
| `capture_flags` | CPC1 数值错误、guard 不可行、搜索耗尽或记录量化/饱和标志 |
| `negative_power` | 负功率不进入本批正向耗电拟合 |
| `domain` | 命令绝对值超过现役软件上限 15000，或转子转速绝对值超过协议数值范围 32768 rpm |
| `braking` | 任一轮命令与转速乘积为负，不用四轮乘积之和掩盖混合驱动/制动 |
| `ambiguous_quadrant` | 某轮命令和转速只有一个严格为零，方向证据不足 |
| `not_steady` | 未积满窗口，或逐轮命令/转速变化超过设定范围 |

稳定的零命令、零转速样本保留用于公共底耗。首版对零值采取严格规则，不能靠手动改成零来伪造静止数据。高功率样本不因拟合残差大而被自动删除。

训练至少需要 10 条入选样本，校准和测试各至少 5 条，且每份实验至少有一个有效稳定样本。这只是软件最小数量检查，不表示数据量足够。五个特征还必须满秩，列归一化后的条件数不超过 `1e6`；单一速度、单一自旋或缺少独立电流变化通常不能辨识全部系数。

## 7. 候选模型、校准与生产 C 复核

首版只拟合**命令代理模型**，不拟合反馈电流模型：

```text
i_j = final_command_j / 1000          # 归一化命令，不是安培
omega_j = rotor_rpm_j * pi / 30       # 转子 rad/s
P_load = P0 + sum(R*i_j² + Km*max(0, i_j*omega_j)
                  + K1*abs(omega_j) + K2*omega_j²)
P0, R, Km, K1, K2 >= 0
```

`P0` 是同一负载测量边界下的四轮公共底耗，只计一次；其余系数为共享轮参数。电阻、机械系数等名称描述特征含义，不能把命令代理系数当作已独立测得的物理电阻或转矩常数。

训练只用 `train` 数据，列缩放后用有界最小二乘求解。校准集确定经验余量：最大正向低估残差加 `instrument_error_w`。测试集不重新拟合或调余量。报告按集合和完整实验分别给出 RMSE、MAE、偏差、99% 低估分位数、最大低估、经验包络违反量与覆盖率，并与相同样本上的当前生产模型比较。

软件验证要求每个测试实验的 RMSE、MAE、99% 低估指标不劣于当前模型，且不违反校准经验包络。校准/测试特征超出训练特征包围盒也会使验证不通过。包围盒仅描述已观测范围，不能证明特征之间所有组合都有实测覆盖；经验余量也不是全状态硬功率保证。

正向稳态域内转换为六项兼容系数：

```text
k0=P0/4; k1=0; k2=K1*pi/30; k3=Km*pi/30;
k4=R; k5=K2*(pi/30)²; current_conversion=1000
```

工具编译并调用生产 C 预测器，检查此转换与五项公式的一致性，再按多个预算验证静态分配加 Stage 3 guard 的最终整数命令。guard 装配使用新鲜稳定反馈、零加速度、关闭启动增补、`safety_factor=1` 和最大命令 15000，以便直接比较物理瓦特预算；这不是完整实际控制过程回放。

不能把 `PowerModelApply()` 中间结果或布尔成功当作最终预算可行：被动损耗可能已超预算，非驱动轮也可能形成无法通过驱动缩放消除的耗电。报告分别列出不可行情况、guard 状态与最终功率检查。此检查验证软件数值路径，不证明实车瞬态、电机执行、功率计延迟或外环稳定性。

## 8. 输出、退出码与重复运行

| 输出 | 内容 |
|---|---|
| `report.json` | 数据质量、各实验指标、训练条件数、经验余量、生产 C 检查、输入/源码 SHA-256、Git 提交/工作区状态、编译器及依赖版本 |
| `report.md` | 便于阅读的摘要；详细原因和指标以 JSON 为准 |
| `model.json` | 拟合完成后输出的候选系数、兼容六项参数、单位、观测域和发布限制。验证失败时也可能存在，必须读取 `validation_passed` |
| `candidate.inc` | 仅在指定 `--emit-c` 且软件验证通过时生成；带 `ANALYSIS ONLY` 标记，不安装、不覆盖生产配置 |

所有模型均为 `hardware_release=false`。即使输入声明为真实数据、计量边界已知且所有软件检查通过，也仍缺少实机独立验证；工具不会自动批准部署。

- **退出 0：**完成拟合并通过当前软件对照，仍仅用于分析。
- **退出 2：**输入/构建/拟合失败，或已拟合候选没有通过软件验证。前者通常只输出错误报告；后者可能同时输出 `model.json`，但不输出 `candidate.inc`。
- **输出目录已存在：**参数解析阶段退出 2，保留旧目录原样，不覆盖旧报告。选择新目录重跑，避免把旧成功候选误认为新结果。

错误报告不一定具有完整指标结构；先检查 `status`、`error` 和 `validation_passed`。不要仅凭 `model.json` 存在或终端打印 `analyzed` 判定通过。

## 9. 有真实数据后的流程

1. 核实功率计在底盘负载侧的接线、量程、精度和内部延迟。保存硬件身份、电压、温度、负载、轮向、固件提交、编译参数与实际 Flash/PID 值。
2. 在相同计量边界采集多个独立完整实验：静止、不同电流和速度的正反向稳态驱动、直行、横移、自旋及不同地面负载。制动与换向另存，本批不把它们混入拟合。
3. 明确最终命令、反馈和功率各自时间来源。旧日志先用于静态候选分析，不添加虚假的成功提交或采样时间字段来升级证据等级。
4. 按完整采集 session 预先分配训练、校准和测试，冻结稳定窗口、仪表误差和验收约定。先看拒绝数量、激励范围、秩/条件数，再看拟合误差。
5. 检查逐实验误差、低估尾部、包络与保守程度。数据不足或验证失败时补采新的独立实验；不通过反复修改测试集或放大余量来凑通过。
6. 候选经过独立回放和评审后，再安排受控实机对照。接管、动态辨识、制动分支和原始采集属于后续实施，不能仅凭本工具生成的 C 片段跳过。

## 10. 软件回归入口

```powershell
& ".\build\power-model-venv\Scripts\python.exe" tests/power_model_fit/test_rebuild.py
make -B -C tests/power_model CC=gcc test
make -B -C tests/chassis_power_module CC=gcc test
make -B -C tests/chassis_power_transient CC=gcc test
make -B -C tests/chassis_spin CC=gcc test
```

这些命令不访问串口、不启动电机。最终固件构建与本批验证结果应结合交付记录阅读，主机回归不能替代上板测量。
