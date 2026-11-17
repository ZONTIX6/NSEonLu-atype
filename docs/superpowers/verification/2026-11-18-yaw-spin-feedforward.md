# 小陀螺 yaw 轮速输出前馈（首轮复测版）

## 实测依据与本轮目标

`D:/Downloads/小陀螺启动3.csv`（SHA256 `ab9cc09f7c74caedfbe3f03ab28d15130cf138565ab992a8ad2b530342443589`）中，积分上限放宽至 9000 后，8 次自旋的后段平均绝对跟踪误差已降到 0.452°，但启动仍有 11.913～12.606° 已记录误差峰，退出反向回摆为 9.024～9.764°。稳态所需命令约 7339.8、积分约 7329.3、轮速逆解角速度约 6.7698 rad/s。

本轮把主要持续抗扰输出交给轮速前馈，减少起停时建立/卸载约 7300 积分的需求。上述比例只提供初值，不能证明全部扰动都是速度的线性函数；实际偏差改善待复测。

## 控制行为

- 配置入口：`config/03_gimbal/gimbal_config.h`。默认 `YAW_SPIN_FEEDFORWARD_ENABLE=1`，系数 **1080 命令刻度/(rad/s)**，前馈最大 **8000**。
- 由 CMD 在安全处理后向本地 gimbal 消息传递自旋请求。仅在 **ONE_BOARD + 速度后端**接入；不依赖融合开关，双板与直接力控后端不接入。
- 速度来自 `ChassisGetVelocitySnapshot().actual`，即四轮反馈逆运动学，沿用现有 0.85 新样本权重的轮速平滑。单位 rpm 乘 `2π/60` 后用于前馈；既不使用底盘目标速度，也不使用远端 IMU 或卡尔曼估计。
- 已实测的正转方向使用 `min(1080 × max(实际角速度, 0), 8000)`；6.77 rad/s 对应约 **7312**。反向未标定，前馈为零。
- 进入自旋后启用；退出模式仍按实际正向余速减小，降到 0.05 rad/s 时释放。释放前的同向旋转继续按该比例补偿。普通跟随/不跟随不能独立启用新的前馈。
- 轮速无效、非有限、最老轮样本超过 20 ms、云台零力或安全门关闭时，立即清除前馈及退出保持状态。反馈恢复后须仍处在自旋档才会重新启用。
- DJI 驱动冻结前馈值与**源轮速时间戳**；每个电机周期再次检查 20 ms 年龄，避免任务停更或反复读取旧样本使其永久有效。该附加输出只用于没有电流 PID 的速度闭环，参与原有唯一 CAN 发送路径和停止门控。
- 速度 PID 输出加前馈后再次限于原 `MaxOut`，yaw 为 **±20000**。`DJIMotorStop()` 同时清除保存的前馈。PID 的 Kp/Ki/Kd、yaw 目标与 **9000** 积分限幅沿用上一版，不用固定补角，也不在模式切换时重置积分。

当前系数只在同向约 6.77 rad/s、yaw 输入保持不动的工况下有依据。人工大幅 yaw 转动、轮胎打滑或其他转速下的准确性需要单独实测；8000 是输出上限，不代表这些工况已经标定。

## VOFA 复测

沿用默认卡尔曼观测档 `VOFA_OBSERVATION_PROFILE=2`、schema 7、108 路、UART7 230400/8N1，帧长度仍为 436 字节。本轮不增加串口负载。

I106 保留 bit0～6，新增：

- bit7（128）：本周期启用输出中已加入非零前馈，发生在总输出限幅前；不代表 CAN 电机已实际执行。
- bit8（256）：PID 与前馈叠加值达到总输出限幅。

正常启用、CAN 邮箱接受且无积分/输出限幅时，原标志 71 加前馈后为 **199**。I104 仍是纯速度环积分；I105 仍是最终 CAN 整数命令，包含前馈。精确的本周期前馈值保存在 `yaw_motor->control_snapshot.output_feedforward`，可通过现有锁定 getter 或调试器读取，未额外占用 VOFA 通道。

下一轮请保持其余控制参数、载荷与地面条件一致，yaw 摇杆/鼠标不动，记录 **6～8 次**：每次进入前静止至少 2 秒，自旋保持约 5 秒，退出后至少等待 3 秒、航向稳定后再开始下一次。保留完整 CSV。

重点比较：I102−I100 的启动峰、退出反向峰、2 秒后稳态 MAE，以及 I104 是否从约 +7300 降到较小的残差积分。以实际目标 I102 为基准，避免上一轮退出尚未结束的偏差污染基准。积分显著变负且启动偏向反转，提示比例偏大；积分持续大幅为正提示补偿不足；仍须结合瞬态波形判断。

旧记录已有捕获缺号和 I5 bit7 FIFO 溢出粘滞位，继续检查并报告；有缺口时峰值只能称已记录极值。预期是同时减小启动与退出偏差，不能在实车复测前宣称偏差已消除。

回退：将 `YAW_SPIN_FEEDFORWARD_ENABLE` 设为 0 后重新构建，或使用上一版 `build/yaw-integral-9000/basic_framework.bin`。

## 软件验证

- `tests/gimbal_yaw_feedforward`：正转启动、减速退出、静止释放、普通模式不误启用、反向零补偿、8000 上限、无效/非有限输入、20 ms 边界、时间回绕、禁用及非法系数版本通过。
- `tests/dji_chassis_power_rtt`：实际驱动的前馈相加、正负总限幅、反馈方向、源时间戳失效、冻结快照、CAN 字节、停止清除、离线、过期电机反馈和安全门归零通过；原功率集成/旁路回归通过。
- `tests/vehicle_motion`：实际 CMD→消息→GimbalTask 集成通过，验证实际速度而非目标速度驱动前馈、源时间戳不刷新、退出减补、普通转向不启用、手动 yaw 引用继续变化、轮速失效及急停撤销。全部原有场景通过。直接力控场景使用 `CHASSIS_POWER_CONTROL_ENABLE=1` 编译运行通过（直接力控的现有构建要求）。
- `tests/chassis_force`、`tests/chassis_velocity_observer`、`tests/vofa_telemetry` 回归通过，新增 I106 标志在 FIFO 中冻结、传输并保留原标志。`tests/vofa_analysis` 使用有 SciPy 的系统 Python 运行，44 项通过；JSON 通道契约生成检查通过。
- Make 与 CMake 对 gimbal 数值文件启用 `-fno-fast-math`，保留失效数值检查。整车 ARM 构建通过，无编译警告/错误；RAM **164144 B / 192 KB（83.49%）**，BIN **182384 B**。编译器报告 `DJIMotorControl` 自身栈帧 280 B、前馈算法自身 8 B，不代表完整运行时栈用量。
- BIN：`build/yaw-spin-feedforward/basic_framework.bin`，SHA256 `37ac869811bbc3385066b884b0cda2f40bef15e42187ac934c7f2e7220ff2874`。只生成固件，未烧录；软件测试不等同于实车效果验证。

构建命令：

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/yaw-spin-feedforward `
 GCC_PATH=C:/Users/xuzih/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin `
 ATYPE_BENCH_MODE=0 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 `
 CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 `
 CHASSIS_VELOCITY_FUSION_CALIBRATED=0 CHASSIS_VELOCITY_FUSION_ENABLE=1 `
 IMU_CAPTURE_CAN_TX_ENABLE=0 IMU_CAPTURE_ONLY=0 ONE_BOARD=1 USB_TELEMETRY_ENABLE=1 `
 VOFA_OBSERVATION_PROFILE=2 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2
```
