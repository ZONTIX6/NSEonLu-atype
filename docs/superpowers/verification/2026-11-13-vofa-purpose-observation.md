# VOFA 功率与卡尔曼专用观测验证

日期：2026-11-13。基线提交：`8a91d0a`。本次实现两个用途档、原始 CSV 分析及有效配置追溯；没有实车数据，因此未改变物理调参值、未烧录、未推送。

## 实现与协议

- `VOFA_OBSERVATION_PROFILE=1/2` 分别选择 schema 4/5，主板默认 2；模式 0/1 与显式 profile 0 的旧布局保留。
- 两档各 96 路、388 字节，230400 / 8N1 单帧约 16.84 ms，20 ms 目标时槽；线速预算静态检查通过。
- 固件索引、可读通道表和主机解析器共用 `config/32_capture/vofa_schema.json`。`python scripts/generate-vofa-schema.py --check` 通过。
- 功率周期保留真正采用的控制快照、实际配置身份与最终整数 CAN 命令；独立输出最新策略及年龄。超电记录命令尝试和发送结果；裁判分别保留两路时间。
- 卡尔曼在生产滤波调用前记录完整输入与实际配置，调用后记录结果。拒绝和管线重置保留最后尝试，重复源样本不增加尝试计数。发送时最新 DM 数据与中间样本输入分开。
- 有效功率参数由生产 C 构造和规范字段指纹校验；Flash/在线参数需要已有 USB 协议的原始 power GET 回复及 CSV 身份匹配。配置不明时只诊断，不以源码默认值冒充实际值。

## 主机验证

新测试按失败用例先行后实现。以下检查通过：

| 范围 | 结果 |
| --- | --- |
| `tests/vofa_telemetry`，`-O2 -fno-fast-math` | 15 个 C 目标：新两档、DM 来源、旧格式、单位、无效数值、忙/错误、缓冲所有权、低字回绕、时钟回绕、抖动、帧尾冲突 |
| `tests/vofa_analysis` | 35 项：原 CSV 布局/时间列/改名、缺列/混合 schema、异常/歧义、重复/缺口、连续段、生产 C 重放、候选比较、独立试次与 CLI |
| `tests/power_model_fit` | 原 32 项回归通过，包括生产 C 模型/限幅对照 |
| `tests/vofa_config` | 6 项：编译选项、完整参数、Flash/在线回复、篡改与缺失证据 |
| `tests/board_flash`、`tests/atype/test_board.py` | 各 12 项 |
| `tests/architecture` | 12 项 |
| 功率模块、超电、裁判、force、DJI adapter | 模块回归及功率 32 配置组合通过 |
| 远端融合、观测器、速度融合、中心补偿 | 相关 C 测试全部通过，包括真实导航系中间输入、拒绝保留和 getter 透传 |
| Git 差异 | `git diff --check` 通过 |

分析测试使用隔离环境 `build/vofa-python/Scripts/python.exe`，依赖为既有 `scripts/power-model-requirements.txt` 中的 numpy 2.2.6 / scipy 1.15.3。主机 C 桥接使用 GCC；固件使用 ARM GNU 13.3.Rel1。

## 固件构建

两档 Make 与 CMake/Ninja 均构建通过。共同选项：

```text
ONE_BOARD=1 CHASSIS_CONTROL_MODE=0 CHASSIS_IMU_CAN_RX_ENABLE=1
CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1
CHASSIS_VELOCITY_FUSION_CALIBRATED=0 VOFA_TELEMETRY_ENABLE=1
VOFA_TELEMETRY_MODE=2 VOFA_OBSERVATION_PROFILE=1 或 2
```

Make 使用 `PROFILE=atype-vehicle`，独立输出目录 `build/vofa-purpose-power` 与 `build/vofa-purpose-kalman`。CMake 使用 `build/vofa-purpose-cmake`，依次配置 profile 1、2 并构建，最终保留 profile 2。CMake/Ninja 工具在 `D:/Android/sdk/cmake/3.22.1/bin`。

| Make 产物 | BIN 字节数 | 链接 RAM |
| --- | ---: | ---: |
| 功率档 | 187088 | 156376 / 196608 B |
| 卡尔曼档 | 185832 | 156440 / 196608 B |

BIN SHA256：

```text
power   1471acf315e8730d265752747ce66f3d166f482a4db9bfc54902206fa392a8f2
kalman  4065cf3630e16a9296752f5236ebb20b15fdaaf63081c09acd89d43678c16f90
```

两档新的序列化源均显式使用 `-fno-fast-math`，保证非有限数据检查不会被优化删除。编译默认配置证据另生成于 `build/vofa-purpose-power/effective-compiled-config.json`；仅匹配现场 CSV 后才能用作实际参数证据。

## 独立审查与修复

- 大型快照增加导致底盘任务的已知调用链达 3928 B，叠加浮点抢占现场可能超过 4096 B。将唯一机器人任务使用的功率临时快照转为私有静态存储；编译器 `.su` 显示 `UpdateChassisPowerBudget` 从 2232 B 降为 184 B，原链降至 1880 B，含约 204 B 抢占现场约 2084 B。独立审查确认每周期初始化及控制语义保持不变。
- CSV 额外 `I96` 不再被默认为时间列；缺 `I95` 而用数值时间列补足宽度也明确拒绝。
- 同一采集改表头/重导出或大量重叠片段不再被当作独立训练与验证试次。
- 候选滤波报告相同输入上的拒绝/回退、无效输出、反初始化/重新初始化及响应诊断；噪声比较明确使用相同有效输出集合，并额外给出共同融合样本集合，防止通过拒绝困难输入造成虚假改善。
- 连续段充分性只依据实际可重放段；参数无效的长段不能替短碎段满足长度门槛。
- 最终 CAN 命令按 int16 解码，允许帧尾避碰的一位 float32 尾数扰动并恢复整数，不截断或接受任意小数。

固件审查及分析工具复审均无剩余已确认阻塞问题。

## 尚待实车验证

未验证 UART 中断 CPU 占用、控制任务实测耗时/栈高水位、无线持续吞吐与丢帧、真实 VOFA 导出变体，以及硬件功率/速度响应。50 Hz 快照不保证包含每次快速控制或滤波更新；连续输入不足时需要补采。真实速度精度、打滑、安装偏移与未知模块延迟不能仅由这些 CSV 唯一确定。后续参数修改必须基于充分数据，再以新的独立试次复采验证。
