# APP 层封装实施计划

> **For agentic workers:** Use inline execution task by task; the user has approved implementation in the current chat.

**Goal:** 按职责收拢 APP 支撑代码，减少公共接口和头文件内实现。

**Architecture:** `application/board/` 管理板级装配，`application/runtime/` 管理任务与运行策略。根目录保留整车入口、共用协议和使用说明。

**Tech Stack:** C11、STM32F427、FreeRTOS/CMSIS-RTOS、GNU Make、CMake、主机 GCC、Python 标准库测试。

## 全局约束

- 不改变控制算法、参数、初始化顺序、任务优先级、栈大小与运行周期。
- 保留已存在的四个未提交文件改动，仅提交本次差异。
- 保留调试器使用的力控/功控诊断变量名称。
- 不新增运行时依赖或通用抽象框架。

## 任务 1：目录归档与任务封装

**文件：** `application/board_*.[ch]`、`application/robot_task.h`、`application/ins_bridge.[ch]`、`application/tuning_policy.[ch]`、`application/robot.c`、`Src/freertos.c`、Make/CMake、受影响测试。

**接口：** `void OSTaskInit(void)`、`bool BoardDevicesInit(void)`、`void BoardOptionalInit(void)`、`void INSBridgeTask(void)`、`void TuningPolicyInit(void)`；现有板级力控配置接口不变。

- [x] 运行现有 INS、Flash、OLED、力控测试及默认固件构建，建立基线。
- [x] 将 board 文件迁入 `application/board/`，将任务、INS 桥接、调参策略迁入 `application/runtime/`，同步调用者的限定路径和构建清单。
- [x] 将任务定义迁入 `.c`，头文件只声明 `OSTaskInit(void)`。回调和句柄改为 `static`，条件编译同步包裹 UI 回调；移除无人使用的裸 IMU getter，将保存策略回调设为 `static`。
- [x] 移除 Flash 测试对私有回调的直接调用，保留通过真实 Flash 保存路径验证安全条件的断言。
- [x] 复跑相关主机测试，构建速度、力控和可选外设分支；编译两个包含公共任务头的消费者，检查可重复包含和独立使用。

## 任务 2：文档、审查与交付

**文件：** 两份 APP 使用说明、当前 IMU/力控说明、索引生成器与生成索引、验证记录。

- [x] 补充目录职责和公共/私有边界，更新当前文档中的路径，保留历史计划。
- [x] 更新索引生成器路径提示并生成索引，单独暂存原来已有修改的文档中的本次差异。
- [x] 执行架构检查与索引测试；CMake 构建速度/力控并核对后端排除与有限浮点选项。
- [ ] 请求独立代码审查，修复有效问题，再检查暂存差异。
- [ ] 将验证结果写入 `docs/superpowers/verification/2026-10-26-application-encapsulation.md`，本地提交并报告编号。

## 验证命令

```powershell
make -C tests/ins_service -B test
make -C tests/tuning_flash -B test
make -C tests/oled -B test
make -C tests/chassis_force -B test
make -s -j6 PROFILE=atype-vehicle BUILD_DIR=build/app-encapsulation-speed all
make -s -j6 PROFILE=atype-direct CHASSIS_CONTROL_MODE=1 BUILD_DIR=build/app-encapsulation-force all
```

Python 解释器和 CMake 使用已安装工具的绝对路径。架构检查运行 `tests/architecture/test_layers.py`；索引测试运行 `python -m unittest discover -s tests/structure_index`。主机测试和完整固件编译验证本次结构整理，实车时序需上板核验。
