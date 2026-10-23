# A 板整车交付说明

日期：2026-10-22。交付主体是 `NSEonLu - atype`；参考工程保持 `b4491b9178e69c3e28fb546c3ab854cbc003ea50` 且不改写。软件验证记录见 [验证报告](atype-validation.md)。所有实物验收仍待执行。

## 模块与装配

跃鹿封装的软件阶段 A–H 已实施，行为与架构证据见 [实施记录](superpowers/plans/2026-10-22-atype-yuelu-execution.md)。功控内部持有预算、外环、瞬态和四轮模型状态；应用提交 5 ms 完整策略与目标，DJI 适配层在 2 ms 输出链执行一次四轮处理，驱动最终门控、量化和 CAN 提交后再记录最终命令。非底盘五电机不进入功控组。

模块接入、单位、所有权与失败语义见 [功控](../modules/chassis_power/chassis_power.md)、[超电](../modules/super_cap/super_cap.md)、[INS/MPU/加热](../modules/imu/ins_service.md)、[OLED](../modules/oled/oled.md)。公共功控接口不包含 DJI/HAL；超电整体快照的两帧有各自时间戳。旧功控、旧超电分项 getter 和旧 MPU 姿态实现已清除。portable 超电仍是独立可移植交付物，不进入固件，专有测试继续执行。

必要设备在 BSP 后由 `board_devices` 装配；视觉在应用 INS 桥按原 1 ms 任务组合。VTX 只验证 BSP 接收配置，OLED 显式注册 SPI/I²C 后端；ADC 按键失败返回无效。共享车辆安全状态在 modules，应用注册参数保存 guard，Flash 擦写在 BSP，写前仍在暂停调度区复核整车停机证明。

## 构建与固件

```powershell
./scripts/build-atype.ps1 -Profile atype-vehicle
./scripts/build-atype.ps1 -Profile atype-bench
./scripts/build-atype.ps1 -Profile atype-direct
```

每个目录 `build/<配置>/` 都生成 `basic_framework.elf/.hex/.bin` 和 map。Make 是主要入口；CMake 使用 `-DATYPE_PROFILE=atype-vehicle/atype-bench/atype-direct`，每次使用独立二进制目录。改动编译覆盖时 Make 的配置戳会触发对象重编译。

构建输出统一放在根目录的 `build/` 内。VS Code 的 CMake 输出为 `build/cmake-atype-vehicle/`，与代码补全的编译数据库一致；遥测/视觉专项构建使用 `build/usb-telemetry/`、`build/usb-vision/`。此前归档到 `build/legacy/` 的旧构建产物已移至回收站，当前构建和验证配置的结果保留；重新编译使用上述脚本和配置目录。

| 配置 | 电源 | 台架回退 | 非底盘执行机构 | 调试 |
| --- | --- | --- | --- | --- |
| atype-vehicle | 超级电容 | 关闭 | 完整云台与发射 | USB 遥测、RTT |
| atype-bench | 默认直供，可覆盖为电容 | 裁判无效时 50 W | 云台、摩擦轮、拨盘锁停 | 调参、保存、四轮整定 |
| atype-direct | 直供 | 关闭 | 完整云台与发射 | USB 遥测、RTT |

所有配置默认阶段 3、功率计与缓冲管理开启、X 全向轮、固定小陀螺 4000。阶段 0 保留原有限流及适用的预算保护，不是解除所有限制。自适应与正弦的参数保留，分别用 `CHASSIS_SPIN_MODE=1/2` 选择。

## 接线

MCU 为 STM32F427IIH6；12 MHz HSE、168 MHz SYSCLK、48 MHz USB。普通 RAM 192 KiB、CCM 64 KiB。DMA 缓冲与动态堆都位于普通 RAM。

