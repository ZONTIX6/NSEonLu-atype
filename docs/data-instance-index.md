# 常用数据指针速查

用 **Ctrl+F** 搜数据用途、指针名或成员名。下表只列日常调试常用的数据入口。

| 数据 | 常用指针 | 常看成员 / 访问示例 | 定义位置 |
| --- | --- | --- | --- |
| 遥控器 | `rc_data` | 左摇杆横向：`rc_data->rc.rocker_l_`<br>左拨杆：`rc_data->rc.switch_left`<br>键盘：`rc_data->key[0].keys` | [application/cmd/robot_cmd.c:42](../application/cmd/robot_cmd.c#L42) |
| 图传键鼠 | `vt02_data` | 鼠标横向：`vt02_data->mouse.x`<br>键盘：`vt02_data->key[0].keys`<br>在线：`vt02_data->online` | [application/cmd/robot_cmd.c:44](../application/cmd/robot_cmd.c#L44) |
| 底盘电机 | `motor_lf`、`motor_rf`、`motor_lb`、`motor_rb` | 角速度：`motor_lf->measure.speed_aps`<br>电流反馈：`motor_lf->measure.real_current`<br>累计角度：`motor_lf->measure.total_angle` | [application/chassis/chassis.c:70](../application/chassis/chassis.c#L70) |
| 云台电机 | `yaw_motor`、`pitch_motor` | 角速度：`yaw_motor->measure.speed_aps`<br>电流反馈：`yaw_motor->measure.real_current`<br>累计角度：`yaw_motor->measure.total_angle` | [application/gimbal/gimbal.c:13](../application/gimbal/gimbal.c#L13) |
| 发射电机 | `friction_l`、`friction_r`、`loader` | 角速度：`friction_l->measure.speed_aps`<br>电流反馈：`friction_l->measure.real_current`<br>累计角度：`friction_l->measure.total_angle` | [application/shoot/shoot.c:14](../application/shoot/shoot.c#L14) |
| 功率计 | `power_meter` | 功率：`power_meter->status.power_w`<br>电流：`power_meter->status.current_a`<br>电压：`power_meter->status.bus_voltage_v`<br>在线：`power_meter->status.online` | [modules/power_meter/power_meter.c:12](../modules/power_meter/power_meter.c#L12) |
| 超级电容 | `cap` | 剩余能量比例：`cap->snapshot.status.energy_ratio`<br>输出使能：`cap->snapshot.status.output_enabled`<br>故障码：`cap->snapshot.status.error_code`<br>输入功率：`cap->snapshot.input_power.input_power_w` | [modules/super_cap/super_cap.c:19](../modules/super_cap/super_cap.c#L19) |
| 裁判系统 | `referee_recv_info` | 机器人 ID：`referee_recv_info->GameRobotState.robot_id`<br>血量：`referee_recv_info->GameRobotState.current_HP`<br>底盘功率上限：`referee_recv_info->GameRobotState.chassis_power_limit`<br>枪管热量：`referee_recv_info->PowerHeatData.shooter_17mm_barrel_heat` | [modules/referee/referee_task.c:19](../modules/referee/referee_task.c#L19) |
| IMU 姿态 | `gimbal_IMU_data` | 偏航：`gimbal_IMU_data->Yaw`<br>俯仰：`gimbal_IMU_data->Pitch`<br>横滚：`gimbal_IMU_data->Roll` | [application/gimbal/gimbal.c:12](../application/gimbal/gimbal.c#L12) |
| 底盘速度 | `chassis_velocity` | 前后速度：`chassis_velocity->actual.vx_mps`<br>左右速度：`chassis_velocity->actual.vy_mps`<br>旋转速度：`chassis_velocity->actual.wz_rpm` | [application/chassis/chassis.c:80](../application/chassis/chassis.c#L80) |

电机同组共用 `measure` 成员，示例中的指针可替换成同一行的其他电机指针。`lf/rf/lb/rb` 分别为左前/右前/左后/右后；`yaw/pitch` 为偏航/俯仰；`friction_l/friction_r/loader` 为左摩擦轮/右摩擦轮/拨弹电机。

`rc_data` 指向控制任务中的遥控器快照，示例查看当前数据。`static` 指针在对应源码作用域查看，并需完成相应初始化。

更新：双击 `scripts/update-structure-index.cmd`。常用名单和示例在生成脚本的 `COMMON_DATA` 中维护；更新时会自动刷新源码位置并核对成员路径。
