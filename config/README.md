# 配置与调参入口

本目录按“上层应用、常用调参 → 控制策略 → 观测与调试 → 底层硬件 → 构建”编号排列。在文件树中按名称升序查看即可保持这一顺序。编号表示查找优先级，不表示代码执行顺序或参数覆盖优先级；文件仍保留 `pid_config.c` 等功能名称。

宏、编译开关和声明放在 `.h`，PID 和有类型的默认配置放在 `.c`；只有宏的配置不额外建立空 `.c`。各参数旁有中文用途、单位和切换说明，修改后重新编译并烧录。

| 显示顺序 | 用途 | 调参文件 |
| --- | --- | --- |
| `_vofa` | VOFA 功率/卡尔曼日常切换 | [vofa_config.h](_vofa/vofa_config.h) |
| `00_flash` | 主板构建/烧录、下载器与输出目录 | [board_flash_config.h](00_flash/board_flash_config.h) |
| `01_pid` | 最常用：底盘、云台、摩擦轮、拨弹和 IMU 温控 PID | [pid_config.c](01_pid/pid_config.c)、[pid_config.h](01_pid/pid_config.h) |
| `02_chassis` | 底盘轮型、跟随模式、尺寸、减速比和轮向 | [chassis_config.h](02_chassis/chassis_config.h) |
| `03_gimbal` | 云台机械零点、Pitch 限角和 Yaw 跟踪限幅 | [gimbal_config.h](03_gimbal/gimbal_config.h) |
| `04_shoot` | 摩擦轮目标、拨弹参数与发射热量策略 | [shoot_config.h](04_shoot/shoot_config.h)、[heat_control_config.h](04_shoot/heat_control_config.h) |
| `05_remote` | 摇杆/键鼠灵敏度、速度档位、输入超时 | [remote_config.h](05_remote/remote_config.h) |
| `06_robot` | 上层整车组织：单/双板角色、视觉通信方式 | [robot_config.h](06_robot/robot_config.h) |
| `20_chassis_power` | 功率阶段、预算、外环 PI、供电/缓冲与功率模型 | [chassis_power_config.c](20_chassis_power/chassis_power_config.c)、[chassis_power_config.h](20_chassis_power/chassis_power_config.h)、[chassis_energy_config.h](20_chassis_power/chassis_energy_config.h)、[chassis_power_model_config.h](20_chassis_power/chassis_power_model_config.h) |
| `21_chassis_spin` | 固定、自适应和正弦小陀螺策略 | [chassis_spin_config.c](21_chassis_spin/chassis_spin_config.c)、[chassis_spin_config.h](21_chassis_spin/chassis_spin_config.h) |
| `22_chassis_force` | 速度/力控选择、力控通用值、车辆标定和标定工具 | [chassis_control_config.h](22_chassis_force/chassis_control_config.h)、[chassis_force_config.c](22_chassis_force/chassis_force_config.c)、[chassis_force_config.h](22_chassis_force/chassis_force_config.h)、[chassis_force_vehicle_config.c](22_chassis_force/chassis_force_vehicle_config.c)、[chassis_force_vehicle_config.h](22_chassis_force/chassis_force_vehicle_config.h)、[force_calibration_config.h](22_chassis_force/force_calibration_config.h) |
| `30_velocity_observation` | 速度时效、远端 IMU 安装/融合、预测功率和轮速影子滤波 | [chassis_velocity_config.h](30_velocity_observation/chassis_velocity_config.h)、[chassis_remote_imu_config.h](30_velocity_observation/chassis_remote_imu_config.h)、[predictive_power_config.h](30_velocity_observation/predictive_power_config.h)、[speed_observation_config.h](30_velocity_observation/speed_observation_config.h) |
| `31_telemetry` | USB/RTT 遥测、标定 CSV 字段与缓冲、网页源码覆盖值 | [usb_telemetry_config.h](31_telemetry/usb_telemetry_config.h)、[rtt_telemetry_config.h](31_telemetry/rtt_telemetry_config.h)、[tuning_overrides.h](31_telemetry/tuning_overrides.h) |
| `32_capture` | VOFA 新档通道定义、本地 IMU 诊断、双板 CAN 采集与 J-Scope 影子观测 | [vofa_schema.json](32_capture/vofa_schema.json)、[imu_capture_config.h](32_capture/imu_capture_config.h)、[shadow_scope_config.h](32_capture/shadow_scope_config.h) |
| `90_board` | 板级绑定/可选设备、裁判旧协议与超时、日志启停 | [board_config.h](90_board/board_config.h)、[referee_config.h](90_board/referee_config.h)、[log_config.h](90_board/log_config.h) |
| `91_motor` | DJI 电机数量、反馈滤波与驱动参数 | [dji_motor_config.h](91_motor/dji_motor_config.h) |
| `92_imu` | A 板本地 IMU 采样、温控与姿态参数 | [imu_config.h](92_imu/imu_config.h) |
| `99_build` | 较少修改：构建配置、允许覆盖的宏、强制链接符号 | [atype.mk](99_build/atype.mk)、[atype-options.txt](99_build/atype-options.txt)、[atype-link-symbols.txt](99_build/atype-link-symbols.txt) |

