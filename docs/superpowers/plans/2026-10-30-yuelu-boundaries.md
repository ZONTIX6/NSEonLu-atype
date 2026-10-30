# 跃鹿框架边界整理 Implementation Plan

> 按用户已批准的顺序在当前会话执行；代码审查使用 requesting-code-review 的独立只读审查流程。

**Goal:** 恢复清晰的 application → modules → bsp 依赖和职责，同时保持现有行为。

**Architecture:** 应用负责整车采集与装配；模块负责协议、算法和设备；驱动通过注册回调接收调参策略。

**Tech Stack:** C11、STM32F427、FreeRTOS、Make/CMake、GCC 主机测试、Python 架构检查。

## Global Constraints

遵守同日设计文档的全部不变量。任务只移动职责、增加必要接口和接线，不改变控制计算、协议、配置或任务时序。

### 1. 反向依赖

- [ ] 保存基线：`make -B -C tests/dji_chassis_power_rtt test`、USB 与标定回归、默认固件构建。
- [ ] 在 `tests/architecture/test_layers.py` 加入遥测模块反向依赖用例，先运行确认失败，再扩大目录覆盖。新增目录暂时出现的实际违规必须通过迁移消除。
- [ ] 将 `modules/telemetry/usb_telemetry.c` 的整车采集/任务逻辑移到 `application/runtime/robot_telemetry.c`，公共入口为 `void RobotTelemetryInit(void)`、`void RobotTelemetryTask(void)`；流模块接口不变。
- [ ] 协议提供 `void ForceCalibrationProtocolConfigure(const FcLimits *limits)`，应用初始化复制限制，协议 CAP/START 使用该配置。先增加非默认限制和重连测试，再实现。
- [ ] 更新 Make 来源和任务接线；运行架构、USB、标定测试，提交。

### 2. 电机策略

- [ ] 扩展架构检查拒绝 `dji_motor.c` 对调参/整定协议的直接依赖，并确认现有代码失败。
- [ ] 新建 `modules/motor/DJImotor/dji_tuning_adapter.[ch]`；由速控/力控适配器注册通用周期策略回调。保持每个回调处于原逻辑执行位置；最终 CAN 门控仍在驱动。
- [ ] 既有整定回归改用真实策略接线，保留原输出/恢复/隔离断言；独立编译驱动验证无需链接调参协议。
- [ ] 运行 DJI 四阶段、无功率计、力控、标定、自动整定与存储回归；检查 196 周期黄金夹具保持原样，提交。

### 3. 底盘应用与验收

- [ ] 新建 `application/chassis/chassis_internal.h`、`chassis_init.c`、`chassis_tuning.c`，把共享状态限制在应用私有接口。保留公开入口和快照符号。
- [ ] 机械迁移原初始化、配置和调参函数；`chassis.c` 保留控制周期及相关运动/功率编排。Make/CMake 在力控模式排除全部速度底盘实现。
- [ ] 比较迁移前后控制函数、常量与调用顺序；运行底盘、整车输入、消息、调参和配置测试。
- [ ] 完整主机/固件矩阵、额外力控/标定/速度观测构建、CMake、产物检查；更新应用说明、结构索引和验证记录。
- [ ] 独立只读代码审查并处理重要问题，最后创建本地提交并报告编号。
