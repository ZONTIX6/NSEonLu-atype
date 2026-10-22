# 独立四轮功率模块

功控内部持有单例，拥有预算、缓冲能量观测、慢外环、瞬态、模型及遥测状态。公共头只依赖纯数据和算法头，可脱离 DJI/HAL 运行主机测试。应用负责裁判/台架/零力等输入、运动学与设备组合；`dji_power_adapter` 负责四轮绑定和单位边界；电机驱动负责 PID、安全门控、量化与唯一 CAN 发送路径。

## 初始化与所有权

先以 `ChassisPower_Init_Config_s` 配置四个模型、算法、预算、外环、瞬态、阶段及功率源开关，调用 `ChassisPowerInit`。配置复制到模块，初始化失败不发布实例，成功后拒绝重复初始化。随后调用 `DJIPowerAdapterInit`，显式给出 LF/RF/LB/RB 四个已注册 M3508 和 `count=4`。空对象、重复成员、非 M3508、未注册成员或第五成员均拒绝。

配置参数保持 `application/chassis/chassis.c` 的现役值：安全系数 0.95、最大原始电流命令 15000、模型电流换算 1000；模型 K0..K5 为 `1.3123816, -0.25836727, 0.0007424524, 0.0024271158, 0.081198552, 3.890596e-08`。速度 PID Kp 保持 4.5（误差为 °/s），封装不重新调参。

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
control.version = ++version;
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

## 调参和实时性

`ChassisPowerRequestConfig` 验证并复制完整配置及版本，慢环边界消费；控制快照携带配置版本，快环按整个版本切换。非法配置不替换参数或清空积分。慢环与快环分别拥有预算/外环和瞬态可变状态，通过短临界区复制快照，不在锁内运行算法，周期内不分配内存。临时快环缓冲属于内部实例或适配器，避免压满电机任务的 2 KB 栈。

RTT 字段、40 ms 输出节拍、USB 字段及 Flash schema 保持兼容。捕获发送由原低优先级任务调用 `ChassisPowerCaptureDrain`，不新增功控任务。整定 PID 仍由电机和整定模块管理。

验证入口：`tests/chassis_power_module`（32 个源/功率计/缓冲/阶段组合）、`tests/dji_chassis_power_rtt`、纯算法与捕获测试，以及 `tests/architecture/test_layers.py`。硬件时序与实车响应须按集成计划单独验收。