## 主板怎样构建与烧录

打开 [`00_flash/board_flash_config.h`](00_flash/board_flash_config.h)，`BOARD_FLASH_1_` 为整车主板，`BOARD_FLASH_2_` 为下方 IMU 采集板。`BOARD_FLASH_TARGET` 或 `-Board 1/2` 选择目标，两板使用独立构建目录。`BOARD_FLASH_PROGRAMMER` 选 `1` 使用 DAP/OpenOCD，选 `2` 使用 J-Link/J-Flash；脚本不会识别实际连接的物理板。

日常只修改 [`_vofa/vofa_config.h`](_vofa/vofa_config.h) 中的 `VOFA_OBSERVATION_PROFILE`：**`1`=功率/超电，`2`=卡尔曼（默认）**，保持 `00_flash/board_flash_config.h` 中的 `BOARD_FLASH_1_VOFA_TELEMETRY_MODE=2`。两档分别输出新 schema 4/5，各 96 路，UART7 JustFloat、230400 / 8N1、20 ms 目标周期。切换后重新构建和烧录，VOFA+ 直接导出完整原始 CSV，不需要手动列名映射或单位换算。通道顺序和单位以 [`32_capture/vofa_schema.json`](32_capture/vofa_schema.json) 为准；[操作说明与两套采集动作](../docs/vofa-telemetry.md#csv-采集与交付)列出简短步骤。

`VOFA_OBSERVATION_PROFILE=0` 保留旧 schema 2/3；传输模式 `0/1` 忽略用途选项，继续使用旧 19 路 JustFloat/64 或 72 路 FireWater。烧录脚本和手动 Make/CMake 共用 `_vofa/vofa_config.h` 的默认档位。显式 `VOFA_OBSERVATION_PROFILE=1/2`、CMake 的 `-DVOFA_OBSERVATION_PROFILE=1/2`，或自定义 `-ConfigFile` 中的旧 `BOARD_FLASH_1_VOFA_OBSERVATION_PROFILE` 会覆盖默认值；日常无需添加这些覆盖。已有 CMake 缓存若设过该选项，须用 `-UVOFA_OBSERVATION_PROFILE` 重新配置才能恢复读取头文件默认值。新旧 schema 的布局和单位不同，不能混用解析表。

主板预设 `CHASSIS_IMU_CAN_RX_ENABLE=1` 接收第二块 A 板的 CAN 数据（默认 CAN2），`CHASSIS_REMOTE_IMU_FUSION_ENABLE=1` 开启只读速度融合，不接管控制。第二板使用 `IMU_CAPTURE_ONLY=1 IMU_CAPTURE_CAN_TX_ENABLE=1`，仅采样、温控和发送，不运行整车任务。`CHASSIS_VELOCITY_FUSION_CALIBRATED=0` 表示未启用本地云台标定路径。需要轮速基线时只关闭主板远端融合，保留第二板发送和主板接收。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Show -Board 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Build -Board 1
# 构建成功、确认下载器连接主板后执行
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Flash -Board 1
```

VS Code 默认 `build task` 委托 `build selected board` 读取所选板卡预设；`download dap` / `download jlink` 通过脚本的 `-Programmer DAP/JLink` 指定下载器，并按 `BOARD_FLASH_TARGET` 选择板卡。`show selected board`、`build selected board`、`flash selected board` 继续使用配置文件中的选择。`Flash` 先构建，失败时停止；脚本不会识别实际插入的硬件。

F5 的 ST-Link / DAPlink / Jlink 跟随 `BOARD_FLASH_TARGET`：设为 `2` 并保存后，选择 Jlink 按 F5 就会构建、加载第二块 IMU 板固件；设回 `1` 则使用整车主板固件。`build selected board for debug` 调用 `-Action Debug`，成功构建后将所选 ELF 复制为 `build/selected-board.elf`；准备失败会清除旧副本，避免沿用上一块板的固件。下载器由调试菜单决定，`BOARD_FLASH_PROGRAMMER` 只选择脚本 Flash 的下载器。这些 F5 入口无需随两板输出目录修改 ELF 路径。

`Jlink-attach` 通过 `-Action Symbols` 准备 `BOARD_FLASH_TARGET` 所选板卡已有 ELF；`DAP-attach` 固定加载第一块主板的 `build/board-1-main/basic_framework.elf`，不受 `BOARD_FLASH_TARGET` 影响。修改 `BOARD_FLASH_1_BUILD_DIR` 时需同步修改 `.vscode/launch.json` 中 `DAP-attach` 的 ELF 路径。两种 attach 均不构建、不烧录，已有 ELF 须与板上固件一致。F5 与 `Jlink-attach` 共用副本，切换板卡前结束使用该副本的调试会话。手动 Make/CMake 需显式传接收/融合等参数。两板接线与构建见 [集成说明](../docs/dual-a-imu-integration.md)。对第二板使用同样的 Show/Build/Flash 命令并改为 `-Board 2`。

`IMU_CAPTURE_ONLY=1` 仅保留本板 IMU 采样与温控；搭配 `IMU_CAPTURE_CAN_TX_ENABLE=1` 通过所选 CAN 发送（默认 CAN2），搭配 UART 遥测则可做本地诊断。采集板不得同时启用整车 CAN 接收或远端融合。

## 常用调参步骤

1. 调 PID：打开 `01_pid/pid_config.c`。`pid_chassis_speed_config` 是四个底盘轮的启动默认参数，`pid_gimbal_yaw_speed_config` 是云台 Yaw 速度环；按各环注释的单位调整 `Kp`、`Ki`、`Kd`、`MaxOut` 和 `IntegralLimit`。
2. 调运动与手感：底盘尺寸/跟随模式在 `02_chassis`，云台限位在 `03_gimbal`，摇杆和键鼠增益在 `05_remote`。
3. 调发射：摩擦轮目标转速和拨弹参数在 `04_shoot/shoot_config.h`，单发热量、预留弹数和射频规划在同目录 `heat_control_config.h`。
4. 调功率/自旋：功率外环独立 PI 的 `kp`、`ki` 在 `20_chassis_power/chassis_power_config.c` 的 `chassis_outer_loop_config`；小陀螺参数在 `21_chassis_spin`。这些不是电机 PID 参数。
5. 调底盘 IMU 融合：在 `30_velocity_observation/chassis_remote_imu_config.h` 配置安装、芯片偏移、滤波噪声与时间门槛。远端融合以底盘中心为原点，轴向直接采用 A 板原生 IMU：当前照片中 +X 向左、+Y 向下、+Z 离开板面。安装四元数为 `[1,0,0,0]`，中心到芯片的偏移为 `(0,+0.116,0) m`，116 mm 已确认量到芯片。现有前/右速度接口由融合边界换算，具体见 [双板坐标说明](../docs/dual-a-imu-integration.md#安装坐标)。完成实测、独立验证并冻结参数后，才把 `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` 改为 `1`；此标识只记录审核声明，不控制算法是否运行，也不代替验收。

板间 CAN 由 [`90_board/board_config.h`](90_board/board_config.h) 中的 `CHASSIS_IMU_CAN_PORT` 选择：`1`=CAN1，`2`=CAN2（默认）。两板预设共用该头文件；改值后分别重新构建、烧录两板，并连接所选接口。手动 Make/CMake 可用同名构建参数覆盖，具体见 [接线与构建](../docs/dual-a-imu-integration.md#接线与构建)。速率仍为 1 Mbps，标准 ID 为 `0x520`，采集任务名义每 20 ms 尝试发送一组。主板使用源板 INS 已去重力的导航系加速度，只做安装与中心偏移补偿，不再使用 DM 模块的重力常量或协议换算。

MPU6500 与旧 BMI088 温控 PID 在 `01_pid/pid_config.c` 中分别命名，当前 A 板使用 MPU6500；温度目标和 IMU 安装参数在 `92_imu/imu_config.h`。

## 宏怎样切换

普通数值参数直接修改对应文件中的初值；宏按旁边列出的取值切换。`CHASSIS_CONTROL_MODE`（速度/力控）和 `CHASSIS_FORCE_CALIBRATION_ENABLE`（力控标定工具）还会影响编译源文件选择，应通过 Make/CMake 构建参数切换。带 `#ifndef` 的宏可由编译定义覆盖；Make/CMake 接受的变量名单在 `99_build/atype-options.txt`，不是所有数值宏都自动支持构建变量。

首次使用本次整理后的目录，请选择新的 `BUILD_DIR`（例如下面的命令），或先清理旧构建目录。旧 Make `.d` 缓存可能仍引用迁移前的源文件路径；不要直接烧录旧目录中遗留的固件。

```powershell
# 随动档：0=车身跟转，1=按云台朝向平移且不主动跟转
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/follow-gimbal CHASSIS_FOLLOW_MODE=1
# 自旋：0=固定，1=自适应，2=正弦
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/spin-sine CHASSIS_SPIN_MODE=2
# 控制后端：0=速度控制，1=直接力控；选择力控还需按22_chassis_force中的说明完成标定
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/force-control CHASSIS_CONTROL_MODE=1
# USB 视觉与 USB 遥测互斥，启用前者必须关闭后者
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/usb-vision VISION_USE_VCP=1 USB_TELEMETRY_ENABLE=0
# CMake 使用同名 -D 参数
cmake -S . -B build/cmake-follow-gimbal -G "MinGW Makefiles" -DCHASSIS_FOLLOW_MODE=1
cmake --build build/cmake-follow-gimbal -j8
```

`ONE_BOARD`、`CHASSIS_BOARD`、`GIMBAL_BOARD` 以及视觉通道宏按“是否定义”判断，写成 `0` 仍视为已定义。角色只选一个；未指定时默认单板。双板可分别传 `CHASSIS_BOARD=1` 和 `GIMBAL_BOARD=1` 编译两套固件，不能同时传入。其他 0/1 开关按文件注释切换，多个值的模式宏按其列出的映射选择。

## 参数优先级与兼容入口

编译阶段，显式编译定义优先于带 `#ifndef` 的源码默认值；构建 profile 也可能提供默认选项，应检查实际构建命令和输出目录。编号只用于文件排序，不改变这些规则。

底盘在线调参的启动恢复优先级从高到低为：有效且模式匹配的 Flash 保存值 → 启用且模式匹配的 `31_telemetry/tuning_overrides.h` → 普通配置默认值。在线调参修改运行副本；修改默认值不会自动清除已有 Flash 参数。网页“同步源码”写入 `31_telemetry/tuning_overrides.h`，重新编译烧录后才成为启动覆盖值，也不会自动替代已有有效 Flash 记录。

Make 和 CMake 会包含各编号子目录，并按速度/力控模式选择配置源文件。原 `Inc/` 和 `modules/` 中的同名配置头仅作兼容转发，实际参数以本目录为准。协议常量、寄存器定义、数学换算、算法实现，以及 Cube/HAL/FreeRTOS 配置保留原模块；可选本地云台融合的实测标定头仍从对应构建目录提供，固定底盘远端 IMU 使用上述独立配置文件。
