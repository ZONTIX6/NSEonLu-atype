# 用 VOFA+ 查看 A 板无线遥测

## 编译固件

`VOFA_TELEMETRY_ENABLE` 默认关闭。启用后，用 `VOFA_TELEMETRY_MODE` 选择 UART7 输出。两档使用独立构建目录，切换时烧录对应目录的 `basic_framework.hex`。

| `VOFA_TELEMETRY_MODE` | 用途 | VOFA+ 协议 |
| --- | --- | --- |
| `0`（默认） | 原有 19 路遥测，默认 50 Hz | JustFloat |
| `1` | 38 路测量数据，每组一行，发送间隔至少 60 ms | FireWater |

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/vofa-justfloat VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=0
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/vofa-firewater VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=1
```

该配置适用于速度控制模式（`CHASSIS_CONTROL_MODE=0`）。UART7 由 VOFA+ 遥测占用，裁判串口使用默认 USART6 或关闭裁判，并保持 `BOARD_AUX_UART7_DEVICE=0`。如果裁判线已经接在丝印 `7`，启用前先将其迁到 USART6。USB 遥测可同时开启，也可用 `USB_TELEMETRY_ENABLE=0` 单独运行无线遥测。[A 板串口接线](atype-integration.md#接线)

当前整车默认直供，此时第 11–13 路无有效超电数据。已接入匹配的 F334 超电且整车使用超电供电时，在对应档位的构建命令后加 `CHASSIS_POWER_SOURCE_MODE=1`。第 12 路需要收到 F334 的 CAN `0x052` 输入功率反馈。 [超电协议](supercap-can-protocol.md)

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/vofa-firewater-cap VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=1 CHASSIS_POWER_SOURCE_MODE=1
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

## 在 VOFA+ 中连接

1. 连接方式选 **SerialPort**，选择调试器的虚拟 COM 号，设为 **115200、8 数据位、无校验、1 停止位、无流控**。
2. 按固件档位选择 **JustFloat** 或 **FireWater**，再打开串口。右侧数据栏以 `I0`、`I1` 等显示通道，点击通道名可重命名。FireWater 模式下，在字节接收区用字符串显示（关闭 `Hex`），并在 `Rx` 设置里显示采样数据包，即可看到每组完整的逗号分隔数据。 [VOFA+ FireWater 协议](https://www.vofa.plus/docs/learning/dataengines/firewater/)
3. 从控件栏拖入「波形图」，右键在 Y 轴菜单勾选要看的通道。例如同时勾选 `I0` 和 `I4`，即可对照左前轮电机的实测与目标转速。默认构建的标称 Δt 为 JustFloat `20 ms`、FireWater `60 ms`。实际间隔和缺帧可结合测量档的周期时间与帧序号查看。 [VOFA+ 波形图操作](https://www.vofa.plus/docs/learning/widgets/wave/)
4. 采完后用左上角 **文件 → 保存数据** 将采样缓冲区另存为 `.csv`。切换固件档位或开启下一次测量前，先保存上一批，再清空采样缓冲区，分别命名文件。

VOFA+ 1.3.10 接收区会按窗口宽度折行。想让 38 列在屏幕上排成一行，可关闭接收区时间戳、拉宽接收区或点 `−` 缩小字体。即使窗口里折行，每组在串口上仍只有一个行末换行，保存的 CSV 也按组分行。

## JustFloat 通道表

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

## FireWater 测量通道表

每行固定 38 个十进制整数，以逗号分隔，仅在最后一列后加一个换行（LF）。整行组好后一次提交给 UART7。发送间隔至少 60 ms，默认约 16.7 Hz。UART7 忙时跳过整行，后续行的发送序号会出现间隔。

| VOFA+ 列 | 数据 | 单位 |
| --- | --- | --- |
| `I0`–`I3` | 四轮实测转子转速 | rpm |
| `I4`–`I7` | 四轮目标转子转速 | rpm |
| `I8` | 本控制周期引用的功率计实测值 | cW，除以 100 得 W |
| `I9`–`I12` | 限制后模型功率、有效功率上限、超电输出侧底盘功率、超电输入功率 | cW |
| `I13` | 超电剩余比例 | ‰，除以 10 得 % |
| `I14`–`I16` | IMU 的 yaw、pitch、roll | 毫度，除以 1000 得 ° |
| `I17` | 四轮在线位掩码 | bit 0–3 对应 LF、RF、LB、RB |
| `I18` | 数据有效位掩码 | 见下文 |
| `I19`–`I22` | 四轮最终尝试提交的 CAN 电流命令 | DJI 原始单位 |
| `I23`–`I26` | 四轮电调反馈电流 | DJI 反馈原始单位 |
| `I27` | 限制前模型功率 | cW |
| `I28` | 裁判缓冲能量 | J |
| `I29` | 功率计样本在控制周期采集时的帧龄 | ms，未知为 `-1` |
| `I30`、`I31` | 控制周期时间戳的高、低 16 位 | ms |
| `I32`、`I33` | 发送序号的高、低 16 位 | 从 0 开始 |
| `I34` | 四轮最终使能位掩码 | bit 0–3 对应 LF、RF、LB、RB |
| `I35`、`I36` | 功率计接收事件序号的高、低 16 位 | 事件计数 |
| `I37` | 四轮对应 CAN 组提交是否成功 | `0` 或 `1` |

四轮顺序均为 LF、RF、LB、RB。三个拆分字段用 `高位 × 65536 + 低位` 还原，避免 VOFA+ 将整个 32 位整数转成单精度时丢低位。板端时钟和序号按 uint32 回绕。

`I18` 的 bit 0–4 沿用 JustFloat 的来源含义。bit 5 表示裁判缓冲有效，bit 6 表示本行有新鲜的最终控制快照，bit 7 表示该快照的控制周期计算有效。无效字段填 0，功率计帧龄未知填 `-1`。功率计的值、帧龄和序号来自控制周期读取的同一条记录。连续两行出现相同功率计序号表示引用了同一个功率计样本。`I37=1` 表示 CAN 组提交成功，电调是否执行需结合反馈判断。
