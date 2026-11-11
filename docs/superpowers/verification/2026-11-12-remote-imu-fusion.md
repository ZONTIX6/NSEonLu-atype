# 第二板 IMU 旁路融合：软件验证记录

日期：2026-11-12。基于 `d209c08` 后的本次改动。本文记录软件检查，**没有连接硬件、烧录或实车验收**。

## 实现范围

- 板 1 预设开启第二板 IMU 融合；板 2 保持 IMU-only 发送。固定安装坐标与杆臂补偿后，远端加速度真正进入速度 KF。
- 轮速历史插值、源时钟积分、重复包去重、异常重置和轮速降级已接通；等待插值右端点有明确时限。
- 输出只供观测/遥测，未接入底盘目标、速度 PID、功控或安全控制。
- CAN 协议、FireWater schema 3 的 72 列保持不变；新增模式、估计时间和配置审核标志。

## 主机验证

| 检查 | 结果与覆盖 |
| --- | --- |
| `make -B -C tests/chassis_remote_fusion test` | 通过，11 个行为用例：跨板时钟偏移、插值/真实加速度作用、重复包、连续等待右端点及超时、安装旋转/非零杆臂、接收抖动、过期/缺包/重启、历史不足/偏斜、非法数据/参数、时间及序号回绕 |
| `make -B -C tests/chassis_velocity_observer test` | 4 个构建目标通过，覆盖旧本地路径和新增远端路径；远端融合无需本地 INS/云台有效，丢远端退轮速，丢轮速失效，恢复可重新收敛 |
| `make -B -C tests/vofa_telemetry test` | 6 个目标通过，含旧远端采集、远端融合未审核/已审核的字段与标志 |
| 板选择脚本测试 | 7 项通过，板 1/2 的远端融合开关分别为 1/0 |
| 配置编译门禁 | 1 个合法组合通过，7 个非法组合均被拒绝 |
| 严格架构检查 | 10 项通过，零层级违规；应用头文件检查通过 |
| 全量验证脚本 | 已执行 66 组，65 组通过、1 组因既有默认配置改动失败，详见下文 |
| USB 遥测/波形查看器 Node 测试 | 单独补跑脚本未继续执行的后续步骤，94/94 通过 |

新功能先执行失败测试再实现；审查发现“每包等待插值右端点时重置，可能长期无法进入 FUSED”的问题后，补充连续等待及超时回归，确认失败后修复，再次通过。独立代码审查和文档审查已完成，无未解决的重要问题。

全量运行命令：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/validate-atype.ps1 -Python 'C:\Users\20802\AppData\Roaming\uv\python\cpython-3.14-windows-x86_64-none\python.exe'
node --test 'tools/usb-telemetry/tests/*.test.cjs' 'tools/waveform-viewer/tests/core.test.cjs'
```

全量检查的唯一失败是 `tests/atype/test_board.py::test_vehicle_defaults`：测试要求 `VOFA_TELEMETRY_MODE=0`，而任务开始前工作区已将其改为 `1`，并同步修改 `docs/configuration-index.md`。这两项既有修改均保留且不纳入本次提交。在 `build/remote-fusion-validation/board-default-reference/` 隔离副本中，仅恢复该头文件的原跟踪版本，其余保留本次配置和头文件，板配置测试 **10/10 通过**。这不能写作当前工作区全量验证全部通过。

首次 Windows PowerShell 运行因将原生命令的 stderr 测试进度解释为异常而提前退出，改用上述 `pwsh` 重跑；没有为此修改验证脚本。

## 固件构建与产物检查

| 配置/目录 | Flash（B） | 链接器 RAM（B） | 结果 |
| --- | ---: | ---: | --- |
| Make 板 1 远端融合 `build/board-1-main` | 183800 | 153424 | 通过 |
| Make 板 2 IMU-only `build/board-2-imu` | 72056 | 125144 | 通过 |
| Make 双板纯采集 `build/remote-record-only` | 173824 | 152480 | 通过 |
| Make 普通默认 `build/remote-fusion-validation/baseline` | 171296 | 151560 | 通过 |
| Make 本地标定兼容 `build/remote-fusion-validation/local-synthetic` | 181960 | 152456 | 通过，仅使用测试桩标定头 |
| CMake 远端融合 `build/cmake-remote-fusion` | 183824 | 153440 | 通过 |

本地标定兼容构建只验证原路径仍能编译；测试桩参数不是真实标定，不用于现场烧录。CMake 构建没有显式指定 `ONE_BOARD`，验证了项目默认板角色路径。

五种主板产物通过 `scripts/check-atype-artifacts.py`。第二板使用适合 IMU-only 的符号检查：INS、四元数 EKF 和 CAN 发送入口存在，底盘/云台/发射/电机控制/遥控命令任务及远端融合入口不存在，没有未解析强符号。

同一工作区下，板 1 开启融合相较纯采集增加 Flash **9976 B**、链接器 RAM **944 B**。RAM 数字含链接预留和 FreeRTOS 堆，不是运行时剩余栈测量；`.su` 的单函数栈数字也不能代替任务整条调用链高水位和耗时实测。

本次构建的 BIN SHA256：

```text
build/board-1-main/basic_framework.bin
69442EF42773C904DC3068685D56AF4C62D87758575A0B553D9F716BC5226647

build/board-2-imu/basic_framework.bin
C7AAF53C0C72A052BD8C62E9EF25A1C53CC86A6CBAC9CF4117E85D9BD8A7E555
```

## 日志与尚待完成的现场工作

本地构建目录不入库。执行证据位于 `build/validation/results.json`、`build/validation/provenance.json`、各组日志，以及 `build/remote-fusion-validation/` 下的构建、配置门禁、隔离默认值、Node、产物检查日志。重复构建前如需保留该次证据，应另行归档。

必须按[测试与验收手册](../../kalman-chassis-test-and-acceptance.md)完成安装方向/杆臂/噪声参数审核、静止与运动测试、独立参考精度比较、故障降级、运行耗时和栈余量测量。默认单位四元数及零杆臂仍是未验证初值，`CHASSIS_REMOTE_IMU_CONFIG_VERIFIED=0`。

跨板观测时刻使用“接收起始时刻减发送端样本年龄”的估计，未知单向传输/排队/内部采样延迟仍在；源时钟只保证积分步长不被接收抖动直接替代，不代表两板已经同步。当前软件完成旁路融合能力，尚不能据此声称实车精度达标或允许接管控制。
