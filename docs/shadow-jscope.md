# 影子诊断实时波形（J-Scope）

使用 J-Link + J-Scope 的 RTT 模式，在底盘运行时读取影子快照。此功能不改变 PID、电流输出或功率预算；默认关闭，默认输出 50 Hz，选择左前轮。

## 构建

仅查看预测功控候选（没有 KF 参数时轮速估计字段无效）：

```powershell
make PROFILE=atype-vehicle BUILD_DIR=build/shadow-jscope CHASSIS_PREDICTIVE_POWER_MODE=1 CHASSIS_SHADOW_JSCOPE_ENABLE=1 LOG_OUTPUT=1 -j8
```

同时查看 KF，需已有经过实验确定的 `kf-config.json`：

```powershell
python scripts/chassis-speed-capture.py emit-kf-header --kf-config kf-config.json --output build/shadow-jscope-kf/chassis_speed_kf_experiment.h
make PROFILE=atype-vehicle BUILD_DIR=build/shadow-jscope-kf CHASSIS_PREDICTIVE_POWER_MODE=1 CHASSIS_SPEED_KF_SHADOW_ENABLE=1 CHASSIS_SPEED_KF_EXPERIMENT_CONFIG=1 CHASSIS_SHADOW_JSCOPE_ENABLE=1 CHASSIS_SHADOW_JSCOPE_HZ=50 CHASSIS_SHADOW_JSCOPE_WHEEL=0 LOG_OUTPUT=1 -j8
```

新建独立目录切换编译选项。构建不会烧录。不要将 `tests/speed_observation/synthetic-kf-config.json` 当成实车标定参数。KF 配置方法见[速度观测说明](chassis-speed-observation.md)。需要同时保留 USART2 短窗采集时使用 `LOG_OUTPUT=3`，并显式开启 `CHASSIS_SPEED_CAPTURE_ENABLE=1`。

| 选项 | 范围 / 默认 |
|---|---|
| `CHASSIS_SHADOW_JSCOPE_ENABLE` | 0/1；默认 0 |
| `CHASSIS_SHADOW_JSCOPE_HZ` | 整数 1～100；默认 50 |
| `CHASSIS_SHADOW_JSCOPE_WHEEL` | 0 左前、1 右前、2 左后、3 右后；默认 0 |

低优先级观测任务每 5 ms 调度一次，输出间隔向上取整到 5 ms，频率是上限。例如 50 Hz→20 ms、100 Hz→10 ms、60 Hz→20 ms（约 50 Hz）。任务延迟、快照失效、重复快照、RTT 缓冲满会降低实际速率。与 USB 开关无关。实时波形是抽样趋势，不替代 2 ms 原始采集和预测误差对齐。

## 连接

1. 将相应固件烧录到 A 板后，用 J-Link 通过 SWD 连接。
2. 打开 J-Scope，新建工程，选择实际 MCU、SWD 接口和 RTT 采样模式；启动采样。
3. 固件上行通道 2 名称为 `JScope_t4f4f4f4f4f4f4f4f4f4u4u4`，应识别到下表的时间戳和 11 个数据字段。按顺序给曲线重命名、分配坐标轴和量程。
4. 优先对比轮速组，或候选/最终电流组；功率另用 W 量程。J-Scope 可暂停、缩放并导出数据。

J-Scope RTT-Normal 自动选择第一个 `JScope_` 通道，且最多十二个字段（含时间戳）。因此启用本功能时，原通道 1 的名称改为 `LegacyScope_f4f4f4f4`，数据布局和原发送接口保持不变；它不再自动出现在 J-Scope 中。关闭本功能并重新构建后恢复原通道 1 名称。文本日志仍使用通道 0。格式和工具行为依据 [SEGGER 官方说明](https://kb.segger.com/J-Scope)。

## 波形字段（固定 48 字节，每次整包写入）

| 顺序 | 建议名称 | 单位与含义 |
|---|---|---|
| 时间 | `time_us` | 影子快周期 `tick_ms × 1000` 的 uint32 时间戳；分辨率 1 ms，约 71.6 分钟回绕 |
| 1 | `kf_now_rpm` | KF 外推至观测时刻的转子 rpm |
| 2 | `kf_2ms_rpm` | 从该观测时刻向前 2 ms 的预测 rpm |
| 3 | `kf_8ms_rpm` | 向前 8 ms 的预测 rpm |
| 4 | `kf_20ms_rpm` | 向前 20 ms 的预测 rpm |
| 5 | `kf_alpha` | 转子角加速度 rad/s² |
| 6 | `shadow_command` | 所选轮的影子候选电流命令，DJI 原始命令单位，不是 A |
| 7 | `final_command` | 所选轮本周期实际尝试提交的命令；发送成功另看 flags bit 6 |
| 8 | `shadow_power_w` | 分配器对四轮候选的总模型功率，W；不是所选单轮功率 |
| 9 | `measured_power_w` | 快周期读取的功率计实测值，W |
| 10 | `flags` | uint32，有效位/状态，定义见下文 |
| 11 | `dropped_packets` | uint32，RTT 整包写入失败累计数；下一次成功发送时可见 |

KF 速度由 rad/s 转成 rpm；未配置、失效、超时暂停或非有限值输出 NaN，不伪造零值。电脑端如何显示 NaN 取决于 J-Scope 版本，判断有效性以 flags 为准。实测功率必须在本快周期内在线、启用、有限且帧龄 ≤20 ms。快照年龄超过 20 ms 或同一发布重复读取时不发送。

flags：bit 0 当前 KF 与加速度有效；bit 1/2/3 分别是 2/8/20 ms 预测有效；bit 4 影子候选可用；bit 5 实测功率有效；bit 6 四轮最终 CAN 组提交成功；bit 7 使用动态模型；bit 8 使用旧模型凸包络；bit 9 影子暂停。高 16 位为 `PPReason`：0 正常、1 无许可、2 无动态模型、3 额度过期、4 版本不匹配、5 超时、6 不可行、7 输入无效。bit 4 只表示影子候选可用，不表示获得实际控制权。

没有动态参数时可能看到 bit 8 和 reason=2，这代表基线包络计算，不能解释为已完成动态模型标定。KF 预测曲线的横轴是预测起点；评价 8 ms 预测准确度时，应与 8 ms 之后的反馈对齐，不能只看同一横坐标上的两条线是否接近。

## 诊断与验证

调试器读取 `shadow_jscope_diagnostics`：`init_result`（负数初始化失败）、`sent`、`dropped`、`stale`，以及打包/发送段的 `elapsed_max_us`。RTT 使用 `NO_BLOCK_SKIP`，满缓冲丢整包，不等待电脑读取。快照和 2 KiB RTT 缓冲放在静态 RAM，电机任务不执行此次发送。

软件验证入口：`make -C tests/shadow_jscope test`、`make -C tests/jscope test`、`make -C tests/predictive_power test`、`python tests/architecture/test_layers.py`。硬件连接、实际 J-Scope 显示、丢包率和任务栈高水位仍需板上验收。
