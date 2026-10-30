# 底盘速度观测 S0＋S1 软件验证记录

执行日期：2026-10-30（Asia/Shanghai）。计划日期保留 2026-10-29。

阶段结论：**软件实现与本轮检查完成，实车未验收**。

```text
software_verified=true
hardware_gate=not_run
default_capture=0
default_kf_shadow=0
default_experiment_config=0
```

本轮从干净的 `061a08f` 继续；已有 Task 1–7 主体代码，不重做初次实现。修复提交为 `50fc7c2`（固件及回归）与 `4f46e0a`（回放及证据）。验证针对这些提交的代码内容，测试运行时它们尚在工作区。没有烧录或车辆运行，没有实车参数/收益或板上实时性结论。

## 1. 本轮补齐的行为

- 观测 tick 与 AdapterLimit 的 DWT cycles 配对，避免跨毫秒反馈在回放中被误判为倒序。周期起止时间单独传入捕获器，晚到 ISR 记录不会被随后追加的周期记录倒退窗口结束时间。
- 不可用频率、频率切换、多次回绕等计时异常保留缺测；不进入最大值、直方图或超预算次数。Arm 在 IDLE 存储准备 metadata，消除临界区内的 1 KiB 复制及重复静态缓冲。
- 原始反馈在来源时钟中断后先冻结旧窗；KF 配置版本取实际安装版本，不取 LF 是否已经初始化。无效功率清零并撤销有效位，当前 KF 无效/暂停不会标成可用。
- 导出在编码完成后再次检查禁驱，并以调度暂停保证任务侧许可发布与非阻塞排队之间的一致性。没有扩大捕获关中断区；UART 内部仍有原来的有界环形缓冲临界区。
- 离线事件即使伴随未知/旧 epoch，也先撤销 KF 有效性。暂停保留校正历史和计数，可用输出及年龄失效。未知耗时不伪造超时；新 KF 配置同时恢复慢层规划。
- 回放区分 train/validation/test、工况、缺帧、失联及版本边界；保留创新拒绝事件作为独立评分参照。低通未给参数时不猜值，补反馈年龄传播、热机段排除以及断帧后的加速度复位。
- manifest 空证据/缺实际来源不能进入硬件审查；捕获哈希校验、合成标记传播、缺测显示及暂停周期重现补齐。功率有效性使用原始 present/fresh 位，接线和对齐证据来自 manifest，保留原始位不伪写确认。

这些问题均有先复现失败、修复后通过的回归。没有把原先已实现功能的首次红灯过程追溯标记为本轮完成。

## 2. 主机矩阵

完整命令和原始输出位于仓库本地忽略目录 `build/speed-verification-20261030/`；`baseline/` 保留修复前结果。最终 `test-results.json` 为 13/13 通过。

| 检查 | 结果 |
|---|---|
| `make -B -C tests/speed_observation test CC=gcc` | time/capture/runtime 全通过 |
| `make -B -C tests/predictive_power test CC=gcc` | core/shadow/KF/注入 DWT 时钟全通过 |
| `make -B -C tests/dji_chassis_power_rtt test CC=gcc` | 真实解码、CAN 提交、门控回归通过 |
| `make -B -C tests/chassis_power_module test CC=gcc` | 通过 |
| `make -B -C tests/chassis_power_capture test CC=gcc` | CPC1 C 回归通过 |
| `tests/speed_observation/test_build_contracts.py` | 2 个测试通过，实际预处理宏组合 |
| `tests/speed_observation/test_capture_decode.py` | 6 个测试通过，含 C/Python 双向字节与缺测 |
| `tests/speed_observation/test_manifest.py` | 5 个测试通过 |
| `tests/predictive_power/test_speed_replay.py` | 26 个测试通过 |
| `tests/predictive_power/test_replay.py` | 2 个旧接口测试通过 |
| `tests/predictive_power/verify_speed.py` | 四类配置轨迹一致，非法接管宏拒绝 |
| `tests/architecture/test_layers.py --strict` | 6 个测试通过，0 边界违规 |
| `tests/chassis_power_capture/test_decoder.ps1` | 旧解码器通过 |

注入时钟测试覆盖快层 299/300/301 μs、连续三次超时暂停，慢层 2000/2001 μs、暂停后不执行求解及重新发布配置恢复。主机注入只证明逻辑，不证明板上实测耗时。

另外使用 DJI 测试 Makefile 的编译宏，分别重编译并运行 capture-only、shadow-only、shadow＋capture＋KF，均通过；日志为 `test-dji-capture.log` / `test-dji-shadow.log` / `test-dji-kf.log`。模块轨迹每种配置比较 2000 条 closed-loop、2500 条 direct50、2500 条 biased，共 7000 条；另有 demand50 诊断场景，不计入 CAN 事件数量。模块轨迹不替代实际驱动的字节断言。

## 3. 固件构建与数值选项

主机 GCC 16.1.0，ARM GNU 13.3.Rel1，Python 3.14；CMake 4.2.3、Ninja 1.13.2 使用 STM32Cube 已安装的本地 bundle，没有安装工具。四种 Make 使用独立目录、`-B -j8`；CMake 使用 Ninja 和 `--clean-first --verbose`。配置均为 `atype-vehicle`、速度模式、力控标定关闭，KF 构建仅使用明确标记 synthetic 的测试头。

