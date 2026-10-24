# 独立四轮功率模块

功控内部持有单例，拥有预算、缓冲能量观测、慢外环、瞬态、模型及遥测状态。公共头只依赖纯数据和算法头，可脱离 DJI/HAL 运行主机测试。应用负责裁判/台架/零力等输入、运动学与设备组合；`dji_power_adapter` 负责四轮绑定和单位边界；电机驱动负责 PID、安全门控、量化与唯一 CAN 发送路径。

## 初始化与所有权

先以 `ChassisPower_Init_Config_s` 配置四个模型、算法、预算、外环、瞬态、阶段及功率源开关，调用 `ChassisPowerInit`。配置复制到模块，初始化失败不发布实例，成功后拒绝重复初始化。随后调用 `DJIPowerAdapterInit`，显式给出 LF/RF/LB/RB 四个已注册 M3508 和 `count=4`。空对象、重复成员、非 M3508、未注册成员或第五成员均拒绝。

配置参数保持 `application/chassis/chassis.c` 的现役值：安全系数 0.95、最大原始电流命令 15000、模型电流换算 1000；模型 K0..K5 为 `1.3123816, -0.25836727, 0.0007424524, 0.0024271158, 0.081198552, 3.890596e-08`。速度 PID Kp 保持 4.5（误差为 °/s），封装不重新调参。

给定完整 `power_config` 和已注册电机，应用启动时组合如下；超电先按 [设备说明](../super_cap/super_cap.md) 独立初始化，功控初始化不访问设备：

```c
if (!ChassisPowerInit(&power_config)) return false;
const DJIPowerAdapterConfig_s adapter = {
    .motors = {motor_lf, motor_rf, motor_lb, motor_rb}, .count = 4,
    .max_current_command = power_config.algorithm.max_current_command,
};
if (!DJIPowerAdapterInit(&adapter)) return false;
```

`power_config` 必须包含四个有效模型和完整预算/外环配置；瞬态可由 `ChassisPowerTransientDefaultConfig()` 起步。配置的全部字段见 `chassis_power_types.h`，A 板具体预算和外环常量见应用初始化与配置索引。最大电流须为正且不超过 32767，阶段为 0..3；不要以全零配置代替车辆参数。初始化与适配绑定在任务启动前完成，无销毁/重新注册接口。

## 5 ms 应用周期

```c
ChassisPowerPolicyInput_s input = {
    .now_ms = now_ms, .dt_s = dt_s,
    .referee = referee, .cap = capacitor, .meter = meter,
    .zero_force = zero_force, .bench_enabled = bench_enabled,
    .bench_limit_w = bench_limit_w,
};
input.previous_valid = ChassisPowerGetSnapshot(&input.previous);
ChassisPowerPolicyOutput_s policy;
bool valid = ChassisPowerUpdatePolicy(&input, &policy);
ChassisPowerControlSnapshot_s control = policy.control;
/* 应用补入同一版本的四轮目标、时间与整定许可。 */
/* 保留策略生成的 version 和 config_version。 */
control.tick_ms = now_ms;
memcpy(control.target_rpm, wheel_rpm, sizeof control.target_rpm);
control.control_enabled &= valid;
DJIPowerAdapterPublish(&control);
if (policy.cap_command_valid) SuperCapSendCommand(&policy.cap_command);
```

模块生成命令，不自行发送超电 CAN。超电缓冲策略保留 ≤20 ms 状态新鲜度要求；设备 Daemon 的 200 ms 语义独立。输入反馈确认目标来自新鲜 `0x052`。裁判明确关闭底盘输出时不允许台架回退绕过关闭。

## 2 ms 快环与最终提交

适配器在每个电机周期调用一次 `ChassisPowerLimit`，提供锁定的完整慢环快照、四轮原始电流、实际/目标 rpm、反馈时间与序号、在线/使能位图和当周期读取的功率计。应用与驱动速度环使用 °/s；功控使用 rpm，仅适配边界除以 6。电流为电调原始命令值，功率单位 W，能量单位 J，时间为毫秒/秒。

模型为 `K0 + K1*I + K2*n + K3*I*n + K4*I² + K5*n²`（模型内部电流按换算系数转换）。阶段 0 保留旧预算并计算快保护诊断，1 启用快保护，2 启动及预测约束，3 完整慢环学习。源/能量硬上限在各阶段均生效。观察阶段可能另算诊断候选，驱动输出只应用一次四轮处理。

限流之后，驱动继续执行急停、整定独占、显式零力恢复、反馈 20 ms 和启停门控，量化为 int16，统一组帧并提交 CAN。然后以实际最终命令调用 `ChassisPowerRecordFinal`。`final_valid` 表示本周期已记录最终命令；`can_submitted` 表示四轮对应 CAN 组提交成功，不证明电机已执行。提交失败仍记录尝试提交的命令，安全门控置零时诊断也为零。

`ChassisPowerGetSnapshot` 原子复制快环与策略的独立时间/版本和有效标记。`ChassisPowerLimit` 返回 false 时输出清零，调用者不得复用旧候选；空输出失败。非有限输入、错误配置、非法 dt 或过期控制快照按失败处理。反馈不足抑制学习并沿用现有增长兜底，最终逐电机安全门控仍由驱动执行。

脱离 DJI 的主机/其他驱动接入同样遵守以下顺序；`cycle_input` 为本周期完整输入，`actual_commands` 为最终门控和量化后实际放入 CAN 的四轮数组，`submitted` 为对应全部 CAN 组的提交结果：

```c
ChassisPowerCycleOutput_s candidate;
bool calculated = ChassisPowerLimit(&cycle_input, &candidate);
/* 驱动使用 candidate.current；calculated=false 时本周期命令必须为零。
 * 此后执行最终门控、量化、组帧和发送，得到 actual_commands/submitted。 */
ChassisPowerFinalInput_s final = {
    .now_ms = now_ms, .control_version = cycle_input.control.version,
    .enabled_mask = final_enabled_mask, .can_submitted = submitted,
};
memcpy(final.current_command, actual_commands, sizeof final.current_command);
ChassisPowerRecordFinal(&final);
```

每个快周期由同一任务依次调用限流及最终记录。计算前、同周期重复记录或不匹配最近限流的控制版本时拒绝；快照中的策略、最终快环数据可来自各自最近的完整周期，消费者须检查各自时间戳、`cycle_valid/final_valid` 和提交标志，不能仅凭 getter 返回 true 判定健康。

## 调参和实时性

`ChassisPowerRequestConfig` 验证并复制完整配置及版本，慢环边界消费；控制快照携带配置版本，快环按整个版本切换。非法配置不替换参数或清空积分。慢环与快环分别拥有预算/外环和瞬态可变状态，通过短临界区复制快照，不在锁内运行算法，周期内不分配内存。临时快环缓冲属于内部实例或适配器，避免压满电机任务的 2 KB 栈。

RTT 字段、40 ms 输出节拍及 Flash schema 保持兼容。USB CSV v3 在原 v2 末尾追加自转阶段、增长原因、增补额度和参数来源，接收器兼容 v1/v2/v3。`adaptive_spin` 输入仅对直供开启限功率优先策略，详见 [自转功率说明](../../docs/chassis-power-priority.md)。捕获发送由原低优先级任务调用 `ChassisPowerCaptureDrain`，不新增功控任务。整定 PID 仍由电机和整定模块管理。

验证入口：`tests/chassis_power_module`（32 个源/功率计/缓冲/阶段组合）、`tests/dji_chassis_power_rtt`、纯算法与捕获测试，以及 `tests/architecture/test_layers.py`。硬件时序与实车响应须按集成计划单独验收。
