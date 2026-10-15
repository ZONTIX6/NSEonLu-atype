# USB 遥测仪表盘

C 板通过 USB CDC 虚拟串口上报数据，网页实时显示底盘、遥控器、四电机、功率和缓冲能量。页面、图表和数据处理都在本机运行。

## 打开网页

电脑安装 Node.js 后，双击本目录的 `start.cmd`，或在仓库根目录运行：

```powershell
node tools/usb-telemetry/serve.cjs
```

保持命令窗口运行，用电脑端 Edge / Chrome 打开 [本地仪表盘](http://127.0.0.1:4173)。浏览器需要支持 Web Serial；页面会检查支持情况。端口占用时可用 `node tools/usb-telemetry/serve.cjs 4174` 换端口，并打开对应地址。

点击“演示数据”可先查看界面，演示模式有明显标识。使用真实设备前点击连接，选择 C 板虚拟串口并授权；进入真实连接时会清除演示数据。C 板描述符目前仍可能显示为 **Vision Comm port**（VID `0483`、PID `00CA`）。

网页的连接授权由浏览器弹窗完成。请在独立浏览器窗口打开页面；内嵌预览环境可能没有串口能力。

## 固件配置与构建

`modules/telemetry/usb_telemetry_config.h`：

| 配置 | 默认 | 含义 |
| --- | --- | --- |
| `USB_TELEMETRY_ENABLE` | `1` | 1：USB CSV 遥测；0：原有 USB 视觉协议 |
| `USB_TELEMETRY_PERIOD_MS` | `20` | 采样发送间隔，默认约 50 Hz |
| `USB_TELEMETRY_HEADER_PERIOD_MS` | `1000` | 重复表头间隔 |
| `USB_TELEMETRY_BUFFER_SIZE` | `2048` | 静态行缓冲字节数 |

使用项目现有 ARM GCC 和 make 工具链，从仓库根目录构建：

```powershell
make -j8 BUILD_DIR=build_usb_telemetry UART1_USE=0 USB_TELEMETRY_ENABLE=1
```

使用项目现有烧录方式，将 `build_usb_telemetry/basic_framework.hex` 或 `.bin` 写入 C 板。构建命令不会自动烧录。遥测模式与 USB 视觉模式互斥；USB 遥测启用时，原有 `VisionSend()` 不再往 USB 混入视觉二进制帧。

切回视觉模式，使用独立构建目录，避免沿用其他编译选项生成的旧对象文件：

```powershell
make -j8 BUILD_DIR=build_usb_vision UART1_USE=0 USB_TELEMETRY_ENABLE=0
```

`UART1_USE=0` 选择裁判链路。使用 UART1 日志配置时，网页无法获得有效裁判功率上限和原始缓冲能量。功率计、电容、遥控器等也必须已连接并启用对应固件配置，页面才会显示为在线。

## 数据与单位

数据默认每 20 ms 采样。来源分别取得完整快照，并保留来源时间；不同设备的采样时刻可以不同。USB 发送忙时丢弃新样本，不等待、不积压，避免影响控制任务。

| 区域 | 显示内容 |
| --- | --- |
| 底盘 | 模式、坐标变换后的 Vx/Vy 控制指令、最终 Wz 指令 |
| 电机 | LF 左前 / RF 右前 / LB 左后 / RB 右后，目标与实际 rpm、电流命令和反馈、在线/使能状态 |
| 遥控器 | 摇杆、拨轮、左右档位、鼠标和键盘 |
| 功率 | 实测功率、模型预测、预算、目标和限制，单位 W |
| 缓冲 | 裁判原始缓冲能量、估算下限和预测能量，单位 J |
| 电容 | 能量比例 %、输入功率、输出状态、故障码 |

底盘的整车实际速度估计尚未实现，因此 Vx/Vy/Wz 显示控制原始单位；电流显示 DJI 原始值，不标为 A。转速是电机侧 rpm。裁判缓冲能量 J 与电容能量比例 % 是不同量。

网页把无效、离线或过期数据留空为“—”，曲线在相应位置断开。固件在发送时重新检查模型、功率计、电容状态和电容输入功率的 20 ms 新鲜度；底盘快照和遥控器使用 100 ms，裁判缓冲使用 300 ms。浏览器还会拒绝超过 500 ms 的底盘/模型快照；连续 500 ms 没有新行时，整条数据流标记陈旧。时间和序号回退、缺行以及长间隔都会分段。

“暂停显示”只冻结画面，仍接收数据；恢复后回到最新值。页面最多保留最近 60 秒且不超过 3000 条样本。导出 CSV 保存这个滚动窗口，原始整数单位保持不变；可导入项目现有 [波形工具](../waveform-viewer/README.md) 做离线分析。

点击功率、缓冲能量卡片或四轮转速绘图区可弹窗放大，也可使用图表旁的“放大 ↗”按钮。按 Esc、点击关闭按钮或弹窗外区域返回；图表获得键盘焦点后，Enter / 空格也能放大。放大图跟随当前实时或暂停状态，暂停时仍可打开和调整窗口大小。

在放大图内按住 Ctrl 滚动滚轮，可以围绕鼠标位置缩放时间轴，查看 100 ms～60 s 的细节，纵轴自动适配可见数据。Ctrl + 加号/减号以图中心缩放；Ctrl + 0 或“恢复全图”回到 60 s。弹窗打开时这些操作只作用于波形，关闭后恢复浏览器快捷键；普通滚轮仍可滚动。缩放不改变暂停状态，实时模式保持缩放范围随数据前进，每次重新打开图表恢复全图。坐标与悬停时间均相对当前显示窗口末端。

鼠标放在普通图或大图内，可查看定位线对应的最近真实采样，包括距窗口末端的秒数、板端 tick（ms）、各条曲线名称和数值。功率为 W，能量为 J，转速为 rpm；读数已经换算，CSV 导出仍保留原始整数单位。无效来源显示“—”，通信断点和距离采样超过 500 ms 的空白不显示读数。无数据或遥控器过期时，键盘和鼠标按键保持熄灭。

## CSV v1 协议

行以 CRLF 结束，表头在串口打开后及每秒重发。串口读取块不等同于一整行，解析器能处理任意分片。

| 后缀/字段 | 原始单位 | 网页换算 |
| --- | --- | --- |
| `*_cw` | 0.01 W | 除以 100 |
| `*_cj` | 0.01 J | 除以 100 |
| `*_milli` | 0.001 控制原始单位 | 除以 1000 |
| `cap_energy_pm` | 0.001 能量比例 | 除以 10 得到 % |
| `buffer_j` | J | 保持整数 J |
| `tick` / `*_tick_ms` / `*_age_ms` | ms | 未取得样本的年龄为 4294967295 |
| `i_*_cmd` / `i_*_fb` | DJI 原始值 | 不换算为 A |

固定 72 列，`version=1`：

```csv
version,seq,tick,tx_drop_count,chassis_valid,chassis_tick_ms,chassis_mode,vx_cmd_milli,vy_cmd_milli,wz_cmd_milli,rc_online,rc_age_ms,rc_lx,rc_ly,rc_rx,rc_ry,rc_dial,rc_sw_left,rc_sw_right,mouse_x,mouse_y,mouse_left,mouse_right,keys,model_tick_ms,motor_online_mask,motor_enabled_mask,rpm_lf,rpm_rf,rpm_lb,rpm_rb,target_lf_rpm,target_rf_rpm,target_lb_rpm,target_rb_rpm,i_lf_cmd,i_rf_cmd,i_lb_cmd,i_rb_cmd,i_lf_fb,i_rf_fb,i_lb_fb,i_rb_fb,power_valid,p_unlimited_cw,p_limited_cw,effective_limit_cw,p_meter_cw,meter_online,meter_age_ms,nominal_budget_cw,target_power_cw,power_source,referee_online,referee_limit_cw,buffer_j,buffer_age_ms,buffer_control_enabled,buffer_estimate_valid,buffer_lower_cj,buffer_predicted_cj,buffer_recovering,cap_online,cap_output_enabled,cap_error,cap_energy_pm,cap_chassis_cw,cap_limit_cw,cap_age_ms,cap_input_online,cap_input_cw,cap_input_age_ms
```

`seq` 按采样周期递增（包括被丢弃的周期）；表头重发可能占用发送机会。网页显示缺行计数，并保留 `tx_drop_count` 便于判断发送拥塞。序号和 tick 使用 uint32，支持自然回绕。

`chassis_mode`：0 零力、1 小陀螺、2 无跟随、3 跟随云台。遥控器开关：1 上、3 中、2 下。`power_source`：0 直供、1 超级电容。在线/有效标志为 0/1，四电机掩码从最低位起为 LF/RF/LB/RB。

串口打开参数为 115200、8N1；此项目 USB CDC 的波特率是主机控制设置，不是 UART 线路速率限制。网页打开端口后设置 DTR，固件据此开始上报。

## 验证

```powershell
powershell -ExecutionPolicy Bypass -File tests/usb_telemetry/run_tests.ps1
node --test tools/usb-telemetry/tests/*.test.cjs
node --test tools/waveform-viewer/tests/core.test.cjs
```

跨端测试会使用 GCC 编译 C 格式化测试程序，并将其实际输出分成任意字节块交给网页解析器，验证字段顺序、符号、功率/能量单位和导出兼容性。独立运行网页无需 GCC；只有开发测试和固件构建需要它。

浏览器 QA 脚本位于 `tools/usb-telemetry/tests/browser-check.js`，通过 Playwright CLI 在本地页面执行：

```powershell
playwright-cli -s=usb open http://127.0.0.1:4173 --browser msedge
playwright-cli -s=usb run-code --filename tools/usb-telemetry/tests/browser-check.js
playwright-cli -s=usb run-code --filename tools/usb-telemetry/tests/chart-check.js
playwright-cli -s=usb run-code --filename tools/usb-telemetry/tests/zoom-check.js
playwright-cli -s=usb close
```

浏览器测试使用真实 ReadableStream 模拟串口，不代表已完成 C 板实物枚举、烧录或实时性能验收。

## 连接排查

- 找不到端口：确认使用可传数据的 USB 线、板卡固件已启用 CDC，且系统识别到虚拟串口。
- 端口占用：关闭正在使用该端口的串口助手、其他网页或调试程序。
- 已连接但没有数据：确认固件使用遥测模式，等待周期表头，检查是否烧录了对应构建产物。
- 部分来源离线：检查该设备的供电、CAN/UART 链路和配置；网页连接 USB 成功并不意味着每个设备都在线。
- 拔插后：点击连接重新选择端口；已有窗口数据仍可导出。
