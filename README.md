# NSEonLu A 板整车工程

基于湖南大学 RoboMaster 跃鹿战队 basic_framework 的独立 STM32F427IIH6 A 板固件。按照实车参考工程接入云台、发射、DBUS、图传键鼠与热量控制，同时保留第一套功控、超级电容、功率计、USB 遥测、在线调参、Flash 保存和 PID 自动整定。

当前交付完成软件构建与自动化验证，尚未烧录、架空测试或实车验收。接线、控制单位和待验项目见 [A 板交付说明](docs/atype-integration.md) 与 [验证记录](docs/atype-validation.md)。

## 构建

将 Arm GNU Toolchain、MinGW Make 加入 PATH；CMake 构建另需 CMake 3.16 或更新版本。

主板构建与烧录使用 [`config/00_flash/board_flash_config.h`](config/00_flash/board_flash_config.h)：目标为主板 `1`，下载器、profile 和输出目录集中在该文件。第二板采集固件及预设已移除。

主板预设开启 `CHASSIS_REMOTE_IMU_FUSION_ENABLE=1`：刚性固定在底盘的 **DM-IMU-L1 经 CAN2** 参与在线卡尔曼**速度观测**，结果和轮速一同由 UART7 输出；电机控制、PID、功控和安全门控仍使用原输入。安装旋转、芯片偏移与噪声参数见 [远端 IMU 配置](config/30_velocity_observation/chassis_remote_imu_config.h)，默认单位旋转、零偏移是名义安装，`CHASSIS_REMOTE_IMU_CONFIG_VERIFIED=0` 表示尚未实测审核。DM 时间与序号由主板接收端估计和生成，不是模块采样时钟；融合运行不等于精度已验收。模块配置、接线与首次核对见 [DM-IMU-L1 集成说明](docs/dm-imu-integration.md)。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Show
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Build
# 连接所选物理板；成功构建后再烧录
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Flash
```

VS Code 也提供 `show selected board`、`build selected board`、`flash selected board` 三个任务。[主板构建说明](config/README.md#主板怎样构建与烧录)列出下载器选择方法。下面的手动 Make/CMake、原有任务与 F5 调试入口继续使用各自参数，不读取此切换文件。

```powershell
make -j8 PROFILE=atype-vehicle
make -j8 PROFILE=atype-bench
make -j8 PROFILE=atype-direct
```

也可运行 `scripts/build-atype.ps1 -Profile atype-vehicle`。每套配置生成 `build/<配置>/basic_framework.elf/.hex/.bin`；Make 会记录配置戳，选项改变后重编译。CMake 提供等效配置：

```powershell
cmake -S . -B build/cmake-vehicle -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle
cmake --build build/cmake-vehicle -j8
```

源码默认配置为单板完整整车、X 全向轮、自适应小陀螺（初始 Wz=8000，沿用工程命令尺度）、直供、阶段 3 功控、功率计和缓冲管理开启；实际参数还受构建覆盖和有效保存值影响。超级电容需显式设置 `CHASSIS_POWER_SOURCE_MODE=1`；atype-direct 为直供；atype-bench 默认直供且允许无裁判 50 W 回退，锁停云台与发射，仅在明确台架条件下整定选中底盘轮。

USART1 固定 DBUS；视觉 USART3、图传 UART8、裁判默认 USART6（编号 16 最左侧、丝印 6）；日志默认 RTT，可选 USART2 非阻塞中断输出。裁判 USART6 使用 RX/TX DMA；`REFEREE_UART_PORT=7` 可切换到 UART7（RX DMA、TX 中断），UART7 裁判与辅助设备同时启用会编译报错。接口对应及本次故障见 [裁判通讯修复说明](docs/referee-uart7-fix.md)。旧 `UART1_USE` 构建参数会报错，替代选项见 [配置指南](docs/configuration-guide.md)。USB 默认用于网页遥测；改用 USB 视觉时同时设置 `VISION_USE_VCP=1 USB_TELEMETRY_ENABLE=0`。

## 调试与烧录

```powershell
make download_dap BUILD_DIR=build/atype-vehicle
# 或已安装 J-Flash
make download_jlink BUILD_DIR=build/atype-vehicle
node tools/usb-telemetry/serve.cjs
```

先完成构建，再选择对应目录烧录。应用只占前 1792 KiB，参数区为 F427 扇区 22/23（0x081C0000 / 0x081E0000），J-Flash 限定固件扇区 0～21；保留参数时不要全片擦除。网页服务地址为 [http://127.0.0.1:4173](http://127.0.0.1:4173)。

参数恢复顺序为有效新版本 Flash、已启用且模式匹配的源码覆盖、普通初始化值。首次融合禁用旧 PID 覆盖并拒绝 C 板存储记录。保存要求全部九电机停稳；整定只能在台架配置进行。操作说明见 [USB 仪表盘](tools/usb-telemetry/README.md)。离线 CSV 查看见 [波形工具](tools/waveform-viewer/README.md)。

## 配置与验证

`config/` 按名称升序排列：`00_flash` 是主板构建与烧录配置，`01～06` 是最常用的应用调参，`20～22` 是底盘控制策略，`30～32` 是观测和调试，`90～92` 是硬件配置，`99_build` 是较少修改的构建入口。目录编号只决定查找顺序，不决定参数覆盖顺序。

| 常用调整 | 直接入口 |
| --- | --- |
| 主板构建与烧录 | [00_flash/board_flash_config.h](config/00_flash/board_flash_config.h) |
| PID 增益与限幅 | [01_pid/pid_config.c](config/01_pid/pid_config.c) |
| 底盘尺寸、轮向、随动模式 | [02_chassis/chassis_config.h](config/02_chassis/chassis_config.h) |
| 云台限位和机械零点 | [03_gimbal/gimbal_config.h](config/03_gimbal/gimbal_config.h) |
| 摩擦轮、拨弹和热量 | [04_shoot/shoot_config.h](config/04_shoot/shoot_config.h)、[heat_control_config.h](config/04_shoot/heat_control_config.h) |
| 遥控器/键鼠手感 | [05_remote/remote_config.h](config/05_remote/remote_config.h) |
| 底盘 IMU 融合安装、噪声与时间门槛 | [30_velocity_observation/chassis_remote_imu_config.h](config/30_velocity_observation/chassis_remote_imu_config.h) |

开关取值和切换方式写在各配置项旁；完整分层目录、Make/CMake 示例和 Flash/源码优先级见 [配置目录说明](config/README.md)。

- [集中配置与调参入口：PID、底盘、云台、发射、功率、IMU](config/README.md)
- [常用指针与宏速查：遥控器、电机、功率计、超级电容、裁判系统等](.Doc/指针查找索引.md)
- [结构体与成员索引：查字段类型、说明与嵌套定义](docs/structure-index.md)
- [配置与参数指南](docs/configuration-guide.md)
- [完整宏和初始化参数索引](docs/configuration-index.md)
- [宏迁移对照与输入基线](docs/atype-macro-migration.md)
- [接线、CAN、单位、模块与安全条件](docs/atype-integration.md)
- [验证结果和硬件验收清单](docs/atype-validation.md)
- [DM-IMU-L1 接 CAN2：接线、模块设置与来源核对](docs/dm-imu-integration.md)
- [整车平移速度融合观测、标定和验证](docs/chassis-velocity-fusion.md)
- [卡尔曼底盘实车测试顺序与验收手册](docs/kalman-chassis-test-and-acceptance.md)
- [底盘功率控制总览：模型、计算、分配与超级电容](docs/chassis-power-control.md)

日常查数据和模式，打开“常用指针与宏速查”，用 `Ctrl+F` 搜中文用途、指针名、成员名或模式名即可。表中列出常用指针访问示例、编译模式开关，以及底盘、云台、发射、摩擦轮和拨弹的运行时模式值与赋值字段。宏的源码默认值不等于构建覆盖后或当前固件的实际值。字段的完整定义另见类型索引。

源码变化后，双击 `scripts/update-structure-index.cmd`，或执行下面的命令同时更新 `.Doc/指针查找索引.md` 和类型索引（Python 3.10+，仅标准库，不需要编译固件）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1
# Python 不能自动发现时指定路径：
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1 -Python "C:\path\to\python.exe"
# 只检查索引是否需要更新，不修改文件：
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1 -Check
```

