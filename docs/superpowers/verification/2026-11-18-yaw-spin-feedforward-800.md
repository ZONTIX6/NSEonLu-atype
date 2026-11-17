# 小陀螺 yaw 前馈系数下调至 800

## 依据与改动

`D:/Downloads/小陀螺启动4.csv`（SHA256 `94a9d8dafdbb1167868a0d330bf779438d0db8d74515fa33b33c57a6a9f81a77`）7 次自旋表明 1080 前馈已参与输出，但启动绝对峰值仍为 4.63～7.94°、退出绝对峰值为 5.23～6.79°，方向均与关闭前馈的上一轮相反。稳态积分约 −1174，正在抵消过量前馈；切入前静止保持积分约 +766～+1205。按每次自旋与静止积分之差估算，下一轮比例候选约 732～795。

本轮仅修改实际控制参数 `config/03_gimbal/gimbal_config.h`：

- `YAW_SPIN_FEEDFORWARD_GAIN`：**1080 → 800 命令刻度/(rad/s)**。
- 实际底盘转速 6.77 rad/s 时，前馈从 7311.6 降至 **5416**，减少约 25.9%。
- 前馈限幅 8000、yaw 总输出限幅 ±20000、速度环积分限幅 9000、两环 PID 参数、启动/退出门控、有效期与正转适用范围沿用原值。
- VOFA 仍为默认卡尔曼档、schema 7、108 通道，I106 bit7 仍表示非零前馈参与，bit8 仍表示叠加输出到限。

同步更新现有前馈及车辆集成测试中的输出期望，不新增只检查常量的测试。历史 1080 版本验证记录保留。

## 验证

- `make -C tests/gimbal_yaw_feedforward -j4 test` 通过：正常、禁用、非法系数版本；覆盖退出保持/释放、无效数值、20 ms 有效期与时间回绕。
- `make -C tests/vehicle_motion test` 通过：实际 CMD→GimbalTask 前馈接线、人工 yaw、轮速过期、急停及全部现有场景。
- 完整 ARM 整车构建通过，无 warning/error；日志中的两个 `#pragma message` 为原工程提示。RAM 164144 B / 192 KB（83.49%），BIN 182384 B。
- 实际 ELF 反汇编中前馈增益常量为 `0x44480000`，即浮点 **800**。
- 从新旧 ELF 符号分别定位两环 PID 常量并读取 BIN，确认两组参数逐字节相同：yaw 速度 Kp=50、Ki=200、Kd=0、MaxOut=20000、IntegralLimit=9000；角度 Kp=8、MaxOut=500。
- 新旧 BIN 大小相同。系数变化使编译器将 `8000/800=10` 改用浮点立即数，前馈函数常量池缩短，后续链接地址有变化；不能用“仅修改几个 BIN 字节”描述本轮产物。

构建使用上一版整车命令，仅输出目录改为 `build/yaw-spin-feedforward-800`：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/yaw-spin-feedforward-800 `
 GCC_PATH=C:/Users/xuzih/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin `
 ATYPE_BENCH_MODE=0 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 `
 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 `
 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 CHASSIS_VELOCITY_FUSION_ENABLE=1 `
 IMU_CAPTURE_CAN_TX_ENABLE=0 IMU_CAPTURE_ONLY=0 ONE_BOARD=1 USB_TELEMETRY_ENABLE=1 `
 VOFA_OBSERVATION_PROFILE=2 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2
```

## 产物与复测

固件：`build/yaw-spin-feedforward-800/basic_framework.bin`，SHA256 `904823bc90a4afb763e4bb8c7896aa4bd997d9cddd792dbcb03a63e0c225fe89`。已生成，未烧录；软件验证不代表实车偏差已消除。

沿用 108 路 VOFA，yaw 摇杆/鼠标不动，复测 6～8 次。每次自旋约 5 秒，退出后等待至少 3 秒且航向稳定再重新进入。比较启动/退出相对 I102 目标的峰值、稳态 MAE 与 I104 积分，继续保留捕获缺号和溢出限制。本轮不预判具体峰值。
