# A 板配置与参数指南

更新日期：2026-10-23。目标 STM32F427IIH6，默认单板完整整车。完整接线、单位、控制与验收边界见 [交付说明](atype-integration.md)，全部宏、条件分支和初始化数值见 [源码索引](configuration-index.md)，输入基线与迁移理由见 [宏迁移表](atype-macro-migration.md)。

## 默认行为

| 项目 | 默认值 | 生效路径 |
| --- | --- | --- |
| 角色 | ONE_BOARD | 命令、云台、发射、底盘均初始化并调度 |
| 底盘 | OMNI_X (1) | LF/RF/LB/RB = CAN1 ID 1/4/2/3，四轮方向系数均 1 |
| 底盘速度环 | Kp 4.5，Ki/Kd 0 | 误差 °/s；源码 PID 覆盖默认关闭 |
| 自旋 | FIXED (0)，4000 命令尺度 | ADAPTIVE (1)、SINE (2) 可选；显式 MODE 优先 |
| 跟随档 | CHASSIS_FOLLOW_MODE=1 | 云台实测朝向为前方，旋转目标为零；设 0 恢复原比例跟转 |
| 电源 | SUPERCAP (1) | 第一套 0x061/0x051/0x052，CAN1 |
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

Make/CMake 共用 `config/atype-options.txt` 的覆盖入口。列出的开关、机械值、缓冲和自旋参数采用 `#ifndef` 默认值；命令行定义优先。不同配置推荐独立目录；Make 的 `configuration.txt` 配置戳也会在选项改变时更新。CMake 示例：

```powershell
cmake -S . -B build/cmake-vehicle -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle
cmake --build build/cmake-vehicle -j8
cmake -S . -B build/cmake-follow-0 -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=0
cmake --build build/cmake-follow-0 -j4
cmake -S . -B build/cmake-follow-1 -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=1
cmake --build build/cmake-follow-1 -j4
```

原 `UART1_USE` 已废止，Make/CMake/预处理均会明确报错。USART1 固定 DBUS，不能切作裁判或日志。`REFEREE_ENABLE=0/1` 控制 USART6 裁判功能；`LOG_OUTPUT=0/1/2/3` 分别关闭、RTT、USART2、两者。日志 USART2 使用中断队列，不抢 UART8 RX DMA。

角色使用 `CHASSIS_BOARD=1` 或 `GIMBAL_BOARD=1` 覆盖；未指定时默认 ONE_BOARD，不能同时定义多个角色。双板都需更新同一消息版本，USB 调参连接底盘板。USB 视觉和遥测、USART2 辅助设备和串口日志、单例设备重复占串口都会触发检查。

`CHASSIS_FOLLOW_MODE` 只接受 `0/1`，省略等价于 `1`，独立于轮型和自旋策略；切换后重新构建并烧录。两种策略都按实测云台朝向解释平移，`1` 只取消原跟随档的主动跟转，不提供车身绝对航向锁定。遥控中档和图传 X 键沿用原入口、模式编号与云台模式；`NO_FOLLOW` 的自动整定权限及其他模式、安全输出许可均保持原样。双板部署时记录两板配置，底盘控制固件决定实际策略，详见 [底盘说明](chassis.md#跟随档策略)。

## 单位与初始化

| 范围 | 单位/规则 |
| --- | --- |
| X 型解算输出与电机 PID | °/s；功控发布边界先除以 6 得 rpm，消费者再乘 6 |
| 遥测、整定轮速 | 电机 rpm，轮序 LF/RF/LB/RB |
| 小陀螺 Wz | 原参考命令尺度，固定 4000；不是新定义的物理角速度 |
| INS | Gyro 对外 °/s、EKF 内部 rad/s；Pitch/Roll 沿参考实车映射 |
| PWM | 秒、0～1；加热 0.1 ms→0.0001 s，百分数在适配边界除以 100 |
| 机械参数 | 轮距、轴距、半径 mm；云台角度 °，编码器值为计数 |
| 能量与功率 | J、W；积分/滤波时间 s；时间戳/超时 ms |

普通初始化值位于 `application/chassis/chassis.c` 的预算/慢环/功率模型结构，以及 gimbal/shoot 各自 PID 结构；它们都纳入源码索引。功控初值保留输入工作区数值；更改车辆 PID、轮序和硬件映射的理由列在迁移表。

## 调参、保存与整定

`tuning_overrides.h` 中 PID/功控/自旋覆盖默认关闭。旧 PID 数组的 0.75 没有被删除，但不会覆盖 A 板 4.5 初值。仅在覆盖开关打开、组模式匹配且数值校验通过后恢复。schema 2 明确拒绝旧 C 板 Flash 记录；新版本有效 Flash 组优先于源码覆盖。

保存需新鲜全车零力、所有已注册 DJI 电机在线且低速停稳、零帧已发送，且无试验。双板还检查远端五电机停稳证明。台架整定使用 atype-bench；左下、右上、全部输入中立，只允许选中轮输出。退出或失败后经过零力状态重置抑制；具体操作见 [USB 工具文档](../tools/usb-telemetry/README.md)。

固件区为前 1792 KiB，参数区为扇区 22/23。普通烧录保留参数区，禁止用全片擦除保留参数。未保存的运行时修改不会因源码同步而自动持久化。

## 可选功能

辅助 UART 设备选择为 0 无、1 TFmini、2 HC05、3 总线舵机。用 `BOARD_AUX_UART7_DEVICE` 或 `BOARD_AUX_UART2_DEVICE` 注册；UART2 与日志互斥。`BOARD_OLED_ENABLE=1` 启用 SPI1 OLED，`OLED_USE_I2C=1` 切换 I²C2 后端；`BOARD_KEY_ENABLE=1` 注册板载键。

外接 BMI088/IST8310、HT/DM/LK 与通用 BSP 保留显式注册 API；`ATYPE_LINK_MODULES=1` 在验证配置中强制链接对应实现。SPI/I²C 的板级扩展以轮询为基准；DMA 模式需先配齐句柄、DMA 流及中断。编码器/ADC 可直接使用生成的 TIM5/TIM8/ADC1；原空驱动目录不计作已实现功能。

## 验证

```powershell
./scripts/validate-atype.ps1 -Python "<Python 3 executable>" -FirmwareMatrix -CMake
```

脚本会运行主机、Node、配置矩阵与链接布局检查，日志位于 `build/validation/`。Flash 掉电恢复使用主机故障注入；没有烧录或实车测试时，只能标注“软件验证通过”。
