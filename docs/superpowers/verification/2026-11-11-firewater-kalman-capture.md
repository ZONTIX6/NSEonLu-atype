# FireWater 双板卡尔曼采集软件验证

日期：2026-11-11。基线提交：`2228c83`。本次不烧录硬件、不修改滤波参数、不接入第二板实时融合。

## 主机测试

以下命令全部通过：

```powershell
make -C tests/vofa_telemetry test
make -C tests/chassis_velocity_observer test
make -C tests/chassis_velocity_fusion test
make -C tests/imu_capture test
make -C tests/ins_service test
```

- 遥测：JustFloat 保持兼容；FireWater 64 列、正负值/比例、32 位时钟回绕、来源年龄、未标定/拒绝/关闭、非有限数、定点饱和、忙时缓冲所有权和失败发送序号；第二板调用真实 `RobotTelemetryTask()`，无需链接底盘/电机运行依赖即可输出来源标记和仅 IMU 有效的数据。
- 观测器：原旋转云台/融合/源失效测试；新增标定和未标定构建的原始四轮/INS/编码器及其时间/序号对应关系、过期诊断保留。
- IMU-only：实际 `RobotInit()` 不调用各电机应用/参数初始化；实际任务装配只创建 INS 与遥测任务；默认任务不初始化可选设备；INS 在 `FUSION_ENABLE=0` 时仍发布带时间戳的原生运动快照。
- 测试先验证旧 FireWater 缺 schema、旧 observer 缺诊断接口、旧启动路径会调用控制应用，再验证新实现通过。纯采集运行入口测试同时检查脱离机器人服务的链接边界。

## 固件构建

以下 Make 构建均成功，使用 ARM GNU 13.3：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/kalman-firewater VOFA_TELEMETRY_MODE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/imu-firewater IMU_CAPTURE_ONLY=1 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=1 USB_TELEMETRY_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=0
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/atype-vehicle VOFA_TELEMETRY_MODE=0
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/velocity-fusion-synthetic CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=1 VOFA_TELEMETRY_MODE=1
```

最后一项只使用既有测试标定头做编译验证，不能用于实车。前两项输出分别供主板和第二板采集。各目录提供 `.elf`、`.hex`、`.bin`。

| 构建 | RAM | Flash |
| --- | ---: | ---: |
| 主板未标定 FireWater | 152040 B | 172152 B |
| 第二板 IMU-only | 126056 B | 73116 B |
| JustFloat / 关闭整车融合 | 150856 B | 168564 B |
| 合成标定 / FireWater | 152328 B | 181424 B |

CMake 路径也通过：使用本机 STM32Cube 附带的 `C:/Users/20802/AppData/Local/stm32cube/bundles/cmake/4.2.3+st.1/bin/cmake.exe`，生成器 `MinGW Makefiles`，目录 `build/cmake-imu-firewater`，选项与第二板 Make 构建一致。RAM 126072 B，Flash 73124 B。

`arm-none-eabi-nm --defined-only build/imu-firewater/basic_framework.elf` 确认保留 `INS_Task`、`StartIMUCapture`、`StartCaptureTelemetry`、`VofaTelemetrySendMeasurement`，未链接 `ChassisInit`、`GimbalInit`、`ShootInit`、`RobotCMDInit`、`DJIMotorControl`、`StartMotorTask`、`StartRobotTask`。

编译器 `.su` 报告：遥测发送函数主板 344 B/第二板 336 B，第二板遥测入口 72 B，未标定观测函数 320 B，INS_Task 152 B。此为单函数静态栈数据，不能替代全调用链/中断及板端高水位测量。UART 最大行长 768 字节在 115200/8N1 下约 66.7 ms，低于 80 ms 最小发送间隔，代码有静态断言。

## 审查与实测边界

独立代码审查覆盖启动隔离、时钟、温控、列/单位/有效位和异步缓冲，未发现重要问题。代码未改变现有底盘滤波参数或正常控制路径。

仍需硬件确认：两板实际启动与姿态收敛、串口完整率、任务时序/栈余量、安装方向及偏移、两板时钟偏移/漂移、坐标对齐。80 ms 降采样适用于首轮静态/低速分析，不能逐步重放高频融合更新或据此宣称精确 Q/R 与真实车速已经验证。
