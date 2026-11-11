# A 板配置与参数指南

2026-11-11 配置整理：业务参数集中到 [`config/`](../config/README.md)，按名称升序从上层常用到下层少用排列：`01～06` 应用调参 → `20～22` 底盘控制策略 → `30～32` 观测/调试 → `90～92` 硬件 → `99_build` 构建。编号只用于查找排序，原路径的配置头仅作兼容转发。

| 常用修改 | 入口 |
| --- | --- |
| 电机/温控 PID | [01_pid/pid_config.c](../config/01_pid/pid_config.c) |
| 底盘轮型、尺寸和跟随模式 | [02_chassis/chassis_config.h](../config/02_chassis/chassis_config.h) |
| 云台机械零点和角度限幅 | [03_gimbal/gimbal_config.h](../config/03_gimbal/gimbal_config.h) |
| 发射转速/拨弹和热量 | [04_shoot/shoot_config.h](../config/04_shoot/shoot_config.h)、[heat_control_config.h](../config/04_shoot/heat_control_config.h) |
| 摇杆/键鼠灵敏度 | [05_remote/remote_config.h](../config/05_remote/remote_config.h) |
| 单/双板与视觉通道 | [06_robot/robot_config.h](../config/06_robot/robot_config.h) |
| 功率预算与外环 PI | [20_chassis_power/chassis_power_config.c](../config/20_chassis_power/chassis_power_config.c) |
| 固定/自适应/正弦自旋 | [21_chassis_spin/chassis_spin_config.h](../config/21_chassis_spin/chassis_spin_config.h) |

各配置项均注明中文用途和切换方式。表中数值是源码默认值，构建变量、源码调参覆盖和有效 Flash 保存值可能改变固件实际参数；优先级见下文。

2026-10-24 更新：跟随档默认恢复为 0；新增自适应直供自转阶段与积极缓冲参数。当前值、生效范围、旧 Flash 参数处理见 [自转功率说明](chassis-power-priority.md)。

更新日期：2026-11-11。目标 STM32F427IIH6，默认单板完整整车。完整接线、单位、控制与验收边界见 [交付说明](atype-integration.md)，全部宏、条件分支和初始化数值见 [源码索引](configuration-index.md)，输入基线与迁移理由见 [宏迁移表](atype-macro-migration.md)。

## 默认行为

| 项目 | 默认值 | 生效路径 |
| --- | --- | --- |
| 角色 | ONE_BOARD | 命令、云台、发射、底盘均初始化并调度 |
| 底盘 | OMNI_X (1) | LF/RF/LB/RB = CAN1 ID 1/4/2/3，四轮方向系数均 1 |
| 底盘速度环 | Kp 0.75，Ki/Kd 0 | 误差 °/s；源码 PID 覆盖默认关闭 |
| 自旋 | ADAPTIVE (1)，初始 8000 命令尺度 | FIXED (0)、SINE (2) 可选；显式 MODE 优先 |
| 跟随档 | CHASSIS_FOLLOW_MODE=0 | 默认恢复原比例跟转；设 1 以云台实测朝向为前方且旋转目标为零 |
| 电源 | DIRECT (0) | 改为 SUPERCAP (1) 使用第一套 0x061/0x051/0x052，CAN1 |
| 功控 | 阶段 3、功率计开、缓冲开 | 只注册四轮；阶段 0 仍有既有预算限流 |
| 台架回退 | 正式配置 0 | atype-bench 为 1，无有效裁判时 50 W |
| 视觉 | USART3 | 9600，默认不占 USB |
| USB | 遥测 1，20 ms | 视觉 VCP 配置必须关闭遥测 |
| 日志 | LOG_OUTPUT=1 | RTT 通道 0，J-Scope 通道 1 |
| 存储 | schema 2 | Flash > 源码覆盖 > 初始化值；均须模式与范围匹配 |

## 编译覆盖

```powershell
make -j8 PROFILE=atype-vehicle
make -j8 PROFILE=atype-bench
make -j8 PROFILE=atype-direct
make -j8 BUILD_DIR=build/atype-sine CHASSIS_SPIN_MODE=2
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/follow-original CHASSIS_FOLLOW_MODE=0
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/follow-gimbal-forward CHASSIS_FOLLOW_MODE=1
make -j8 BUILD_DIR=build/atype-usb-vision VISION_USE_VCP=1 USB_TELEMETRY_ENABLE=0
make -j8 BUILD_DIR=build/atype-capture LOG_OUTPUT=2 CHASSIS_POWER_CAPTURE_ENABLE=1 CHASSIS_POWER_CALIBRATION_ENABLE=1
```

Make/CMake 共用 `config/99_build/atype-options.txt` 的覆盖入口。列出的开关、机械值、缓冲和自旋参数采用 `#ifndef` 默认值；命令行定义优先。不同配置推荐独立目录；Make 的 `configuration.txt` 配置戳也会在选项改变时更新。CMake 示例：

```powershell
cmake -S . -B build/cmake-vehicle -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle
cmake --build build/cmake-vehicle -j8
cmake -S . -B build/cmake-follow-0 -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=0
cmake --build build/cmake-follow-0 -j4
cmake -S . -B build/cmake-follow-1 -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=1
cmake --build build/cmake-follow-1 -j4
```