| 用途 | 外设与引脚 | 参数 |
| --- | --- | --- |
| DBUS | USART1 RX PB7 | 100000、偶校验、HAL 9 位字长 |
| 视觉 | USART3 TX PD8 / RX PD9 | 9600 |
| 图传键鼠 | UART8 TX PE1 / RX PE0 | 115200 |
| 裁判 | 默认 USART6 TX PG14 / RX PG9 | 115200、8N1；编号 16 最左侧、丝印 6；`REFEREE_UART_PORT=7` 可切 UART7 PE8/PE7 |
| CAN1 | RX PD0 / TX PD1 | 1 Mbps；底盘、电容、功率计 |
| CAN2 | RX PB12 / TX PB13 | 1 Mbps；云台、发射；双板通信 |
| MPU6500 | SPI5 PF7/PF8/PF9、CS PF6 | 默认板载姿态源 |
| IMU 加热 | TIM3 CH2 / PB5 | 周期 100 µs，BSP 占空比 0～1 |
| 蜂鸣器 | TIM12 CH1 / PH6 | BSP 周期秒、占空比 0～1 |
| USB CDC | PA11 DM / PA12 DP | 默认遥测；USB 视觉与遥测互斥 |
| 可选串口日志 | USART2 TX PD5 / RX PD6 | 115200；TX 中断队列，无 TX DMA |
| 可选辅助串口 | UART7 RX PE7 / TX PE8 | 与 UART7 裁判互斥；裁判切 USART6 或关闭后可用 |
| OLED | SPI1 PB3/PB4/PA7；DC PB9、RST PB10 | 默认 SPI 后端，需显式启用 |
| OLED 模拟按键 | ADC1 IN6 / PA6 | 板级采样；转换失败不产生按键 |
| I²C 扩展 | I²C2 PF0 SDA / PF1 SCL | 支持轮询；可选 I²C OLED |
| 编码器计数 | TIM5 PH10/PH11；TIM8 PI5/PI6 | 已生成编码器模式，可轮询 CNT；不启用无处理函数的更新中断 |

电容、功率计接线及报文沿用第一套；其余 GPIO 初始输出、电源输出与第二套生成配置一致。

| UART | RX DMA / 通道 | TX DMA / 通道 |
| --- | --- | --- |
| USART1 | DMA2 S2 / C4 | 无 |
| USART3 | DMA1 S1 / C4 | DMA1 S4 / C7 |
| UART8 | DMA1 S6 / C5 | DMA1 S0 / C5 |
| USART6 | DMA2 S1 / C5 | DMA2 S6 / C5 |
| USART2 | DMA1 S5 / C4 | 中断发送 |
| UART7 | DMA1 S3 / C5 | 中断发送 |

UART7/8 接收改为 Direct mode，避免短帧尾字节滞留 FIFO。六路串口容量与 DMA/IRQ 派发保留接收实际长度。

## 电机与单位

| 电机 | CAN / 拨码 ID | 反馈 ID | 输出帧 / 槽（从 0 起） |
| --- | --- | --- | --- |
| LF | CAN1 / 1 | 0x201 | 0x200 / 0 |
| RF | CAN1 / 4 | 0x204 | 0x200 / 3 |
| LB | CAN1 / 2 | 0x202 | 0x200 / 1 |
| RB | CAN1 / 3 | 0x203 | 0x200 / 2 |
| Yaw GM6020 | CAN2 / 1 | 0x205 | 0x1FF / 0 |
| Pitch GM6020 | CAN2 / 2 | 0x206 | 0x1FF / 1 |
| 左摩擦轮 | CAN2 / 3 | 0x203 | 0x200 / 2 |
| 右摩擦轮 | CAN2 / 4 | 0x204 | 0x200 / 3 |
| 拨盘 M2006 | CAN2 / 7 | 0x207 | 0x1FF / 2 |

同一帧由第一套统一 DJI 输出链提交。只向功控注册 LF/RF/LB/RB；没有接入参考工程旧 `PowerControl()` 或旧电容 `0x301/0x302` 路径。电容 CAN1 命令/状态/输入功率分别为 `0x061/0x051/0x052`，功率计为 `0x605`。

X 型解算沿用参考：`c=√0.5`，LF=`-vx*c+vy*c+wz*L`、RF=`vx*c+vy*c+wz*L`、LB=`-vx*c-vy*c+wz*L`、RB=`vx*c-vy*c+wz*L`；各轮旋转臂沿用参考几何。输出为电机 °/s，进入功控快照时除以 6，再由功控发布接口乘 6 送入电机速度环。轮速 PID 初始 Kp 为 4.5；旧源码的 0.75 覆盖值仍保留在文件里但开关关闭。

Yaw 对齐编码器 350；Pitch 范围 −30～30°、反馈为最终映射的 Pitch 与 Gyro[1]；Yaw 使用 YawTotalAngle/Gyro[2]。拨盘 45°/发、每圈 8 发。MPU 对外角速度为 °/s，EKF 内部为 rad/s，保留参考 Pitch/Roll 映射；低温原始值按 int16 解码。初始化退化重力、SPI 失败和非有限姿态不会放行整车控制；校准中途SPI失败会终止初始化，恢复通信也不会启用未完成偏置。

## 控制与安全

电机 2 ms、整车 5 ms、INS 1 ms、守护 10 ms、USB 20 ms。功控、热控、PID 积分使用实际时间；RTT 功控诊断维持约 40 ms，而非照搬 1 ms 周期下的计数。J-Scope 使用 RTT 通道 1，文本日志使用 0。

