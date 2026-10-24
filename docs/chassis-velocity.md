# 底盘目标与实际速度

统一指针在 `application/chassis/chassis.h` 声明，在 `chassis.c` 定义：

```c
extern const ChassisVelocityFeedback_s *const volatile chassis_velocity;
```

它始终指向静态的 `chassis_velocity_data`，包含 `target` 和 `actual`。`volatile` 修饰指针本身，使该调试入口在链接裁剪后仍保留；数据为只读。调试器 Watch 可展开 `*chassis_velocity`，或观察以下六项：

| 目标 | 实际 | 单位 |
| --- | --- | --- |
| `chassis_velocity->target.vx_mps` | `chassis_velocity->actual.vx_mps` | m/s，向前为正 |
| `chassis_velocity->target.vy_mps` | `chassis_velocity->actual.vy_mps` | m/s，向右为正 |
| `chassis_velocity->target.wz_rps` | `chassis_velocity->actual.wz_rps` | 圈/s，带旋转方向符号 |

两组数据各有 `valid` 和 `tick_ms`。实际时间戳取最老一轮 CAN 反馈，目标时间戳取本次底盘计算时刻。`EstimateSpeed()` 每次 `ChassisTask()` 更新；若底盘任务停止，指针数据也会停止更新，因此消费者必须检查时间。跨任务读取应调用快照接口，它在临界区复制整个目标/实际对，并复查两者的 20 ms 新鲜度：

```c
#include "chassis.h"

ChassisVelocityFeedback_s speed;
(void)ChassisGetVelocitySnapshot(&speed); // 返回 actual.valid
if (speed.actual.valid) {
    float forward_mps = speed.actual.vx_mps;
    float sideways_mps = speed.actual.vy_mps;
    float spin_rps = speed.actual.wz_rps;
    // 使用本地快照，避免多字段读取跨越两个更新周期。
}
if (speed.target.valid) {
    // speed.target 与 actual 使用相同单位，允许直接求差。
}
```

## 数据来源和单位

- 实际值：临界区复制 `motor_lf/rf/lb/rb->measure.speed_aps`、接收时间和在线状态，除以 6 还原转子 rpm。保留 DJI 解码器已有的速度滤波，不额外滤波。停机/零力时仍读取反馈，因此滑行和手推可以显示非零速度。四轮中任意一个无首帧、离线、超过 20 ms 或含非有限值，整车实际速度置无效。
- 目标值：由 `pending_power_control.target_rpm[]` 换算，表示底盘下发轮速目标对应的整体运动需求；控制不允许、零力或整车安全门禁禁止驱动时为零。功率限流作用于电流，不改写目标。单轮自动整定的驱动层覆盖值不属于此底盘目标。
- 两组速度均在底盘车身坐标系，取几何中心；不随云台转动变成云台坐标。旋转正号与既有底盘运动学保持一致，实车须核对方向。不是直接把 `vx_cmd/vy_cmd/wz_cmd` 当 SI 单位。
- 当前 `robot_def.h` 配置：轮半径 60 mm、减速比 19、轴距 350 mm、轮距 300 mm。电机对象顺序为 LF/RF/LB/RB，当前 CAN ID 分别是 1/4/2/3。全向轮方向校正沿用 `OMNI_X_*_DIRECTION`。

轮周速度：`u = rotor_rpm * 2π * R / (60 * reduction)`，单位 m/s。全向轮令 `p=1/√2`，`L=(wheel_base+track_width)/2`，校正方向后的四轮速度按 LF/RF/LB/RB 排列：

```text
vx = (-uLF + uRF - uLB + uRB) / (4p)
vy = ( uLF + uRF - uLB - uRB) / (4p)
spin_rps = (uLF + uRF + uLB + uRB) / (4pL · 2π)
```

全向轮物理旋转有效力臂为 `pL≈0.22981 m`。当前正解算的 Wz 控制标度并非物理角速度，因此目标也必须从轮速目标换算。麦轮模式采用现有麦轮符号矩阵逆解，旋转力臂为 `L`。

这是轮速里程计估算，不包含打滑修正或 IMU 融合。轮径、减速比、轮序、安装角和方向必须与实车一致；几何参数失准或打滑会直接造成估算偏差。

## 遥测

`ChassisTelemetrySnapshot_s.velocity` 携带同一份目标/实际数据。CSV v4 为 111 列，前 101 列沿用 v3，后面依次追加：

```csv
velocity_actual_valid,velocity_actual_tick_ms,velocity_actual_vx_mmps,velocity_actual_vy_mmps,velocity_actual_wz_mrps,velocity_target_valid,velocity_target_tick_ms,velocity_target_vx_mmps,velocity_target_vy_mmps,velocity_target_wz_mrps
```

`*_mmps` 为整数 mm/s，`*_mrps` 为整数毫圈/s；网页都除以 1000。两组有效性互相独立，也独立于功控模型有效性；发送前再次检查年龄。网页“底盘整体速度”显示六个数值，“底盘速度对比”提供六条曲线，实线实际、虚线目标，左轴 m/s、右轴圈/s，支持选择曲线、放大、暂停、历史查看和 CSV 导出。旧 v1–v3 记录正常显示原有数据，新增速度显示“未提供”而不是零。当前固件输出 v5，保留 v4 前 111 列并追加自转上限诊断；导出保留各记录的原始版本和缺失字段，混合版本时切换表头。

不更改 `Chassis_Upload_Data_s` 的双板 CAN 报文布局；本功能的数据入口为上述统一指针/快照和 USB 遥测。

## 验证

主机测试覆盖物理单位、纯运动和组合运动、方向反转、麦轮/全向轮、过期边界、时钟回绕和非法输入；协议测试覆盖固件字节流、带符号速度、独立有效性、旧版读取和导出。浏览器检查覆盖目标/实际、六条曲线、暂停、过期、旧固件、放大和 320–1440 px 布局。固件完成 vehicle 全向轮与麦轮构建，未烧录或验证实车地面速度精度。
