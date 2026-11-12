# DM-IMU-L1 接入：软件验证记录

本任务将固定底盘的第二块 A 板替换为 CAN2 上的 DM-IMU-L1，并按用户要求删除旧 CAN 协议、发送路径、第二板构建预设和专用测试。公共样本类型抽出至 `modules/imu/chassis_imu_sample.h`；本地 UART IMU 诊断及主板云台 INS 保留。工作期间另一任务提交了 `4d4024d` 的 JustFloat 遥测支持，本任务基于该提交完成，未回退其改动。

## 主机验证

以下测试均通过：

- `tests/dm_imu_can_protocol`：手工字节夹具、14bit四元数、厂家掩码错误的变异验证、单位转换、水平/倾斜静止去重力、动态旋转、非法长度/顺序/姿态、组装超时、节流、时钟回绕。
- `tests/dm_imu_runtime`：生产CAN回调到快照再到生产融合算法；CAN2/0x11注册、60ms接收年龄边界、读快照不刷新时间、断流轮速回退及恢复。分别编译VOFA模式1/2通过。
- 100Hz接收抖动回归：首片时刻模式 `[0,10,19,31]` 每40ms重复。原20ms硬门槛可复现0次融合更新；改为首片间隔至少18ms后，1秒测试预热后持续FUSED、更新至少40次、重置0次。
- 原有 `chassis_remote_fusion`、`chassis_velocity_observer`、`chassis_velocity_center`、`chassis_gimbal_imu`、`vofa_telemetry`、`ins_service`、`vehicle_motion`、`imu_capture` 回归通过。
- 板配置测试10项，主板构建脚本测试10项通过；包括拒绝Board2、越界目录、非法目标，Show不执行构建、构建失败不烧录。
- 框架分层严格检查：0条边界违规；应用头文件与电机链接边界检查通过。
- 结构索引生成/一致性检查及 `git diff --check` 通过。

协议、runtime和遥测来源标志测试均在实施前观察到预期失败。最终测试日志位于忽略目录 `build/dm-final-*.log`；协议针对厂家错误掩码的临时变异程序已清理。

## 固件与链接

| 入口 | 输出 | Flash | 链接器RAM（含保留堆栈） |
| --- | --- | ---: | ---: |
| `scripts/build-board.ps1 -Action Build -Board 1` | `build/board-1-main` | 184312 B | 152816 B |
| CMake，RX/速度观测/远端融合均为1，VOFA模式2 | `build/cmake-dm-imu` | 184332 B | 152824 B |

两者均成功生成ELF/BIN，`scripts/check-atype-artifacts.py`检查通过。Make与CMake均为DM数值解码保留`-fno-fast-math`。旧协议文件与源列表依赖已移除；原第二板构建不再是可用入口。

最终BIN SHA256：

```text
Make:  2be4d1c231c10cb8762f59a858055f41df6c6e4e77346aeba7e0ec7c504e2b65
CMake: cf804921862bf535b33822a08691cec0f619c4c755b110cd98cd3eb8a3b0ab86
```

## 上板待验证

本次没有烧录或连接传感器验收。需要按[接入说明](../../dm-imu-integration.md)配置模块为CAN 1Mbps、主动100Hz、MST_ID=0x11，确认实物帧序、四元数方向、静止/倾斜去重力、安装旋转与偏移、断流恢复和任务耗时。协议无源时间戳/组序号，接收时间只是估计，组装窗口不证明同步采样。默认安装参数未审核，融合仅用于观测，软件测试不代表实车精度已通过。
