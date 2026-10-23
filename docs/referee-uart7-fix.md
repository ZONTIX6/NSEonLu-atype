# A 板裁判系统通讯修复

日期：2026-10-23。对照工程：`D:\nsezxb\NSEonLu`（C 板）；修复工程：`D:\nsezxb\NSEonLu - atype`。

最新配置：用户随后要求将裁判数据改到 USART6，当前默认已改为 `REFEREE_UART_PORT=6`；接线须移至编号 16 最左侧、丝印 `6`。以下根因记录描述此前使用 UART7 接线时的问题。

## 根因

用户确认裁判 User 线接在 A 板位号图编号 16 的最右侧、丝印 `7` 的接口。这是 **UART7**；编号 16 是一组串口的位号，三个接口从左到右分别为 USART6、UART8、UART7。

C 板通过 `UITaskInit(&huart1, ...)` 注册裁判接收。A 板原来通过 `BOARD_REFEREE_UART=&huart6` 注册，UART7 只初始化了硬件，没有注册裁判接收。数据进入 UART7，但协议解析器等待 USART6，所以裁判数据不更新。不能把 C 板的 USART1 原样搬过来，因为 A 板 USART1 已用于 DBUS。

两套工程的流式解析、CRC、13/17 字节 `0x0201` 和 14/16 字节 `0x0202` 支持基本相同；协议帧长不是本次接口不匹配的原因。

## 修复后的资源分配

| 用途 | 串口和引脚 | RX DMA | TX |
| --- | --- | --- | --- |
| DBUS | USART1，PB7（RX） | DMA2 S2 / C4 | 不发送 |
| 视觉 | USART3，PD8 / PD9 | DMA1 S1 / C4 | DMA1 S4 / C7 |
| 图传键鼠 | UART8，PE1 / PE0 | DMA1 S6 / C5 | DMA1 S0 / C5 |
| 裁判，可选 | UART7，PE8 / PE7 | DMA1 S3 / C5 | UART TX 中断 |
| 可选日志 | USART2，PD5 / PD6 | 未注册日志接收 | UART TX 中断队列 |
| 裁判，默认 | USART6，PG14 / PG9 | DMA2 S1 / C5 | DMA2 S6 / C5 |

默认 `REFEREE_UART_PORT=6`；只允许 `6/7`，其余串口被编译检查拒绝。`REFEREE_ENABLE=1` 且选择 UART7 时，`BOARD_AUX_UART7_DEVICE` 必须为 0，否则编译报错。USART2 日志与 USART2 辅助设备、跨串口重复单例设备仍由原有检查拒绝。六路 USART 服务容量足以覆盖允许的设备组合，DMA 控制器与 Stream 的组合互不重复。

UART7 没有配置 TX DMA，`RefereeSend()` 根据实际 `hdmatx` 选择中断发送，避免对空 DMA 句柄调用 HAL；USART6 的 DMA 发送保留。UART7 的硬件配置已是 115200、8N1、normal/direct RX DMA，并已有 `UART7_IRQHandler` 与 `DMA1_Stream3_IRQHandler`，无需增加 DMA Stream。

另修正了两套工程共有的恢复缺陷：HAL 保持 BUSY_RX 时先中止 RX，再启动接收；中止超时留下非 READY DMA 时先恢复 DMA 状态；裁判离线恢复后重新装载 300 ms 倒计时，避免每 10 ms 反复清掉半帧。RX 恢复不改变 TX 状态。C 板对照目录未修改。

## 构建及检查

```powershell
make -j8 PROFILE=atype-vehicle
# 可选 UART7，仅在将线迁移到丝印 7 后使用：
make -j8 BUILD_DIR=build/referee-uart7 REFEREE_UART_PORT=7
# 以下配置应编译报错，禁止裁判和辅助设备争用 UART7：
make -j8 REFEREE_UART_PORT=7 BOARD_AUX_UART7_DEVICE=1
```

当前接线使用 `build/atype-vehicle/basic_framework.hex`，调试符号使用同目录 ELF。接线为电管 TX → A 板 R（PG9）、电管 RX → A 板 T（PG14）、GND → G；按丝印确认信号，不凭插头形状判断线序。

上板后观察 `referee_info.GameRobotState.robot_id`、`referee_power_state.has_robot_state`、`referee_power_state.has_power_heat`、`referee_power_state.power_heat_tick_ms`。UART 诊断在 `referee_usart_instance->stats`：

- `rx_event_count` / `rx_byte_count`：交给接收回调的事件及字节数；被中止的部分缓冲区不计入。
- `last_rx_tick_ms`：最近一次接收回调时间。
- `error_count` / `last_error_code`：UART 错误回调次数及最近 HAL 错误码。
- `restart_count`：成功中止忙 RX 的次数。
- `start_failure_count` / `last_start_status`：接收启动或中止失败次数及最近 HAL 结果。

字节数不增长时检查实际接口、TX/RX 线序和发送端；字节数增长但有效帧不增长时检查 `referee_stream_parser.stats` 的 CRC 错误。功率字段为零不能单独判定断链，详见 [功率调试说明](referee-power-debug.md)。

自动验证包含端口选择与冲突拒绝、实际编译执行的句柄唯一性检查、无 TX DMA 的裁判发送、DMA 停滞/中止超时后的恢复、离线倒计时和原有协议回归。硬件通讯恢复仍需下载固件后上板确认。

切换 USART6 后重新验证：8 个板级配置用例全部通过，覆盖默认 USART6、可选 UART7、端口唯一性和冲突拒绝；裁判标准/兼容协议集成测试与 UART 恢复测试通过。整车 Make 默认配置和 CMake 显式 `REFEREE_UART_PORT=6` 均构建成功，产物布局检查通过；ELF 中 USART6、DMA2 Stream1/Stream6 中断入口均为强定义。Make 固件 Flash 为 149360 B，静态 RAM 为 105864 B。构建日志为 `build/referee-uart6-build.log`、`build/cmake-referee-uart6.log`，尚未进行上板收发验证。

此前 UART7 修复的实际验证结果（切换 USART6 前）：

- `scripts/validate-atype.ps1 -FirmwareMatrix` 的 81 项检查命令全部通过，包含 39 个主机测试目录、2 个便携模块目录、架构检查、5 个 DJI 变体、8 个板级配置用例、70 项 Node 测试、16 套 Make 构建及各自产物检查。
- 整车、台架、直供三套 CMake 构建显式设置 `REFEREE_UART_PORT=7`，配置、构建与产物检查全部通过。
- 隔离目录实际构建 `BOARD_AUX_UART7_DEVICE=1`，按预期报 `UART7 cannot be shared`，没有产生可用固件。
- UART7 与 DMA1 Stream3 的中断入口均为 ELF 中的强定义；固件不侵入参数 Flash，DMA 缓冲位于普通 RAM。

整车 Make 固件 Flash 为 149416 B，静态 RAM 为 105792 B，连同预留堆栈为 146752 B。原有车辆参数检查提示保留，构建无错误。日志在 `build/referee-uart7-validation.log`、`build/validation/`、`build/cmake-referee-atype-*.log`、`build/reject-referee-uart7.log`。本地构建保留任务开始前已有的底盘 Kp=0.75 和 `CHASSIS_POWER_BENCH_TEST=1` 工作区改动；这些改动不纳入本次通讯修复提交。
