# 双 A 板 IMU 与 +Y 116 mm 补偿实施计划

> **For agentic workers:** 按下列独立文件范围并行实施，完成后由主代理集成验证和复核。

**Goal:** 恢复下方 A 板专用 IMU 采集，通过 CAN1 送往整车主板；用户已确认图示为前/右坐标，图示 +Y 116 mm 对应内部前/左/上坐标 `(0, -0.116, 0) m`；移除 DM 模块适配路径。

**Architecture:** 恢复 `39f8380^` 的九帧 CAN 协议、采集任务与第二板预设，保留后续 VOFA 档位改进。去重力由采集板 INS 完成；主板仅在融合前用现有三点中心补偿扣除安装偏移引起的加速度，避免重复补偿。官方A板U3位号图、MPU6500图14和照片共同推导出芯片后/右/上到内部前/左/上的旋转 `[0,0,0,1]`；仍保留未实车验证标记。

**Tech Stack:** STM32F427、C11、FreeRTOS、Make/CMake、PowerShell、主机 GCC/Python 测试。

## Global Constraints

- 只读速度观测继续不接管控制；保留 `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED=0`。
- 主板继续为 `ONE_BOARD` 整车角色；第二板是 `IMU_CAPTURE_ONLY=1` 采集角色。
- 只创建本地提交，不推送、不烧录硬件。

## Task 1: 双板传输

Files: `modules/imu/chassis_imu_can_protocol.*`、`chassis_imu_sample.h`、`application/runtime/chassis_imu_can.*`、`robot_task.c`、`robot_telemetry.c`、`application/robot.c`、`config/32_capture/imu_capture_config.h`、对应协议/运行时/采集测试；删除 DM 协议和测试。

- [x] 恢复历史协议及测试；恢复前已确认当前缺失发送端。
- [x] 恢复 CAN1 `0x520` 发送/接收、CRC、源时钟及失效处理；沿用公共 sample 头，移除 DM 重力换算与接收路径。
- [x] 运行 `make -B -C tests/chassis_imu_can_protocol test`、运行时和 `tests/imu_capture` 回归。

## Task 2: 构建与板卡入口

Files: `Makefile`、`CMakeLists.txt`、`config/00_flash/board_flash_config.h`、`config/99_build/atype-options.txt`、`scripts/build-board.ps1`、`tests/board_flash/test_board_flash.py`。

- [x] 恢复第二板独立目录与发送开关，保留主板当前 VOFA/USB 配置和统一档位入口。
- [x] 验证 Show 1/2、非法组合与构建失败不下载；构建两个固件。

## Task 3: 安装补偿与来源

Files: `config/30_velocity_observation/chassis_remote_imu_config.h`、`tests/chassis_remote_fusion/*`、VOFA 来源代码/测试与当前用户说明。

- [x] 使用生产配置构造固定中心、正反匀速旋转与角加速数据：内部 `r=(0,-.116,0)`，纯 yaw 时传感器附加加速度 `(+.116*alpha_z,+.116*omega_z^2,0)`；期望中心加速度约零，同时保留真实平移加速度。
- [x] 先运行回归记录零偏移配置的失败，再设 `CHASSIS_REMOTE_IMU_OFFSET_BODY_M {0.0f, -0.116f, 0.0f}` 并验证。
- [x] 用图纸推导的安装旋转生成解析运动夹具，观察单位安装配置失败；设为 `[0,0,0,1]` 后通过，并修正集成测试中的安装姿态。
- [x] 独立复核发现20ms实际间隔限频会丢弃轻微调度抖动的19ms样本；新增161→180ms回归先失败后通过，将限频下限改为15ms，任务名义周期仍20ms。
- [x] 遥测来源改为 A 板，去除 DM 专用来源标记；来源回归先失败后通过。
- [x] 更新双板接线、烧录目标与时间语义说明。
- [x] 运行受影响测试与固件构建，独立复核，`git diff --check` 后仅提交本任务文件。检查结果与未实测项目见 [验证记录](../verification/2026-11-14-dual-a-imu-offset.md)。
