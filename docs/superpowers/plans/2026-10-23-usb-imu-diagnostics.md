# USB IMU 诊断实施计划

**目标：** 在现有 A 板 USB 网页显示 IMU，并让底盘跟随的实际输入可观测。

**架构：** 复用 INS 和底盘原子快照，CSV v2 在旧字段后追加只读字段；网页兼容 v1，复用现有图表与缓存。用户已明确保留整车跟随逻辑。

**技术栈：** STM32F427、C、原生 JavaScript、Node.js、Playwright。

## 步骤

- [x] 新增协议行为测试，运行 `node --test tools/usb-telemetry/tests/imu.test.cjs`，确认当前解析器拒绝 v2。
- [x] 修改 `usb_telemetry_format.h/.c`、`usb_telemetry_stream.c` 和 `usb_telemetry.c`：85 列 v2，IMU 有效位和 10 ms 新鲜度，定点缩放 1000。新增 `usb_telemetry_imu.h/.c` 转换一致快照，`INSSnapshot_s.has_sample` 区分未知和失效的已发布姿态，保留诊断年龄；不改变 INS 控制健康语义。
- [x] 修改 `application/chassis/chassis.h/.c`，快照只读发布 `offset_angle`；保留用户已有 Kp 改动但不纳入提交。
- [x] 修改 `protocol.js`：按精确表头选择 v1/v2，验证版本、符号与字段数；缺失 IMU 留空，新增物理量除以 1000。CSV 导出保持列数一致并可重新导入。
- [x] 修改 `index.html`、`styles.css`、`app.js`：读数、三组曲线、跟随角和现场解释，演示覆盖 IMU；复用图表交互。
- [x] 更新 C/JS 跨端夹具和说明，运行 USB C 测试、全部 Node 测试，构建遥测与视觉配置。
- [x] 用 Playwright 验证旧/新输入、IMU 失效、显示暂停与导出、曲线放大和窄屏；运行界面检测，完成独立代码复核。

## 验证结果（2026-10-23）

- `tests/usb_telemetry` 5 组、`tests/ins_service` 4 组、`tests/atype` 5 组通过；覆盖原始姿态保留、未知/失效样本、非有限数、uint32 回绕与 tick=0。
- `node --test tools/usb-telemetry/tests/*.test.cjs`：48 项全部通过，含真实 C 输出解析、v1/v2 和混合版本导出。
- Playwright `imu-check.js`、`browser-check.js`、`chart-check.js`、`selection-check.js`、`zoom-check.js`、`history-check.js` 全部通过。查看桌面截图确认新增读数和曲线布局，验证 320～1440 px 无横向溢出。
- `make -j8 PROFILE=atype-vehicle` 通过，RAM 146760 B、Flash 149168 B，产物 `build/atype-vehicle/basic_framework.hex`。
- `make -j8 PROFILE=atype-vehicle BUILD_DIR=build/atype-usb-vision VISION_USE_VCP=1 USB_TELEMETRY_ENABLE=0` 通过，RAM 139240 B、Flash 135120 B。
- 界面机械检测报告现有小字号、配色与空值破折号等问题；新增布局沿用既有样式，未扩大为整页重设计。独立复核发现的失效 IMU 年龄丢失问题已修复并补测，复核通过。
- 软件验证使用模拟串口和数据；未烧录或进行实机 USB、IMU 轴向及电机跟随验收。
