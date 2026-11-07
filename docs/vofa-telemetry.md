# 用 VOFA+ 查看 A 板无线遥测

## 编译固件

`VOFA_TELEMETRY_ENABLE` 默认关闭。在仓库根目录编译，并烧录 `build/vofa-telemetry/basic_framework.hex`。

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/vofa-telemetry VOFA_TELEMETRY_ENABLE=1
```

该配置适用于速度控制模式（`CHASSIS_CONTROL_MODE=0`）。UART7 由 VOFA+ 遥测占用，裁判串口使用默认 USART6 或关闭裁判，并保持 `BOARD_AUX_UART7_DEVICE=0`。如果裁判线已经接在丝印 `7`，启用前先将其迁到 USART6。USB 遥测可同时开启，也可用 `USB_TELEMETRY_ENABLE=0` 单独运行无线遥测。[A 板串口接线](atype-integration.md#接线)

当前整车默认直供，`ch11`–`ch13` 此时无有效超电数据。已接入匹配的 F334 超电且整车使用超电供电时，沿用超电功控配置构建。`ch12` 需要收到 F334 的 CAN `0x052` 输入功率反馈。[超电协议](supercap-can-protocol.md)

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/vofa-telemetry-cap VOFA_TELEMETRY_ENABLE=1 CHASSIS_POWER_SOURCE_MODE=1
```

## 连接调试器

1. 将塔克调试器的无线主机插到电脑 USB 口，接收端设为无线从机。普通款主机的 STA 灯为蓝色，从机为绿色。若尚未配对，按手册第 2.2 节完成配对并给两端重新上电。
2. 在 A 板位号 `16` 的串口组找到最右侧、丝印 `7` 的接口。接线前按 A 板和调试器的丝印核对引脚。[UART7 接口位置](referee-uart7-fix.md)

   | A 板 UART7 | 无线从机 |
   | --- | --- |
   | `TX`（PE8） | `RX` |
   | `GND` | `GND` |
   | `5V`（从 A 板取电时） | `5V` |

   单向遥测只需要上表中的连线。调试器引脚和供电方式见《塔克无线高速 DAP 调试下载器手册 V3.0.1》第 1.4、2.4.1 节，手册推荐从机使用 5 V 供电。

3. 上电后在 Windows 设备管理器的「端口（COM 和 LPT）」里确认主机新增的虚拟 COM 号。对应示意见手册第 2.5 节。

## 在 VOFA+ 中画图

1. 连接方式选 **SerialPort**，选择调试器的虚拟 COM 号，设为 **115200、8 数据位、无校验、1 停止位、无流控**。
2. 协议引擎选 **JustFloat**，打开串口。收到完整帧后，右侧数据栏会出现 19 路数据，表中的 `ch0`–`ch18` 对应 VOFA+ 默认显示的 `I0`–`I18`。[VOFA+ JustFloat 协议](https://www.vofa.plus/docs/learning/dataengines/justfloat/)
3. 从控件栏拖入「波形图」，右键在 Y 轴菜单勾选要看的通道。例如同时勾选 `I0` 和 `I4`，即可对照左前轮电机的实测与目标转速。波形图的采样间隔 Δt 可按默认周期填写 `20 ms`。[VOFA+ 波形图操作](https://www.vofa.plus/docs/learning/widgets/wave/)

## 通道表

| 通道 | 数据 | 单位 |
| --- | --- | --- |
| `ch0`–`ch3` | LF、RF、LB、RB 电机转子实测转速 | rpm |
| `ch4`–`ch7` | LF、RF、LB、RB 电机转子目标转速 | rpm |
| `ch8` | 功率计实测功率 | W |
| `ch9` | 限制后模型估算功率 | W |
| `ch10` | 有效功率上限 | W |
| `ch11` | 超级电容输出侧底盘功率 | W |
| `ch12` | 超级电容输入功率 | W |
| `ch13` | 超级电容剩余百分比 | % |
| `ch14`–`ch16` | IMU 的 yaw、pitch、roll | ° |
| `ch17` | 电机在线位掩码，bit 0–3 对应 LF、RF、LB、RB | — |
| `ch18` | 数据有效位掩码，见下文 | — |

LF、RF、LB、RB 依次为左前、右前、左后、右后。将转子 rpm 除以 `REDUCTION_RATIO_WHEEL`（默认 19.0）即可换算为轮端 rpm。电机在线位清零时，该电机的实测和目标转速均发送 0。

`ch18` 的 bit 0 对应功控模型（`ch9`、`ch10`），bit 1 对应功率计（`ch8`），bit 2 对应电容输出与剩余百分比（`ch11`、`ch13`），bit 3 对应电容输入（`ch12`），bit 4 对应 IMU（`ch14`–`ch16`）。数据不可用时发送 0，可结合有效位区分真实零值。

每帧由 19 个小端 32 位浮点数和 `00 00 80 7F` 帧尾组成，共 80 字节。数据字节若恰好拼成帧尾，固件会调整冲突浮点数的最低尾数位，避免接收端提前切帧。发送周期跟随 `USB_TELEMETRY_PERIOD_MS`，默认 20 ms（50 Hz）。UART7 忙时跳过当期整帧。[JustFloat 解码器](https://github.com/je00/Vodka/blob/master/dataengines/justfloat/justfloat.cpp#L39-L50)