DBUS 只接受完整 18 字节、有效通道与开关的帧；图传只在 CRC、命令与长度都正确时刷新在线。掉线、急停、无效或过期 INS、命令生产任务停顿会停止全部九电机。双板通信双向检查10 ms有效期，旧包不能反复刷新安全状态；底盘只接受显式中立复位位。恢复需输入有效，右拨杆上、摇杆和拨轮居中，并且当前控制源无键鼠输入。拨轮向下超过一半（正值 >300）进入急停。

自动整定只在 `atype-bench` 下放行：左拨杆下、右拨杆上、全部输入中立，底盘无跟随，四轮与辅助电机反馈新鲜且已停稳。仅所选底盘轮能输出；其他八电机保持零输出。取消、租约超时、输入掉线或失败后先清除输出，退出抑制需经过明确零力状态才能重置。

Flash 保存要求整车处于新鲜零力状态、全部已注册 DJI 电机停止且低速、反馈 ≤20 ms，持续 100 ms，最近控制循环 ≤5 ms，并成功发送零帧，且没有整定试验。双板时还要求来自云台板的五电机停稳证明与反馈年龄，USB 调参与整定连接底盘板；两块板必须同时更新消息布局。热量与 CAN 接收时间在同一临界区复制，避免跨包缩短年龄。

## 裁判与热控

选中的裁判串口（默认 USART6，可选 UART7）仅注册一个流式解析器，同时服务功控和热控。UART7 使用 RX DMA1 Stream3/Channel5、TX 中断，不增加 TX DMA 分配；USART6 保留原有 RX/TX DMA。`RefereeGetHeatSnapshot()` 与功率快照并列；状态与热量的时间戳分别检查，任一超过 300 ms 都无效。热量以 10 Hz 发布，消费方每次计算仍检查来源时间戳，发布停顿不会锁存有效状态。

`0x0201` 接受 13/17 字节，输出标志来自偏移 12/16。`0x0202` 接受 14/16 字节，17 mm 热量来自偏移 10/12，42 mm 来自 12/14。16 字节的 17 mm 选择严格按已确认方案；参考工程原 `_1` 在偏移 10，故此处是明确协议映射调整。帧长 16 不自动启用旧功率字段。

热控保留 0.5 s 规划窗口，预留从参考源码的一发改为七发：原参数在延迟仿真中热量达到 302/300；3200 组 0～300 ms 延迟、不同相位的逐发仿真用于验证修正。实车冷却、发弹误差与热量峰值仍需验证。

## Flash 与参数来源

固件使用前 1792 KiB；扇区 22/23（`0x081C0000/0x081E0000`）各 128 KiB 用于参数日志。schema 2 拒绝旧 C 板记录，保留 CRC、最后提交标记、扇区轮换与掉电恢复。链接断言和产物检查保证固件不进入参数区；J-Flash 限定固件扇区 0～21，普通烧录不得全片擦除。

有效参数按“版本、模式、范围均通过的 Flash → 已启用且模式匹配的源码覆盖 → 普通源码初值”恢复。运行时修改必须板端确认；未保存的修改不自动跨断电保留。网页源码同步目标仍是本工程的 `modules/telemetry/tuning_overrides.h`。

## 可选模块验证边界

`BOARD_AUX_UART2_DEVICE/BOARD_AUX_UART7_DEVICE`：0 不注册，1 TFmini，2 HC05，3 总线舵机；USART2 与 UART 日志互斥，UART7 辅助设备与 UART7 裁判互斥，同种单例不能同时占两路串口。可选模块验证配置显式使用 `REFEREE_UART_PORT=6`。`BOARD_OLED_ENABLE`、`BOARD_KEY_ENABLE` 默认 0；`OLED_USE_I2C=1` 选择保留的 I²C 后端。按键中断只记时，10 ms 任务完成消抖。

`ATYPE_LINK_MODULES=1` 强制保留 `config/atype-link-symbols.txt` 中驱动初始化及控制入口；验证脚本检查 ELF 已定义符号，避免把被链接器裁掉的代码算作验证。外接 BMI088/IST8310、HT/DM/LK、电机堵转检测、舵机、Flash 与通用 BSP 都不默认注册。SPI/I²C 扩展验证以轮询接口为基准，DMA 模式需实际配置对应句柄和不冲突的流，不能只更改实例工作模式。

原 `encoder`、`unicomm`、`step_motor`、`bsp_adc` 目录没有完整通用驱动实现，保留占位并明确记录；实际可用的编码器/ADC 走已生成 TIM5/TIM8/ADC1，通信走 CANComm/USART。没有把空文件列为可用模块。便携裁判副本同步了新帧长与热量 API，并独立测试；便携电容副本保留原来 `0x061/0x051` 子集，输入功率 `0x052` 由主工程完整实现。
