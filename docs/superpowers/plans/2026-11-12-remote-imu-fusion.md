# Remote IMU Fusion Implementation Plan

> Execution uses the available collaboration tools for isolated file ownership and independent review; all workers share this checkout.

**Goal:** 第二块底盘 IMU 真正参与在线卡尔曼速度观测，保持控制隔离，并更新实测验收资料。

**Architecture:** 新增纯数值远端适配器，轮速历史配对、源时钟积分、固定安装中心补偿，再调用已有 KF。应用层读取 CAN 一致快照并发布融合快照；FireWater 保持 72 列。

**Tech Stack:** C11、STM32F427、FreeRTOS、GCC 主机测试、Make/CMake、PowerShell。

## Global Constraints

- 不改变电机目标/PID/功控/安全控制的输入来源；不烧录硬件。
- CAN 协议和 schema 3 列布局不变，跨板时间估计不冒充同步。
- 默认安装及噪声为未验证初值，独立标识；固定容量、无堆分配。
- 保留用户的 `config/90_board/board_config.h`、`docs/configuration-index.md` 及其他既有改动。

## Task 1：远端数值模块

Files: `modules/algorithm/chassis_remote_fusion.[ch]`, `tests/chassis_remote_fusion/`。

接口：`ChassisRemoteFusionUpdate(state, config, wheel, wheel_skew_ms, remote, remote_usable, now_ms, output)`；类型见新模块头，底层输入为 `ChassisVelocity_s` 与 `ChassisImuCanSnapshot_s`。

- [x] 先运行失败测试：连续20ms远端样本在加速度变化时输出 FUSED，融合速度与纯轮速不同；相同源包更新计数与输出时刻不变。
- [x] 实现32项轮速历史、有界配对、固定姿态/杆臂、源时钟三点补偿、KF以及故障恢复。
- [x] `make -B -C tests/chassis_remote_fusion test`，覆盖坐标/时序/缺失/非有限数/恢复。

## Task 2：应用集成与配置

Files: `application/chassis/chassis_velocity_observer.[ch]`, `config/30_velocity_observation/chassis_remote_imu_config.h`, `config/32_capture/imu_capture_config.h`, `tests/chassis_velocity_observer/`。

- [x] 先增加远端集成目标并运行 RED：本地 INS/云台桩无效，远端连续有效仍应进入 FUSED。
- [x] 在观测入口增加远端分支，状态映射与一致诊断快照；保持轮速输出及原本地路径。
- [x] 增加非法组合编译检查及重复、远端失效、本地失效、四轮失效恢复断言。
- [x] `make -B -C tests/chassis_velocity_observer test`，全部旧/新目标通过。

## Task 3：构建、两板与遥测

Files: `Makefile`, `CMakeLists.txt`, `config/99_build/atype-options.txt`, `config/00_flash/board_flash_config.h`, `scripts/build-board.ps1`, `tests/board_flash/`, `application/runtime/vofa_telemetry.c`, `tests/vofa_telemetry/`。

- [x] 先补板1/2新开关和遥测bit16/17/18的失败测试。
- [x] 增加显式开关、数值编译选项、板选择器要求；保留schema/列语义。
- [x] 运行板选择器与遥测主机测试，构建新两板与兼容配置。

## Task 4：文档、回归和审查

Files: `docs/kalman-chassis-test-and-acceptance.md`, `docs/chassis-velocity-fusion.md`, `docs/vofa-telemetry.md`, `config/README.md`, `README.md`，必要的专用验证记录。

- [x] 将远端融合列为已实现待实测；补参数位置、估计时间限制、FUSED判读、故障与验收流程，保留纯采集回退命令。
- [x] 运行相关主机测试、架构检查与 Make/CMake 固件构建，保存实际结果，不将软件结果写成实车通过。
- [x] 独立代码审查并处理问题；核对 diff 仅任务改动，创建本地提交并报告。