| 目录 | 配置 | text＋data（B） | size 的 data＋bss（B） |
|---|---|---:|---:|
| `build/speed-base` | 新开关全关闭 | 167548 | 150748 |
| `build/speed-capture` | 仅采集 | 176004 | 179676 |
| `build/speed-shadow` | 仅已有影子模式 | 180104 | 154052 |
| `build/speed-kf` | 影子＋采集＋KF | 192776 | 183444 |
| `build/speed-kf-cmake` | 同上，CMake | 192788 | 183452 |

所有构建成功；构建前后 C/头文件/配置内容哈希一致。Make 与 CMake 每套均核对 9 个相关数值源的最终 `-fno-fast-math` 位于优化选项之后。原始 map、`.su`、符号表、二进制 SHA256、配置/编译命令和构建日志已保存于上述验证目录；`validation-summary.json` 汇总结果。CMake 有仓库既有 `add_custom_command(COMMENT)` / CMP0175 开发警告，未影响构建，未在本任务改动无关构建逻辑。

## 4. RAM 与栈：比较口径

`size` 的 bss 汇总包含链接脚本 `._user_heap_stack=40964 B`；它不是纯静态变量之和。两套配置以相同链接脚本比较增量，单独列出任务在既有 FreeRTOS 堆内消耗的容量，避免把它误说成新的链接器保留空间。

ARM ABI 探针确认：CSCCapture 25188 B、PPWheelKF 88 B、配置 40 B；新增观测任务 384 words ×4=1536 B，TCB 100 B，按 heap_4 的 8 B 头/对齐计入 1656 B 分配容量。DJI 实例由 448 B 增至 456 B，即每实例 +8 B；按最多 12 个实例保守计 +96 B payload。

| 比较 | 链接 RAM 增量 | 加观测任务及最多 12 电机 payload | 对 32768 B 的余量 |
|---|---:|---:|---:|
| KF＋采集相对已有 shadow | 29392 B | 31144 B（30.41 KiB） | 1624 B |
| KF＋采集相对全关闭 base | 32696 B | 34448 B（33.64 KiB） | **超出 1680 B** |

因此，仅“相对已有影子模式新增观测”的静态/分配容量核算落在 32 KiB 内。若验收口径要求从全关闭配置算全部功能，则当前不满足预算，应保持关闭并另做内存缩减。newlib malloc 元数据/碎片、实际堆余量和栈高水位仍需板上测量，不能仅凭 1624 B 理论差额放行。

Make 每套生成 180 份 `.su`；KF 的代表性单函数栈为 `cycle=512 B`、`DJIMotorControl=184 B`、`observe_wheels=152 B`、`PPWheelKFCorrect=112 B`、`CaptureFinal=96 B`。这些值不能取最大值代替调用链；电机任务 512 words 的余量必须用实际调用链/中断上下文和板上高水位验收。既有 `UpdateChassisPowerBudget=1760 B` 也应纳入应用任务观测。

## 5. 合成示例与协议修订

本地可检查的示例在 `build/speed-verification-20261030/example/report.json`，含捕获 JSONL、manifest、参数和 metadata。它由 `tests/predictive_power/test_speed_replay.py` 的确定性 fixture 生成，再经真实 CLI 和生产 C KF 回放；`synthetic=true`、`hardware_gate=insufficient_evidence`、功率不可用、短窗未拼接。仅证明工具链。

可在任意新 build 目录复现，以下生成器只依赖已提交测试夹具：

```powershell
$speedPython = 'C:\Users\20802\AppData\Roaming\uv\python\cpython-3.14-windows-x86_64-none\python.exe'
@'
import json, pathlib, sys
sys.path.insert(0, 'tests/predictive_power')
import test_speed_replay as f
p=pathlib.Path('build/speed-example');p.mkdir(parents=True,exist_ok=True)
(p/'capture.jsonl').write_text(''.join(json.dumps(r)+'\n' for r in f.records()),encoding='utf-8')
(p/'kf-config.json').write_text(json.dumps(f.config()),encoding='utf-8')
m=f.capture.build_manifest(dict(synthetic=True,trajectory_id='synthetic-ramp',artifacts={'capture':str(p/'capture.jsonl')}))
(p/'manifest.json').write_text(json.dumps(m),encoding='utf-8')
'@ | & $speedPython -
& $speedPython scripts/wheel-speed-replay.py --capture build/speed-example/capture.jsonl --manifest build/speed-example/manifest.json --kf-config build/speed-example/kf-config.json --output build/speed-example/report.json --build-dir build/speed-example/replay
```

CSC1 保留既有 schema 1 的 `diagnostics+64=observation_cycles`，修订初版计划的重复 `period_max_us` 字段，周期 tick 与该观测计数配对。累计最大周期由 runtime diagnostics 提供，报告计算窗内最大周期。未改变帧长度、CRC 或 CPC1。旧实现用 MotorBegin tick 的跨 tick 数据不追溯认定为精确一致性证据。

功率比较目前只接受已核实的零采样延迟；未知延迟或非零延迟返回不可用，后者需先提供实际对齐的测量数据。实车参数选择不自动化；响应指标是完整原始轮速轨迹的响应，不是各滤波器独立算法延迟验收。

## 6. 尚未执行的硬件阶段

操作规程已写入[使用说明](../../chassis-speed-observation.md)。Task 8 的训练/冻结参数、每主工况至少 10 组配对试次、实际 PID/Flash 导出、接线边界/采样延迟、最坏完整周期与临界区耗时、栈/堆高水位、超时与恢复、长运行/回绕及部分失联均没有板上实测证据。

300 μs、2 ms、32 KiB 是验收约束，不是本次软件通过后自动成立的硬件保证。保持所有新开关关闭；S1 没有实车放行，S2 未启动。
