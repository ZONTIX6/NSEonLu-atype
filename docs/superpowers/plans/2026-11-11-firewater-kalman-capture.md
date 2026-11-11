# FireWater 整车卡尔曼采集实施计划

**目标：** 将 FireWater 的功率测量行替换为可导出 CSV 的整车速度融合诊断行，支持先采集原始输入、后用实测标定启用融合。

**设计：** 保留 UART7/115200/8N1 和异步发送，使用 schema 2、固定 64 列定点整数、80 ms 最小间隔。整车 observer 在同一临界区捕获四轮、INS 和云台编码器；遥测只读该快照，并分别报告最新输入、滞后融合估计、控制快照的年龄。USB 和 JustFloat 格式保持兼容。

**约束：** 不改变滤波参数或电机控制；不使用测试标定作为实车标定；不提交已有的 board_config.h 与采集目录改动。融合关闭时明确输出 status=-1；未标定时仍采集原始输入。80 ms 是降采样观测，不能用于逐步重放滤波器或确定高速动态的精确 Q/R。

**用户补充的双板方案：** 第二块 A 板刚性固定在底盘中心附近，使用同一工程的 `IMU_CAPTURE_ONLY=1` 独立采集。该模式只启动 INS 与遥测，保留本板加热，不初始化电机/底盘/云台/遥控/参数写入。两份 CSV 以 `I63` 标明角色（主板 0、独立 IMU 板 1），后者状态 `I6=-2`。目前不新增两板通信或实时融合；两板独立时钟和导航参考系须离线对齐。

## 实施与验证

- [x] 扩展 `application/chassis/chassis_velocity_observer.[ch]` 的只读诊断，包含 raw RPM、输入时间、原生四元数、去重力加速度、角速度、yaw/pitch 编码器。`tests/chassis_velocity_observer` 验证标定及未标定路径、源失效与时间对应关系。
- [x] 修改 `application/runtime/vofa_telemetry.[ch]`，接收 `ChassisFusedVelocitySnapshot_s`，输出 schema、时钟/序号、状态、实际/目标/融合速度、原始输入、最终电流及功率。所有无效数值为 0，未知年龄为 -1；浮点非有限数清有效位；发送忙不覆盖在途缓冲，并用序号显示丢行。
- [x] 在 `application/runtime/robot_telemetry.c` 读取融合快照，保持现有 USB 收集路径。`tests/vofa_telemetry` 验证列顺序/单位、UART cadence、溢出/回绕、未启用/未标定/拒绝/失效状态，以及 JustFloat 兼容。
- [x] 增加仅 IMU 编译选项与启动隔离；`tests/imu_capture` 验证实际应用启动不调用控制器且只创建 INS/遥测任务，`tests/ins_service` 验证该模式无需整车融合也能发布运动快照，`tests/vofa_telemetry/test_imu_capture.c` 验证第二板真实运行入口输出仅 IMU 有效的数据。
- [x] 更新 `docs/vofa-telemetry.md` 的列名、有效位、采集构建命令和采集动作；说明最新输入与融合配对数据不能混用，给出 CSV 导出交付要求。
- [x] 执行主机遥测/observer/fusion 测试，构建 FireWater 未标定采集固件、普通关闭融合固件、使用测试标定的编译验证固件、第二板 Make/CMake 固件。检查栈使用与第二板链接符号；独立审查。验证记录见 `docs/superpowers/verification/2026-11-11-firewater-kalman-capture.md`。

## 64 列布局

`0` schema；`1–2` 板端 ms；`3–4` 发送序号；`5` 有效位与配置（bit13供电、bit14独立IMU）；`6` 融合状态；`7` 底盘模式；`8–9` 在线/使能；`10–12` 观测/估计/轮速年龄；`13–16` 四轮 raw RPM；`17–20` 四轮反馈年龄；`21–23` 轮速反算 vx/vy/wz；`24–26` 目标 vx/vy/wz（平移mm/s，旋转mrev/s）；`27–28` 估计 vx/vy；`29–30` 配对轮速 vx/vy；`31–32` 中心导航加速度 XY；`33` IMU 年龄；`34–35` IMU 序号；`36–39` 原生四元数；`40–42` IMU 导航加速度 XYZ；`43–45` 传感器角速度 XYZ；`46–47` yaw/pitch 编码器；`48–49` yaw/pitch RPM；`50–51` yaw/pitch 年龄；`52–55` 最终四轮电流命令；`56–59` 限制前/后预测功率、有效功率上限、控制周期功率计读数；`60–62` 控制/目标/功率计年龄；`63` 记录板角色。
