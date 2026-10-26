# 单轮力控标定工具与共享仪表盘验证

日期：2026-10-26。实施范围为 [原工具计划](../plans/2026-10-26-chassis-force-calibration-tools.md) 与用户追加的 [四标签设计](../specs/2026-10-26-force-calibration-dashboard-tab-design.md)。软件构建与主机测试不表示已完成物理标定；本次未烧录、未打开真实电机串口、未加载电机。

## 交付行为

- 工具默认关闭；启用后四轮仅接受独立标定状态机输出。正常力控的三个实测组仍为 0，未标定锁定保持有效。
- 单轮正负梯形脉冲、1500 CAN 单位幅值、500 ms 总通电、1000 rpm/50 ℃ 边界、连续静止准备、反馈/命令/能源/心跳门槛与发送邮箱前最终复查。
- 原始 RX 与当次 CONTROL 快照固定编码为 48 字节；1280 槽缓存位于 CCM。冻结及成功零电流提交后读取，容量/缺口/失败提交不被当作完整记录。
- `@FC1` 负责能力、状态、准备、开始、心跳、停止、清除和冻结数据块。额外只读 `META,generation,capture_id` 返回冻结 pulse/limits/filter 及原 session/generation，保证新 ARM 或重连后仍能恢复原测量条件；原 CAP/OK/DATA 字段未更改。
- 仪表盘四标签共用唯一 SerialSession、reader/DTR 和发送队列；STOP 优先，活跃标定切出需后续状态确认成功零提交。结果未知保留控制锁。旧地址仅跳转 `/#force-calibration`。
- 普通 CSV 与 force-v1 分开保留原始历史和导出格式；force-v1 字段按名称和单位映射。未提供的来源为空，控制 cycle 不推进超过 100 ms 的重复包不能维持有效读数。
- 原始 CSV/JSON、增长裕量候选、转矩表及离线分析；缺失原始字段、synthetic、数据不完整、轮号不匹配或无零提交证据不能形成四轮完成结论。修改分析输入会使旧候选失效。

## 已通过的软件检查

| 范围 | 实际检查 |
| --- | --- |
| 引擎 | 正负脉冲、参数拒绝、18 类终止输入、最终输出边界、连续静止与 START 再复查、回绕、尾段、缺少零提交时不能导出 |
| 记录 | 手写负数/float32/小端字节向量、有限缓存、不覆盖历史、冻结后只读、重读一致、序号回绕/缺口 |
| 真实驱动 | 实际 DJIMotorControl、LF/RF/LB/RB 到 ID 1/4/2/3 映射、四轮 PID 跳过、邮箱等待失去许可当帧归零、失败 CAN 命令留证、成功零提交门槛 |
| 互斥与恢复 | SET/SAVE 检查后被抢占的确定性反例；PID End 不能释放标定 owner；STOP 排队后重连、旧 STOP 和 ARM/STOP 会话切换竞态、旧冻结 capture 在新 ARM 后仍可读 |
| 主机回归 | `make -C tests/{force_calibration,chassis_force,dji_chassis_power_rtt,usb_telemetry,pid_autotune,tuning_flash} -B test` 全通过 |
| 优化 | 标定全套主机测试再以 `-Ofast -fno-fast-math` 通过 |
| Node | 全部 72 项 `tools/usb-telemetry/tests/*.test.cjs` 通过；覆盖协议分片、CRC 独立已知向量、会话/停止/心跳、增长缺口、报告结构、共享路由与队列，force-v1 列名与实际 C 格式化器比对 |
| 架构 | 新纯 C 目录纳入扫描；临时 RTOS/硬件/application 反例能被抓到，实际新违例 0、旧基线 0 |
| 构建约束 | ARM 编译器验证 9 种非法组合拒绝；读取实际 compile_commands/ELF 验证默认排除源码、无标定入口/缓存，数值源最终 `-fno-fast-math` |
| 浏览器 | FC 模拟串口：四标签单连接、夹具门槛、切换停止/未知锁定/重试、心跳、后台、刷新、重连、导入与实际下载、disabled/旧固件/端口占用/无 Web Serial，390 px 无横向溢出 |
| 既有页面 | browser、tuning、chart、selection、performance、persistence 检查通过；V2 整定完整模拟流程、采用/文件重试、四轮手动继续、SET 超时未知锁及重连恢复、旧固件与断连均通过。模拟本地文件写入失败的 HTTP 500 是预期注入；未出现页面 JS 异常 |

主机日志在 `build/fc-regression-*.log`、`build/fc-optimized-tests.log`；Node 日志在 `build/fc-js-tests.log`。浏览器日志在 `build/fc-dashboard-browser.log`、`build/fc-tuning-browser.log`、`build/fc-auto-browser.log`、`build/fc-browser-*.log`。所有串口替身仅在浏览器内创建，不枚举真实端口。

## 独立构建与内存

四种 Make 构建都使用独立目录并检查退出码：

