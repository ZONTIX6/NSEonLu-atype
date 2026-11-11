# CAN1 IMU 汇总采集软件验证

日期：2026-11-11；实现起点 `110ea6a`。第二块 A 板固定在底盘，CAN1 发送 IMU；主板 UART7 输出 schema 3 单份 CSV。本次未烧录实车，未启用远端 IMU 速度融合或控制。

## 主机验证

以下测试全部通过：

```powershell
make -C tests/chassis_imu_can_protocol test
make -C tests/chassis_imu_can_runtime test
make -C tests/imu_capture test
make -C tests/vofa_telemetry test
make -C tests/chassis_velocity_observer test
```

- 协议：显式小端、54 字节/9 帧、独立已知 CRC fixture，完整组一致性；缺片、混组、乱序、重复、超时、长度、CRC、版本、非有限数/四元数检查、源无效及计时回绕/恢复。只有完整有效协议组更新接收快照；源无效组也可发布以清除测量有效性。
- CAN runtime：CAN1/标准 ID `0x520`、20 ms 周期、发送时源年龄、失败立即停止本组并取消自身待发邮箱、前组残留及未完成取消处理、10 ms 总期限、源年龄/接收年龄分离、接收端只读临界区、无效源与独立时钟。
- 总线等待：模拟邮箱至少等待 0.5 ms 才释放，原 0.3 ms 在完整发送断言失败；改为每片最多等待 1 ms 后通过。仍保留整组 10 ms 上限及取消逻辑。
- 启动：原 UART 纯采集、无 UART 的 CAN 纯采集均只运行 INS/温控与低优先级通信，不初始化电机控制及遥控应用。
- FireWater：schema 3 的 72 列、远端来源/时间/年龄/拒绝计数；断链与源无效时不退回本地云台 IMU；已有 schema 2 和 JustFloat 测试保持通过。旧 writer 在 schema 3 测试中先失败，再验证新实现通过。
- 原整车观测器标定/未标定及旋转云台回归通过。

## 固件构建

两套 Make 构建成功，均生成 `.elf`、`.hex` 和 `.bin`：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/kalman-can1-main CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_VELOCITY_FUSION_ENABLE=1 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=1
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/imu-can1-sender IMU_CAPTURE_ONLY=1 IMU_CAPTURE_CAN_TX_ENABLE=1 VOFA_TELEMETRY_ENABLE=0 USB_TELEMETRY_ENABLE=0 CHASSIS_VELOCITY_FUSION_ENABLE=0
```

同组选项的 CMake / MinGW Makefiles 构建也成功，目录为 `build/cmake-kalman-can1-main` 和 `build/cmake-imu-can1-sender`。使用本机 STM32Cube 附带 CMake 4.2.3；新协议/runtime 在 Make 和 CMake 均保留 `-fno-fast-math`。

| 构建 | RAM | Flash |
| --- | ---: | ---: |
| Make 主板 CAN 接收 | 152352 B | 173344 B |
| Make 第二板 CAN 发送 | 125144 B | 72020 B |
| CMake 主板 CAN 接收 | 152368 B | 173376 B |
| CMake 第二板 CAN 发送 | 125160 B | 72044 B |

发送板 ELF 符号核对：包含 `INS_Task`、`ChassisImuCanSend`、`CANTransmit`；不含 `ChassisInit`、`GimbalInit`、`ShootInit`、`RobotCMDInit`、`DJIMotorControl`、`StartMotorTask`、`StartRobotTask` 和 `VofaTelemetrySendMeasurement`。CAN 发送只用本模块的诊断 ID。

静态单函数栈报告：CAN 发送 208 B，接收解码 88 B，主板 FireWater 376 B。72 列最坏 864 字节，115200/8N1 下约 75 ms，低于 80 ms 最小发送间隔，编译期断言检查。静态数值不能替代板端完整调用链与中断栈高水位测量。

## 审查与硬件边界

独立审查检查了分片、IRQ 临界区、时钟、邮箱取消、控制隔离和 CSV 对应关系。发送等待窗口调整后，无其他重要问题。

仍需实车检查 CAN1 的终端及接线、实际负载与丢 ACK 后恢复、采样/发送时序、UART 完整率和任务栈。两板时钟未同步；I33 是发送时源年龄加首片接收后的本机经过时间，不包含首片排队或传感器内部延迟。单 CSV 不代表同时采样；80 ms CSV 不能逐次重放所有高频融合输入。
