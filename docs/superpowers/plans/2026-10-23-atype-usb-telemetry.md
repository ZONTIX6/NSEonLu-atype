# A 板 USB 遥测适配计划

**目标：** 让当前工程的 USB 遥测设备、网页连接流程和数据导出统一标识为 A 板。

**现状：** 固件已使用 STM32F427 A 板 USB OTG FS（PA11/PA12），通过现有快照接口采集底盘、DBUS、功控、功率计、裁判和电容；网页 CSV v1 字段与固件一致，DJI 速度通过适配器从度/秒转换为 rpm。网页仍残留 C 板标识，导出名为 `c-board-`，USB 产品名仍为视觉串口。

**方案：** 沿用当前采集接口与 CSV v1、`@T1`、`@A1` / `@A2` 协议；统一网页标识与导出前缀，遥测模式的 USB 产品名改为 `A Board Telemetry`。视觉模式保留 `Vision Comm port`；VID/PID、序列号、引脚、控制与保存逻辑保持现有配置。

**技术栈：** STM32F4 HAL、USB CDC、C、原生 JavaScript、Node.js、Playwright。

## 执行与验证

- [x] 将 `tools/usb-telemetry/index.html`、`app.js`、`tuning-ui.js` 的板卡标识统一为 A 板，导出前缀改为 `a-board-`。
- [x] 在 `Src/usbd_desc.c` 根据 `USB_TELEMETRY_ENABLE` 选择产品名，更新工具 README 的串口识别说明。
- [x] 运行 `make -B -C tests/usb_telemetry test` 和全部 USB 工具 Node 测试，验证传输、单位、采集协议、调参和持久化兼容性。
- [x] 构建 `atype-vehicle` 与 USB 视觉配置，检查 ELF 中的 USB 产品名与占用。
- [x] 运行浏览器采集、调参、持久化流程并检查桌面和窄屏页面，验证真实串口模拟输入、断开重连、导出和演示。
- [x] 检查最终差异并创建仅包含本次改动的本地提交。

**验收边界：** 软件验证使用模拟串口；USB 实物枚举、实时采集和烧录需在 A 板连接后确认。

## 验证结果（2026-10-23）

- USB 主机 C 测试 5 组、USB 工具 Node 测试 43 项全部通过，包含实际 C 格式化输出到网页解析器的跨端测试。
- `make -j8 PROFILE=atype-vehicle` 通过；RAM 146704 B，Flash 148008 B，产物位于 `build/atype-vehicle/`。
- `make -j8 BUILD_DIR=build/atype-usb-vision VISION_USE_VCP=1 USB_TELEMETRY_ENABLE=0` 通过；遥测 ELF 仅含新产品名，视觉 ELF 仅含原视觉产品名。
- Playwright `browser-check.js`、`tuning-check.js`、`persistence-check.js` 通过；确认导出为 `a-board-*.csv`，覆盖 320～1440 px 页面、采集、断线重连、参数应用、同步重试及模拟掉电恢复。
- 界面机械检查提示现有配色和页面结构问题；本次只调整板卡标识，未更改配色与布局。
- 独立只读代码复核完成，未发现可操作的问题；最终差异检查通过。
