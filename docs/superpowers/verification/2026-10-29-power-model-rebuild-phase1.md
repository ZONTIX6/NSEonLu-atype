# 功率控制与模型重建第一批验证记录

日期：2026-10-29。用户已批准设计 `dc9e217` 的第一批实现及验证、提交。

## 交付范围

- 失败或部分失败的 CAN 提交不再更新电流增长限幅历史；失败字节和诊断仍保留。
- 速度应用、模型/瞬态/自旋测试共用同一份生产六项参数，系数和归一化不变。
- 新增静态命令代理模型离线重建工具，支持 26 列标定日志、已解码 CPC1 和显式 `aligned_static` 契约。
- 完整实验分组、稳定段/异常筛选、受约束拟合、独立校准与测试、生产 C 预测/Stage 3 限流复核，以及来源哈希和分析报告。
- 所有候选均 `hardware_release=false`，不自动写入生产配置。未烧录、未启动硬件、未重标定本车参数。

## 测试证据

发送历史回归先在原实现失败：首周期 6000 命令提交失败后，次周期仍应最多 6000，原实现实际给出 12000。仅加入成功提交历史门控后通过。连续失败、增大请求、换向、失败零命令、恢复、重复最终记录及时间回绕均使用真实功率模块测试。

离线工具从缺失入口的失败测试开始实现；数据审查再发现并修复了正常重复功率事件清空稳定窗、CSV 行宽异常被忽略、非对象 manifest 无法生成拒绝报告三项问题。重复计量事件现在不增加拟合样本，但保留控制状态连续性；其中的失败、失联、制动和命令跳变仍影响稳定准入。

最终独立源码快照中通过：

| 检查 | 结果 |
|---|---|
| `python -m unittest discover -s tests/power_model_fit -p 'test_*.py'` | 32 项通过：21 项数据契约、11 项端到端/拟合/CLI |
| `make -B -C tests/power_model CC=gcc test` | 历史与生产参数、有限性、非单调、无解和整数限流通过 |
| `make -B -C tests/chassis_power_transient CC=gcc test` | 通过 |
| `make -B -C tests/chassis_power_module CC=gcc test` | 32 种配置组合、spin/idle 和新增发送历史通过 |
| `make -B -C tests/chassis_spin CC=gcc test` | 通过 |
| `make -B -C tests/chassis_power_outer_loop CC=gcc test` | 通过 |
| `make -B -C tests/dji_chassis_power_rtt CC=gcc test` | 原最终输出轨迹与 DJI 集成通过 |
| `make -B -C tests/predictive_power CC=gcc test` | 预测核心与影子通过 |
| `python tests/predictive_power/test_replay.py` | 2 项通过 |
| `python tests/predictive_power/verify_speed.py` | legacy/shadow 共 7000 条最终 CAN 命令一致，接管模式仍被拒绝 |

7000 条包含普通闭环 2000、50 W 直供 2500、模型偏差场景 2500。另有无最终输出记录的需求对照，不计入 7000。此对比验证影子隔离，原 DJI 轨迹回归另外验证既有成功发送行为。

## 隔离验证与构建

工作期间有另一项速度观测任务在相同目录修改文件，并提交了仅含其计划的 `f83914e`。本批从该提交导出基线，只叠加本批文件和 `chassis.c` 的共享模型两个差异块，形成 `build/power-phase1-review-source`，排除既有 PID/自旋参数及并行未提交代码。

导出时 Windows 行尾转换曾使 DJI CSV 字节对比失败；数值完全一致。将验证副本的两份 CSV 恢复为 `git show f83914e:<路径>` 的原始字节后通过，没有改产品代码、测试逻辑或仓库中的预期数据。后续预测检查继续完成，未跳过失败的验证项。

工具为 Windows MinGW GCC、Arm GNU 13.3 Rel1；离线环境 Python 3.12.13、NumPy 2.2.6、SciPy 1.15.3。独立快照使用以下构建：

```powershell
make PROFILE=atype-vehicle BUILD_DIR=build/legacy -j8
make PROFILE=atype-vehicle BUILD_DIR=build/shadow CHASSIS_PREDICTIVE_POWER_MODE=1 -j8
```

| 独立快照构建 | RAM | FLASH | 结果 |
|---|---:|---:|---|
| 默认速度 | 150192 B | 159280 B | 通过 |
| 速度影子 | 152128 B | 171616 B | 通过 |

这些数值对应排除了用户未提交自旋/PID配置的快照，不能与包含这些配置的整车工作区构建直接归因比较。此前工作区构建也通过，但最终交付依据以上独立复验。未执行 CMake，本次没有新的固件源文件或构建选项。

## 合成端到端示例

在独立快照执行 `tests/power_model_fit/fixtures.py build/example`，然后运行 `scripts/rebuild-power-model.py --manifest build/example/manifest.json --output-dir build/example-report --emit-c`。

已恢复生成参数 `P0=5, R=0.08, Km=0.02, K1=0.003, K2=0.00008`，测试集 160 个稳定样本的生产 C 预测 RMSE 为约 `8.73e-6 W`，五项双精度式与 C 预测最大差约 `3.50e-5 W`。这是合成方程恢复的数值精度，不是传感器或本车功率误差。

完成 2880 次静态分配加生产 Stage 3 guard 检查，其中 1464 次预算低于被动损耗，明确标为不可行；其余可行预算违反为 0，搜索耗尽为 0。不可行情况下不会因为 C 函数返回成功而报告功率可满足预算。示例 `validation_passed=true`，但仍为 `hardware_release=false`。

## 审查与剩余工作

独立审查覆盖固件改动、生产参数一致性、失败历史测试、离线分组/拟合/报告及真实 C 验证；发现的问题已修复并复查通过。

本批没有证明真实地面性能收益。后续需要确认负载侧功率计边界、原始命令/反馈单位、采样延迟，收集覆盖静止、驱动、制动和不同负载的独立实验，再重建并验证本车参数。动态采集、反馈电流模型、动态参数辨识和接管均不在本批交付范围。
