# NSEonLu A 板整车工程

基于湖南大学 RoboMaster 跃鹿战队 basic_framework 的独立 STM32F427IIH6 A 板固件。按照实车参考工程接入云台、发射、DBUS、图传键鼠与热量控制，同时保留第一套功控、超级电容、功率计、USB 遥测、在线调参、Flash 保存和 PID 自动整定。

当前交付完成软件构建与自动化验证，尚未烧录、架空测试或实车验收。接线、控制单位和待验项目见 [A 板交付说明](docs/atype-integration.md) 与 [验证记录](docs/atype-validation.md)。

## 构建

将 Arm GNU Toolchain、MinGW Make 加入 PATH；CMake 构建另需 CMake 3.16 或更新版本。

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

默认配置为单板完整整车、X 全向轮、固定 Wz=4000、超级电容供电、阶段 3 功控、功率计和缓冲管理开启。atype-direct 为直供；atype-bench 默认直供且允许无裁判 50 W 回退，锁停云台与发射，仅在明确台架条件下整定选中底盘轮。

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

- [结构体与成员索引：按中文用途、类型名或成员名查找](docs/structure-index.md)
- [配置与参数指南](docs/configuration-guide.md)
- [完整宏和初始化参数索引](docs/configuration-index.md)
- [宏迁移对照与输入基线](docs/atype-macro-migration.md)
- [接线、CAN、单位、模块与安全条件](docs/atype-integration.md)
- [验证结果和硬件验收清单](docs/atype-validation.md)

结构体索引包含项目代码、芯片驱动、第三方库、便携副本和测试桩。用 `Ctrl+F` 搜索“电机”“功率计”等用途，或搜索 `speed_aps`、`power_w` 等成员名，再沿嵌套关系跳转。成员说明沿用源码注释，未注释字段会明确标记；保留条件编译分支，因此收录不代表参与当前固件链接。

源码变化后，双击 `scripts/update-structure-index.cmd`，或执行下面的命令更新索引（Python 3.10+，仅标准库，不需要编译固件）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1
# Python 不能自动发现时指定路径：
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1 -Python "C:\path\to\python.exe"
# 只检查索引是否需要更新，不修改文件：
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/update-structure-index.ps1 -Check
```

也可跨平台执行 `python scripts/generate-structure-index.py`（检查使用 `--check`）。生成失败会返回非零状态并保留原索引；中文模块关键词在生成脚本的 `PURPOSES` 表中维护。CMD 自动化调用可设置环境变量 `STRUCTURE_INDEX_NO_PAUSE=1` 跳过窗口暂停。

```powershell
./scripts/validate-atype.ps1 -Python "<Python 3 executable>" -FirmwareMatrix -CMake
```

自动化包括主机 C 测试、Node 工具测试、16 套 Make 配置、3 套 CMake 配置及 ELF/Flash/DMA RAM 检查。外接模块在独立配置中强制链接；没有完整实现的原占位目录在模块表明确列出。编译和主机故障注入不代替实际通信、轮向、热量、功率、掉电或整定验收。

## 来源与许可

上游框架说明保留在 [YUELU.md](YUELU.md)。本项目源于 [HNUYueLuRM/basic_framework](https://github.com/HNUYueLuRM/basic_framework)，本地开发仓库来源为 [blue-has-qing/NSEonLu](https://gitee.com/blue-has-qing/NSEonLu)。项目许可证见 [LICENSE](LICENSE)；HAL、FreeRTOS、CMSIS 等组件遵循各自许可。