也可跨平台分别执行 `python scripts/generate-structure-index.py` 和 `python scripts/generate-data-instance-index.py`（检查使用 `--check`）。常用指针、宏和模式名单在后者的 `COMMON_DATA`、`COMMON_MACROS`、`COMMON_MODE_MACROS`、`OPTIONAL_MODE_MACROS`、`COMMON_MODES` 中维护；力控编译模式在对应头文件存在、且接入 `config/99_build/atype-options.txt` 构建选项后自动列出。缺失已收录的宏或模式常量会明确报错并保留原文档。加 `--full` 可另行生成完整实例表。每份文档在成功生成后原子替换，生成失败返回非零状态。CMD 自动化调用可设置环境变量 `STRUCTURE_INDEX_NO_PAUSE=1` 跳过窗口暂停。

```powershell
./scripts/validate-atype.ps1 -Python "<Python 3 executable>" -FirmwareMatrix -CMake
```

自动化包括主机 C 测试、Node 工具测试、16 套 Make 配置、3 套 CMake 配置及 ELF/Flash/DMA RAM 检查。外接模块在独立配置中强制链接；没有完整实现的原占位目录在模块表明确列出。编译和主机故障注入不代替实际通信、轮向、热量、功率、掉电或整定验收。

## 来源与许可

上游框架说明保留在 [YUELU.md](YUELU.md)。本项目源于 [HNUYueLuRM/basic_framework](https://github.com/HNUYueLuRM/basic_framework)，本地开发仓库来源为 [blue-has-qing/NSEonLu](https://gitee.com/blue-has-qing/NSEonLu)。项目许可证见 [LICENSE](LICENSE)；HAL、FreeRTOS、CMSIS 等组件遵循各自许可。
