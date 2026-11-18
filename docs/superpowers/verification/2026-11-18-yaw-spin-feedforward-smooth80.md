# 小陀螺 yaw 前馈 800 / 80 ms 平滑试验

## 依据与改动

`D:/Downloads/小陀螺启动6.csv`（SHA256 `affcaec1389c62e7cc251ff5be5af019982736c765afd09311a8bd4f735f1fe5`）5 次切入前的航向误差均在 0.17° 内，启动/退出已记录绝对峰值均值仍为 3.21° / 3.74°，稳态 MAE 约 0.39°。本次试验保持增益 800，检验前馈变化时序能否改善起停瞬态；80 ms 尚非实车标定值。

- 新增 `YAW_SPIN_FEEDFORWARD_TIME_CONSTANT_MS=80.0f`。
- 在现有“实测轮速 × 800”限幅目标之后，增加按实际任务时间计算的一阶低通：`alpha = dt_ms / (80 + dt_ms)`，`output += alpha * (target - output)`。首次启用输出从零建立；同一毫秒内重复调用不增加滤波时间。
- 正常退出仍按剩余实测转速补偿。转速降至 0.05 rad/s 或以下后释放锁存，目标设为零，前馈尾量继续衰减，低于 1 个整数 CAN 命令刻度时归零。在尾量期间，普通跟随不能重新激活补偿，再次切入自旋则连续接续。
- 云台禁用/急停、轮速无效、非有限值、过期或反转立即清除前馈及滤波状态。任务调用间隔超过 20 ms 也清除旧状态；若仍请求自旋则从零重建。
- 保留源轮速时间戳及驱动端的 20 ms 二次时效检查，平滑尾量也不允许延长过期数据的寿命。
- 增益 800、前馈上限 8000、总命令上限 20000、速度积分限幅 9000 及两环 PID 沿用上一版。
- VOFA 默认卡尔曼档仍为 schema 7 / 108 路，I106 bit7 包含实际非零平滑前馈参与，bit8 表示叠加输出限幅。

时间常数不是固定等待时间。1 ms 任务周期下，阶跃目标约在 80 ms 达到 63%，240 ms 达到 95%；真实前馈还随实测转速变化。

## 验证

- `make -C tests/gimbal_yaw_feedforward -j4 test` 通过正常、禁用、非法增益三个版本。覆盖阶跃建立、稳态增益、正常减速、停转尾量、尾量中普通跟随/重新切入、变任务周期与重复时间戳、极值限幅、急停/无效/反转清状态、20/21 ms 时效边界、任务间断和 uint32 时钟回绕。动态响应采用独立连续时间响应区间验证。
- `make -C tests/vehicle_motion test` 全部 12 个场景通过；其中实际 CMD→GimbalTask 接线验证输出平滑、稳态 5416、人工 yaw、正常退出尾量、轮速过期后清零并平滑重建、急停清滤波状态，以及源时间戳保持。
- `make -C tests/vehicle_motion test_force "CFLAGS=-std=c11 -Wall -Wextra -Werror -O0 -g -DCHASSIS_POWER_CONTROL_ENABLE=1"` 通过。直接力控测试需要显式满足其功率控制前置条件；本轮整车固件仍使用速度后端。
- `make -C tests/dji_chassis_power_rtt -j4 test` 两个现有驱动集成测试通过。
- 完整 ARM 整车编译通过，无 warning/error；两个 `#pragma message` 为既有工程提示。RAM 164152 B / 192 KB（83.49%），相较上一版增加 8 B；BIN 182504 B，相较上一版增加 120 B。
- 从新旧 ELF 的 LOAD 段及符号位置提取比较，确认 `pid_gimbal_yaw_speed_config`、`pid_gimbal_yaw_angle_config` 和 `chassis_spin_config` 逐字节相同；`configuration.txt` 构建选项相同。工作区已有底盘自旋参数改动未被覆盖或提交，它们在上一版 800 固件中已经生效。
- 实际前馈函数反汇编常量池包含 `0x44480000`（800）与 `0x42a00000`（80）。
- `git diff --check` 对本次任务文件通过。

构建命令沿用上一版，仅修改输出目录：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/yaw-spin-feedforward-800-smooth80 `
 GCC_PATH=C:/Users/xuzih/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin `
 ATYPE_BENCH_MODE=0 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 `
 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 `
 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 CHASSIS_VELOCITY_FUSION_ENABLE=1 `
 IMU_CAPTURE_CAN_TX_ENABLE=0 IMU_CAPTURE_ONLY=0 ONE_BOARD=1 USB_TELEMETRY_ENABLE=1 `
 VOFA_OBSERVATION_PROFILE=2 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2
```

## 产物与复测

固件：`build/yaw-spin-feedforward-800-smooth80/basic_framework.bin`。

SHA256：`abd6064947d0ec0f3ce345be78a284ed949be73829db8f40ebb904831d36337c`。

已生成，未烧录。构建日志为 `build/yaw-spin-feedforward-800-smooth80-build.log`；新旧参数比较脚本及结果为固件目录内的 `verify-parameters.py`、`parameter-comparison.json`。

保持 yaw 摇杆/鼠标不动，以原 108 路 VOFA 记录 5～8 次完整起停。每次自旋约 5 秒，退出后等待至少 3 秒且航向稳定再切入。与启动 6 比较起停峰值、反摆、稳态 MAE 和 I104 积分；若起停变差则回退上一版 800 固件。软件响应验证不代表实车偏差已经消除，CSV 的捕获缺号/溢出限制仍须保留。
