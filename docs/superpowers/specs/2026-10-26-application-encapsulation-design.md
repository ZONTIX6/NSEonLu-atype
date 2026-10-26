# APP 层目录与接口封装整理

日期：2026-10-26

## 目标与边界

整理截图中 `application/` 根目录散落的板级适配和运行支撑文件，收紧内部实现的可见范围。保留现有控制算法、配置值、初始化次序、任务周期、优先级、栈大小以及单板/云台板/底盘板的条件编译行为。

已有未提交修改位于 `application/chassis/chassis.c`、`modules/algorithm/chassis_spin_config.h`、`docs/structure-index.md` 和 `.Doc/指针查找索引.md`。保留它们，仅暂存本次整理产生的差异。

## 方案比较

1. 只移动文件：目录清爽，但头文件内实现和多余公共接口仍然存在。
2. 按职责归档并收紧接口（推荐）：整理目录，拆分任务声明与实现，隐藏私有句柄和回调；通过现有测试与固件构建验证行为一致。
3. 全面重做 APP 与 modules/BSP 的分层：改动面涉及驱动及控制路径，超过这次整理的必要范围。

## 推荐目录

```text
application/
  board/
    board_devices.[ch]
    board_optional.[ch]
    board_analog_key.[ch]
    board_chassis_force.[ch]
  runtime/
    robot_task.[ch]
    ins_bridge.[ch]
    tuning_policy.[ch]
  chassis/  cmd/  gimbal/  shoot/
  robot.[ch]
  robot_def.h
  application.md
  APP层应用编写指引.md
```

`board/` 负责与本车硬件和标定相关的装配，`runtime/` 负责任务创建、姿态与视觉的组合调用及调参保存策略。应用入口和跨应用协议继续留在根目录，现有两份使用文档补充目录及接口说明。

## 接口与数据流

- `robot.c` 通过 `runtime/robot_task.h` 调用 `OSTaskInit(void)`；任务函数全部移入 `robot_task.c`，任务句柄与回调限定为文件私有。
- 保留供调试器观察的力控/功控诊断变量及其名称。
- `board_devices.h` 仅声明 `BoardDevicesInit(void)`，去掉无人调用的裸 IMU 指针访问接口，使 MPU6500 类型不再通过公共头文件传播。IMU 服务仍通过原采样/加热回调接入。
- `tuning_policy.h` 仅声明 `TuningPolicyInit(void)`，保存条件回调设为私有；通过 Flash 保存流程验证条件，避免测试直接依赖内部函数。
- 板级可选外设保持在 FreeRTOS 默认任务中初始化；姿态桥接继续按原有板角色决定是否发送视觉数据。
- 跨目录调用使用相对于 `application/` 的限定头文件路径，例如 `board/board_devices.h`，明确归属。

## 构建和文档

同步 Make 源文件清单、CMake 后端排除规则、浮点编译选项匹配规则及受影响测试的源文件/头文件路径。更新当前使用文档和索引生成器；历史计划作为历史记录保留。索引文件已有用户改动，更新时单独隔离本次差异。

## 验证标准

1. 整理前后，INS 服务与三种板角色桥接、单双板调参保存、OLED/模拟按键、底盘力控测试通过。
2. 分层检查没有新增违规，索引生成器测试通过。
3. Make 构建覆盖默认速度后端、直接力控后端，以及可选外设/功率采集分支；CMake 覆盖速度与力控后端，确认源文件选择及 `-fno-fast-math` 仍有效。
4. 公共任务头文件可被多个编译单元包含，不定义任务函数和存储对象；任务配置保持原值。
5. 审查最终差异，仅提交本次任务相关修改，报告本地提交编号。硬件时序和实车表现不在主机验证的证明范围内。

## 基线结果

2026-10-26：INS 服务及三种板角色桥接、单双板 Flash 调参保存、OLED SPI/I2C 与模拟按键、底盘力控现有主机测试均已在整理前通过。