```powershell
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/fc-tool-speed CHASSIS_CONTROL_MODE=0
make -j4 PROFILE=atype-direct BUILD_DIR=build/fc-tool-normal-force CHASSIS_CONTROL_MODE=1
make -j4 PROFILE=atype-bench BUILD_DIR=build/fc-tool-bench CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_SPIN_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=1 CHASSIS_FORCE_CALIBRATION_VALID=0 POWER_METER_ENABLE=1 USB_TELEMETRY_ENABLE=1
make -j4 PROFILE=atype-bench BUILD_DIR=build/fc-tool-cap CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_SPIN_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=1 CHASSIS_POWER_SOURCE_MODE=1 CHASSIS_BUFFER_POWER_ENABLE=1 POWER_METER_ENABLE=1 USB_TELEMETRY_ENABLE=1
```

| Make 产物 | 普通 RAM / B | CCM / B | FLASH / B |
| --- | ---: | ---: | ---: |
| 速控 | 150512 | 0 | 166512 |
| 普通力控 | 151576 | 0 | 164924 |
| 直供标定 | 150416 | 61440 | 213740 |
| 电容标定 | 150424 | 61440 | 214516 |

三个 CMake 后端也已通过；标定是 Release（`-Ofast` 后追加数值源 `-fno-fast-math`）：

```powershell
$fcCmake='C:/Users/20802/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe'
& $fcCmake -S . -B build/cmake-fc-speed -G "MinGW Makefiles" -DCHASSIS_CONTROL_MODE=0
& $fcCmake --build build/cmake-fc-speed -j4
& $fcCmake -S . -B build/cmake-fc-normal -G "MinGW Makefiles" -DATYPE_PROFILE=atype-direct -DCHASSIS_CONTROL_MODE=1
& $fcCmake --build build/cmake-fc-normal -j4
& $fcCmake -S . -B build/cmake-fc-bench -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DATYPE_PROFILE=atype-bench -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_SPIN_MODE=0 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -DCHASSIS_FORCE_CALIBRATION_VALID=0 -DPOWER_METER_ENABLE=1 -DUSB_TELEMETRY_ENABLE=1
& $fcCmake --build build/cmake-fc-bench -j4
```

CMake Release 标定普通 RAM 150280 B、CCM 61440 B、FLASH 228940 B。`.ccmram` 实际位于 `0x10000000`；协议 response 缓冲位于 `0x20000000` 普通 SRAM 范围。整个引擎/采集描述与缓存受静态 64 KiB 断言约束。链接脚本的 `_Min_Heap_Size=0x5000`、`_Min_Stack_Size=0x5000` 未改。

复查命令：使用已安装 Python `C:/Users/20802/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe` 运行 `tests/force_calibration/test_build_contracts.py` 和 `tests/architecture/test_layers.py`；未调用 Microsoft Store 的 Python 别名。

## 审查修正与证据

独立审查确认并修正了 START 静止复查、STOP 旧回执/会话竞态、SET/SAVE 抢占互斥、旧 capture 读取、导入报告结构/条件保留、分析输入失效、ARM 等待期间旧 STATUS 误释放、本地操作未知锁定、断线 busy 恢复和 force-v1 重复快照过期。相应失败反例均已补回归。

页面截图和下载为 **synthetic 串口替身证据**，不能当作车辆实测：

- [桌面页面](../../../output/playwright/force-calibration-desktop.png)、[390 px 页面](../../../output/playwright/force-calibration-mobile.png)。
- [模拟 JSON](../../../output/playwright/force-calibration-capture.json)、[模拟 CSV](../../../output/playwright/force-calibration-capture.csv)、[修改条件后旧分析失效](../../../output/playwright/force-calibration-changed.json)。
- JSON 实际核对原 pulse/META 身份、12 条原始事件、电压 24.3、源码字段 abc123、测量条件；改裕量后不含旧 analysis。

前六阶段提交：`e1e0c4a`、`bc08635`、`d889c9a`、`82469fe`、`0e529a2`、`49c5cdd`。构建隔离和板端审查修正提交为 `fe1e4ee`，统一仪表盘标签及报告/生命周期修正为 `fe85815`。完成回复报告最终文档提交。另有其他会话的波形性能与设计文档提交，不属于本功能的实现提交。

## 首次上板待验收

- 急停、端口关闭、USB RX 溢出、心跳撤力响应；最终零电流实际发送与电调执行。
- 真实 RX/控制采样间隔、漏帧、容量终止；ISR、发送前复查及整电机任务最坏耗时。
- 各任务栈高水位与堆余量；CCM 访问及复制到 USB SRAM 后的端点行为。
- 负载侧功率计标度、20 ms 新鲜度、预算掉额与裁判禁用、电容确认失效的实物效果。
- 夹具承力、限流/温度反馈与方向。先最小电流短脉冲确认归零，再进行逐轮测量。

这些项目仍待人工硬件验收。本次没有自动回填 K_out、轮缘力或增长裕量，没有更改 `CHASSIS_FORCE_CALIBRATION_VALID`。
