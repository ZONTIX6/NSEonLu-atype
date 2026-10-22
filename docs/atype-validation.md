# A 板融合软件验证记录

日期：2026-10-22。状态：软件验证通过；未烧录、未连接板卡、未执行架空或实车验收。本记录对应同一提交中的源码与配置。输入快照、参考版本和宏迁移见 [迁移表](atype-macro-migration.md)，接线及使用边界见 [交付说明](atype-integration.md)。

## 环境与复现

Windows / PowerShell；Arm GNU Toolchain 13.3.Rel1（GCC 13.3.1）、MinGW GCC 16.1.0、GNU Make 4.4.1、CMake 3.30.5、Node 24.15.0。CMake 使用 MinGW Makefiles 生成器。Make 保持 `-Werror`，CMake 使用 `-Wall -Werror`；主机回归按各测试 Makefile 的严格告警选项构建。

```powershell
./scripts/validate-atype.ps1 -Python "<可用的 Python 3 可执行文件>" -FirmwareMatrix -CMake
```

所有工具须加入 PATH。脚本完成 **75 项检查命令，75 项通过**：30 个主机测试目录、2 个便携模块测试目录、板级配置测试、Node 测试、16 套 Make 构建及16次产物检查、3套 CMake 的配置/构建/产物检查。配置测试含5个用例，Node含65个用例；检查命令数量不等于单元用例数量。

逐项结果保存在 `build/validation/results.json`，输出位于同目录各 `.log`。构建目录及日志是本地交付产物，不纳入源码提交。

另行通过旧Make入口构建检查，以及三项预期失败检查：Make/CMake传入旧UART1_USE均报DBUS迁移错误，USB视觉与遥测同时开启在编译时拒绝。后者日志为 `reject-make-uart1.log`、`reject-cmake-uart1.log`、`reject-usb-conflict.log`，不计入上述75项。

## 风险回归结果

| 范围 | 验证结果与证据 |
| --- | --- |
| 运动学和单位 | 零输入、平移、旋转、组合、偏角与参考公式一致；轮序LF/RF/LB/RB；aps→rpm→aps不放大；MPU温度符号、°/s和Pitch/Roll映射、公共四元数接口及加热PWM适配通过 |
| 九电机与功控隔离 | 真实DJI输出链测试九电机CAN分组；辅助电机先注册和后注册两种顺序都只限制四轮；新鲜反馈与最终零输出闸门、全车保存和整定隔离通过 |
| 安全状态 | 急停锁存、离线、中立复位、命令生产者超时、台架许可、时间回绕通过；板间CAN分片/CRC、原子快照及10ms新鲜度边界通过；双板逻辑经独立复审 |
| MPU初始化失败 | 编译实际MPU初始化与任务，硬件/EKF边界桩；第3000次SPI读取失败后，即使通信恢复也不得初始化/更新EKF或放行健康状态。修前复现失败，修后与正常校准路径均通过 |
| 裁判和热量 | 13/17字节状态及14/16字节功率热量映射、CRC、长度、分片粘包、独立来源过期与恢复通过；热量消费者每次检查300ms有效期 |
| 热量延迟 | 保留0.5s规划窗，预留改为7发；3200组0～300ms反馈延迟及不同相位逐发仿真，每组60s，均未达到300热量上限。原一发预留出现302/300，故未照搬 |
| 功率/电容/功率计 | 原模型、预算、缓冲观察、启动与持续超功率、退化恢复、采样捕获及协议回归通过；仍由第一套0x061/0x051/0x052与0x605链路处理 |
| 整定时间 | 原协议、租约、退出和质量测试通过；新增2ms与2/3ms抖动的相同500ms窗口积分，RMSE均为sqrt(1000)，不按样本数产生偏差 |
| Flash和参数 | F427扇区22/23、端点/对齐/溢出保护、CRC正确旧schema拒绝、逐字写入中断和扇区切换恢复通过；整车停止且反馈新鲜才能保存 |
| 输入和外设 | DBUS截短/无效开关、图传错误CRC/命令/长度、TFmini校验与单位、HC05发送边界通过；可选驱动初始化和控制入口强制链接并检查已定义符号 |
| 工具与配置 | 65项Node测试通过；旧UART1_USE、日志/设备冲突、无效功控阶段和无串口捕获配置被拒绝；有效覆盖和各配置实际构建通过 |

## 固件矩阵

以下各目录均生成 `basic_framework.elf/.hex/.bin` 与 map，并通过 `scripts/check-atype-artifacts.py`。

