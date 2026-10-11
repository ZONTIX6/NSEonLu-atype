# 裁判系统实时功率调试

## 当前接收支持

USART1 裁判模式（`UART1_USE=0`）支持两种 `0x0202` 数据段长度：

| 数据段长度 | 偏移 4 的 float | 偏移 8 的 uint16 | 42 mm 枪口热量 |
|---|---|---|---|
| 16 字节旧版协议 | 实时底盘功率，单位 W | 缓冲能量，单位 J | 偏移 14 |
| 14 字节协议 | 保留，不作为有效功率 | 缓冲能量，单位 J | 偏移 12 |

载荷长度指帧头中的 `data_length`，不含帧头、命令码和 CRC16。两种完整帧都必须通过 CRC8、CRC16 校验；其他长度不更新功率状态。

用户实际设备已通过断点确认发送 16 字节数据段。之前仅接受 14 字节导致整帧丢弃：虽然 `0x0201` 的功率上限为 80 W，`has_power_heat` 仍是 false，缓冲能量和实时功率位置也保持零值。

## Watch 表达式

烧录兼容固件后，调试器中添加：

```c
referee_power_state.chassis_power_w
referee_power_state.has_chassis_power
referee_power_state.buffer_energy_j
referee_power_state.has_power_heat
referee_power_state.power_heat_tick_ms
referee_power_state.chassis_power_limit_w
```

- `chassis_power_w`：16 字节裁判帧中的实时底盘功率，单位 W。
- `has_chassis_power`：最近的功率热量帧含有有限的实测功率；14 字节帧或 NaN/无穷大时为 false，功率值清零。
- `buffer_energy_j`：缓冲能量，单位 J；它是能量，不能与实时功率互换。
- `has_power_heat`：是否成功解析过功率热量帧。
- `power_heat_tick_ms`：最近成功解析时的 HAL tick，应随有效帧更新。
- `chassis_power_limit_w`：`0x0201` 下发的功率上限，单位 W。

上述原始状态保存最近数据，时间过期不会自动清除。使用 `RefereeGetPowerSnapshot()` 时，仍要求 `0x0201` 和 `0x0202` 都在 300 ms 内更新，超过 300 ms 快照清零且 `valid`、`chassis_power_valid` 为 false。

```c
RefereePowerSnapshot_s power = {0};
if (RefereeGetPowerSnapshot(&power) && power.chassis_power_valid)
{
    /* power.chassis_power_w is fresh referee-measured chassis power in W. */
}
```

`referee_info.PowerHeatData.reserved_power` 在 16 字节帧下也能看到原始功率，但没有独立的新鲜度或数值有效标志，调试优先使用上述状态变量。

`chassis_power->state.measured_power_w` 来自 CAN1、ID `0x605` 的独立功率计。本次提供裁判系统的功率读取与快照接口，未切换控制外环的数据来源；仅接裁判系统时，该 CAN 功率计变量仍可能为零。

## 编译与上板核对

```powershell
New-Item -ItemType Directory -Path build/referee-legacy-power -Force | Out-Null
make -j8 BUILD_DIR=build/referee-legacy-power UART1_USE=0
```

烧录文件为 `build/referee-legacy-power/basic_framework.hex`。固件也同时生成 ELF 和 BIN。

现有 VS Code 调试配置使用 `build/basic_framework.elf`；该默认目录的固件本次也已完整重编译。停止当前调试会话，重新启动并下载程序后再观察新变量。若手动烧录隔离目录中的 HEX，调试器应加载同目录的 ELF。

1. 确认电管 User 串口连接 USART1，串口设置 115200、8N1。
2. 移除调试接收函数中的条件断点，让程序持续运行。
3. 确认 `has_power_heat`、`has_chassis_power` 变为 true，`power_heat_tick_ms` 持续更新。
4. 对照主控模块屏幕上的底盘实时功率，查看 `chassis_power_w` 是否随负载变化。缓冲能量按发送端实际值显示，不保证非零。
5. 长时间暂停调试器后，快照可能超时；恢复运行并等待新帧后再次查看。

## 本次验证结果

裁判功率、接收集成、字节流解析、功率链路四组主机测试通过；包含 16 字节接收、14 字节兼容、坏 CRC、非法长度、超时和非有限功率场景。完整 USART1 裁判模式 ARM 固件构建成功，RAM 使用 98,392 B、FLASH 使用 107,492 B。链接器提示 libnosys 的 `_close`、`_lseek`、`_read`、`_write` 未实现，不影响本次串口解析构建；硬件功率对照仍需烧录后确认。