原 `UART1_USE` 已废止，Make/CMake/预处理均会明确报错。USART1 固定 DBUS，不能切作裁判或日志。`REFEREE_ENABLE=0/1` 控制裁判功能；`REFEREE_UART_PORT` 默认 `6`，对应编号 16 最左侧、丝印 6 的 USART6（TX PG14 / RX PG9）。只允许 `6/7`；设 `7` 使用 UART7（TX PE8 / RX PE7）。UART7 未分配 TX DMA，裁判 UI 自动采用 TX 中断；USART6 使用其已有 TX DMA。`LOG_OUTPUT=0/1/2/3` 分别关闭、RTT、USART2、两者。日志 USART2 使用中断队列，不抢 UART8 RX DMA。

角色使用 `CHASSIS_BOARD=1` 或 `GIMBAL_BOARD=1` 覆盖；未指定时默认 ONE_BOARD，不能同时定义多个角色。角色与视觉通道宏按“是否定义”判断，写成 `0` 仍视为启用，切回默认应移除对应构建变量或修改 `06_robot/robot_config.h` 的默认定义。双板都需更新同一消息版本，USB 调参连接底盘板。USB 视觉和遥测、USART2 辅助设备和串口日志、单例设备重复占串口都会触发检查。

`CHASSIS_FOLLOW_MODE` 只接受 `0/1`，省略采用源码默认 `0`，独立于轮型和自旋策略；切换后重新构建并烧录。两种策略都按实测云台朝向解释平移，`1` 只取消原跟随档的主动跟转，不提供车身绝对航向锁定。遥控中档和图传 X 键沿用原入口、模式编号与云台模式；`NO_FOLLOW` 的自动整定权限及其他模式、安全输出许可均保持原样。双板部署时记录两板配置，底盘控制固件决定实际策略，详见 [底盘说明](chassis.md#跟随档策略)。

## 单位与初始化

| 范围 | 单位/规则 |
| --- | --- |
| X 型解算输出与电机 PID | °/s；功控发布边界先除以 6 得 rpm，消费者再乘 6 |
| 遥测、整定轮速 | 电机 rpm，轮序 LF/RF/LB/RB |
| 小陀螺 Wz | 原参考命令尺度，固定/自适应初始值 8000；不是 rpm 或物理角速度 |
| INS | Gyro 对外 °/s、EKF 内部 rad/s；Pitch/Roll 沿参考实车映射 |
| PWM | 秒、0～1；加热 0.1 ms→0.0001 s，百分数在适配边界除以 100 |
| 机械参数 | 轮距、轴距、半径 mm；云台角度 °，编码器值为计数 |
| 能量与功率 | J、W；积分/滤波时间 s；时间戳/超时 ms |

普通初始化值按功能位于 `config/01_pid/pid_config.c`、`config/20_chassis_power/chassis_power_config.c` 和 `config/20_chassis_power/chassis_power_model_config.h` 等文件；全部纳入源码索引。应用代码读取这些默认配置，调参无需再到各模块初始化函数内寻找数值。

## 调参、保存与整定

编译阶段，同名编译定义优先于带 `#ifndef` 的源码默认值；只有 `config/99_build/atype-options.txt` 列出的选项会由 Make/CMake 构建变量传入。其他参数直接修改对应配置源码。

启动调参恢复按“有效且模式匹配的 Flash 保存值 → 已启用且模式匹配的源码覆盖 → 普通初始化默认值”取值。[31_telemetry/tuning_overrides.h](../config/31_telemetry/tuning_overrides.h) 中 PID/功控/自旋覆盖默认关闭；只有覆盖开关开启、模式匹配且校验通过时才使用相应数组。底盘普通 PID 的当前 Kp 默认值为 0.75，修改源码不会清除旧 Flash 保存值。schema 2 拒绝旧 C 板记录。目录编号不影响这些优先级。

保存需新鲜全车零力、所有已注册 DJI 电机在线且低速停稳、零帧已发送，且无试验。双板还检查远端五电机停稳证明。台架整定使用 atype-bench；左下、右上、全部输入中立，只允许选中轮输出。退出或失败后经过零力状态重置抑制；具体操作见 [USB 工具文档](../tools/usb-telemetry/README.md)。

固件区为前 1792 KiB，参数区为扇区 22/23。普通烧录保留参数区，禁止用全片擦除保留参数。未保存的运行时修改不会因源码同步而自动持久化。

## 可选功能

辅助 UART 设备选择为 0 无、1 TFmini、2 HC05、3 总线舵机。用 `BOARD_AUX_UART7_DEVICE` 或 `BOARD_AUX_UART2_DEVICE` 注册；UART2 与日志互斥，UART7 与该端口上的裁判功能互斥。默认裁判使用 USART6，可启用 UART7 辅助设备；若显式选择 UART7 裁判，则必须关闭 UART7 辅助设备；冲突配置在编译阶段拒绝。`BOARD_OLED_ENABLE=1` 启用 SPI1 OLED，`OLED_USE_I2C=1` 切换 I²C2 后端；`BOARD_KEY_ENABLE=1` 注册板载键。

外接 BMI088/IST8310、HT/DM/LK 与通用 BSP 保留显式注册 API；`ATYPE_LINK_MODULES=1` 在验证配置中强制链接对应实现。SPI/I²C 的板级扩展以轮询为基准；DMA 模式需先配齐句柄、DMA 流及中断。编码器/ADC 可直接使用生成的 TIM5/TIM8/ADC1；原空驱动目录不计作已实现功能。

## 验证

```powershell
./scripts/validate-atype.ps1 -Python "<Python 3 executable>" -FirmwareMatrix -CMake
```

脚本会运行主机、Node、配置矩阵与链接布局检查，日志位于 `build/validation/`。Flash 掉电恢复使用主机故障注入；没有烧录或实车测试时，只能标注“软件验证通过”。