| Make目录 | 关键配置 |
| --- | --- |
| atype-vehicle | 默认完整整车、超级电容、阶段3、固定小陀螺 |
| atype-bench | 直供、50W回退、辅助执行机构锁停、台架整定 |
| atype-direct | 完整整车直供 |
| atype-stage0 / stage1 / stage2 | 功控阶段0/1/2；阶段0仍有既有预算限流 |
| atype-adaptive / sine | 可选自适应/正弦小陀螺 |
| atype-no-meter | 关闭功率计 |
| atype-usb-vision | USB视觉开启、USB遥测关闭 |
| atype-capture | 捕获和校准开启，LOG_OUTPUT=2 |
| atype-chassis / gimbal / dual-bench | 双板两种角色、底盘台架角色 |
| atype-optional | 强制链接外接驱动、SPI OLED、按键、UART7 TFmini、UART2 HC05 |
| atype-i2c-oled | 强制链接外接驱动、I²C OLED、UART7总线舵机 |

CMake的 `atype-vehicle`、`atype-bench`、`atype-direct` 分别在 `build/cmake-<配置>` 通过。旧 `Makefile.upgrade` 入口转为复用主Makefile，避免维护另一份过期源文件清单。

链接检查确认：F427栈顶0x20030000；参数区0x081C0000/0x081E0000；无固件段进入参数区；`.data/.bss`位于DMA可访问普通RAM；无未定义强符号；当前INS和USB/UART8中断入口存在。可选配置另检查 `config/atype-link-symbols.txt` 的所有实现入口实际保留。

| 配置 | Flash镜像 B | 静态RAM B | 含链接预留堆栈RAM B |
| --- | ---: | ---: | ---: |
| atype-vehicle | 143260 | 101792 | 142752 |
| atype-bench | 142388 | 101784 | 142744 |
| atype-direct | 142164 | 101784 | 142744 |
| atype-capture | 146892 | 116296 | 157256 |
| atype-optional | 163536 | 103088 | 144048 |

普通RAM总量196608 B。静态RAM已包含FreeRTOS 64KiB堆；最后一列另计链接脚本20KiB堆、20KiB栈预留，不能把它理解为实测任务栈占用。CCM 64KiB未用于DMA数据。任务栈余量、实时执行时间和长期资源余量仍须上板测量。

主要BIN产物的SHA256：

| 文件 | SHA256 |
| --- | --- |
| `build/atype-vehicle/basic_framework.bin` | `d863ea41d1b36a8689820fe53ac81dc5d72cdcd5045570d63c53a9b0c38296bf` |
| `build/atype-bench/basic_framework.bin` | `034b36168e423d0d21269741d7e6bd81ef6ec0262afcde0a6054092056758abf` |
| `build/atype-direct/basic_framework.bin` | `a181ca3f71d694e80a427acd7cb5d9e5d502c67a758be68ccaf52aacd69df613` |

## 未执行的验收

以下全部为**待硬件验证**，本次没有标记任何“实车通过”：

| 顺序 | 待验内容 | 应保存的记录 |
| --- | --- | --- |
| 1 | MCU识别、复位、时钟、RTT、电源/GPIO初态、USB枚举 | 固件SHA、接线、启动日志、枚举结果 |
| 2 | MPU校准、加热、温度、轴向、静漂和云台反馈方向 | 温度/姿态曲线、失败处理与方向记录 |
| 3 | DBUS、图传、视觉、裁判、电容、功率计、九电机及双板通信 | 帧率、超时、CRC/DMA错误、双板断链停机与复位 |
| 4 | 架空逐轮核对ID、轮位、方向、参考、反馈及电流 | 四轮单独与组合运动记录 |
| 5 | 平移、跟随、固定旋转、云台及发射行为回归 | 参考行为对照、机械限位、热量峰值 |
| 6 | 低功率起步、增载、电容供电、缓冲消耗/恢复、异常输入 | 功率计/电容/裁判/控制同步曲线 |
| 7 | USB连续采样、调参、保存重启、实际掉电、整定及退出 | 参数来源、记录恢复、退出零输出证据 |
| 8 | 长时间运行及各外接模块实际设备验证 | 任务耗时、栈余量、CAN/UART丢帧和USB稳定性 |

另外，`.ioc`已按运行源码修正并保留主工程USB组织，但本机未执行CubeMX重生成；后续需在独立副本进行生成前后对照，特别核对时钟、USB、UART8、DMA/IRQ、FreeRTOS和用户代码。SPI/I²C扩展DMA模式未预分配全部硬件资源，当前可选验证以轮询为基准。原有空占位目录未补造通用驱动，其可用替代与边界见交付说明。

本次只修改第一套工程。参考工程最终仍为 `b4491b9178e69c3e28fb546c3ab854cbc003ea50` 且工作区干净；初始8个用户修改文件的原文保存在本地 `codex/atype-input-baseline` 提交。历史C板验证文档保留原始实验背景，当前构建与接线以本报告、配置指南和交付说明为准。
