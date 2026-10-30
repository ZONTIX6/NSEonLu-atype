# 跃鹿框架边界整理验证记录

执行日期：2026-10-30（Asia/Shanghai）。基线：`53b9c51`。软件整理与验证完成，未推送、未烧录、未进行实车时序测试。

## 修改范围

1. `95fa7b0`：整车遥测采集/Flash 调度移至 `application/runtime/robot_telemetry.c`，标定限制由应用复制到协议模块；USB 会话初始化保留限制。协议编码与流传输继续属于 modules。
2. `5910a20`：四轮 PID 调参、自动整定和台架策略迁至 `dji_tuning_adapter.c`。速控、力控和标定适配器注册通用策略回调；驱动保留反馈、PID、停止证明、最终门控与唯一 CAN 发送路径。
3. 最后一次提交：速度底盘分为周期控制 `chassis.c`（521 行）、初始化 `chassis_init.c`（160 行）、调参 `chassis_tuning.c`（161 行）。公共头不变；保留电机/速度调试符号，跨文件声明只出现在私有头内。更新构建、架构检查、模块说明和自动生成索引。

控制算法、参数、CAN 分组/ID、单位、任务周期/优先级/栈配置、Flash schema 和协议字段未修改。力控模式同时排除三个速度底盘实现文件。

## 行为保持证据

- 逐函数比较基线：归一化三个私有调参函数名称后，底盘的 **13 个原函数体、5 组配置初始化器完全一致**。初始化及控制调用顺序保持原样。
- 独立只读审查确认遥测主体、整定流程、PID 注册/应用/重置在名称和状态访问映射后与原实现一致；快照与实时许可的选择、HAL tick 调用、短路条件和临界区位置保持不变。
- 原有 196 周期功控黄金夹具未修改。DJI 四阶段、无功率计、九电机隔离、整定恢复、晚到停机、CAN 失败及打包时效回归通过。
- `tests/predictive_power/verify_speed.py` 的禁用/影子/采集/KF 组合输出一致性检查通过，保留闭环 2000 条、直供与偏置各 2500 条的实际模块输出对照。该比较验证不同观测开关之间的一致性，不是实车性能结论。
- FC1 原协议断言保留；新增非默认限制的按值复制、初始化后保留、CAP 展示和 START 越界拒绝检查通过。

## 验证结果

统一验证脚本的最终结果为 **98/98 条命令通过**，包括全部主机 Make 测试、两个 portable 测试、四阶段/无功率计变体、Python 板级配置、Node 工具、16 套 Make 配置及产物检查、3 套 CMake 配置/构建/产物检查。命令数不等同于单元用例数。原始结果保存在本地忽略目录 `build/validation/results.json`。

另外完成：

- 架构检查器最终 **10 项自身测试通过、受检分层违规为 0**；模块到应用依赖规则覆盖本次涉及的遥测与 DJI 目录，底盘私有头隔离规则覆盖全部应用与模块。该结果不表示所有历史模块均已迁移。
- 应用公共头多消费者编译、通用电机独立编译/无调参链接符号检查通过，均已接入统一脚本。
- 结构索引 **55 项测试通过**，两份生成索引的 `--check` 通过。完整类型索引同时同步了此前已存在但未收录的标定/速度观测类型。
- Node **94 项测试通过**。
- 新增 Make 力控、台架标定、速度观测/KF/合成实验参数组合构建通过，均检查 ELF、Flash/RAM 边界和 HEX/BIN。
- 额外 CMake Debug 速控、Release 力控构建通过；**14 组宏契约**、后端符号隔离及 Release 有限浮点选项检查通过。
- 速度观测宏组合 **2 项测试通过**；合成 KF 参数只用于编译验证。
- 三阶段独立代码审查及最终检查器复审均未发现 Critical/Important 问题。增量构建依赖、统一测试入口和全模块私有头检查等建议已落实。

## 验证期间处理的构建/测试问题

- 基线标定测试存在 PRIMASK 桩重复声明和 DWT 函数重复定义；力控测试也重复定义 DWT timeline。仅调整测试平台的头选择和自定义时钟开关，未修改固件时钟实现。
- 完整矩阵的台架目录包含旧 `application/tuning_policy.c` 依赖，源自之前的目录迁移。核实路径均位于工作区 build 后，仅清理相关配置的 `.o/.d/.su` 编译产物；保留原始采集和参数文件。从失败的固件阶段继续，合并已通过的主机结果，原始中断记录及恢复记录均保留。
- 原产物检查器仅支持速控所需符号，会把正常力控裁剪误判为缺少 `ChassisPowerLimit`。增加显式 `--backend speed|force|calibration`，默认速控行为保持不变。实际三种产物检查通过，故意给速控产物指定 force 后端会被拒绝。

复现完整矩阵（将实际 CMake 可执行文件目录加入 PATH）：

```powershell
./scripts/validate-atype.ps1 -Python '<Python 3 executable>' -FirmwareMatrix -CMake
python tests/architecture/test_layers.py --strict
python tests/chassis_force/test_build_contracts.py
python scripts/check-atype-artifacts.py build/yuelu-force --backend force
python scripts/check-atype-artifacts.py build/yuelu-calibration --backend calibration
```

后两项需先分别使用 `CHASSIS_CONTROL_MODE=1` 和 `PROFILE=atype-bench CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_CALIBRATION_ENABLE=1 CHASSIS_FORCE_SPIN_MODE=0` 构建对应目录；宏契约脚本使用 `build/cmake-speed-debug` 和 `build/cmake-force-release` 的实际构建产物。

## 资源与时序边界

| 默认 Make 固件 | 基线 | 整理后 | 差值 |
|---|---:|---:|---:|
| Flash 镜像 | 167552 B | 167840 B | +288 B |
| 静态 RAM | 109780 B | 109804 B | +24 B |
| 含链接堆栈预留 RAM | 150744 B | 150768 B | +24 B |

GCC 静态栈帧：`DJIMotorControl` 前后均为 176 B，`ApplyAutotuneCycle` 均为 136 B，`UpdateChassisPowerBudget` 均为 1760 B。新增策略入口有 8–16 B 的独立栈帧，应按实际调用链计入，而不能仅比较最大单个函数。任务栈分配未改变；板上耗时、栈高水位与中断负载未实测。
