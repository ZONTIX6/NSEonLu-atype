#ifndef CHASSIS_FORCE_CONFIG_H
#define CHASSIS_FORCE_CONFIG_H
#include "chassis_control_config.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float x_m, /* 轮心相对实测质心的前向坐标，单位 m；车体 x 向前。 */
          y_m, /* 轮心相对实测质心的左向坐标，单位 m；车体 y 向左。 */
          dx, /* 轮缘力单位方向向量的 x 分量，无单位，包含最终 CAN 正负方向。 */
          dy; /* 轮缘力单位方向向量的 y 分量，无单位；dx^2+dy^2 应约为 1。 */
    float radius_m, /* 受载有效轮半径，单位 m；须大于 0。 */
          reduction, /* 转子转速/输出轴转速的减速比，无单位；须大于 0。 */
          k_out_nm_a; /* 减速器输出轴转矩常数，单位 N*m/A；须实测且大于 0，不再乘减速比。 */
    float current_limit, /* 逐轮电流 CAN 命令幅值上限，单位命令计数；合法值 >0 且 <=16384。 */
          force_limit_n, /* 逐轮可传递的轮缘力幅值上限，单位 N；须经受载测量且大于 0。 */
          current_slew; /* 逐轮电流命令变化率上限，单位 CAN 命令计数/s；须大于 0。 */
} ChassisForceWheel_s;

typedef struct {
    uint32_t backend, /* 后端协议标识，无单位；当前力控校验只接受 1。 */
             schema_version, /* 参数结构版本，无单位；当前校验只接受 1。 */
             checksum; /* 全部配置字段的校验摘要；参数填写完成后调用 ChassisForceConfigChecksum 计算。 */
    bool calibration_valid; /* 标定验收标志：false 未完成，true 已验收；仍须通过每项参数校验。 */
    ChassisForceWheel_s wheel[4]; /* 四轮逐轮参数，顺序 LF 左前、RF 右前、LB 左后、RB 右后。 */
    float force_max_n, /* 满幅平移输入对应的请求合力，单位 N；须大于 0。 */
          spin_torque_nm, /* 固定自旋请求转矩，单位 N*m；须大于 0 且不超过自旋上限。 */
          spin_start_torque_nm, /* 功率自适应的起步请求转矩，单位 N*m；须大于 0 且不超过自旋上限。 */
          spin_torque_max_nm; /* 自适应自旋请求转矩上限，单位 N*m；须大于 0。 */
    float force_rise_n_s, /* 平移请求力的上升速率，单位 N/s；须大于 0。 */
          force_fall_n_s, /* 平移请求力的下降速率，单位 N/s；须大于 0。 */
          torque_rise_nm_s, /* 请求转矩的上升速率，单位 N*m/s；须大于 0。 */
          torque_fall_nm_s; /* 请求转矩的下降速率，单位 N*m/s；须大于 0。 */
    float velocity_limit_mps, /* 车体平移速度保护上限，单位 m/s；须大于 0，不是正常目标速度。 */
          omega_limit_rad_s, /* 车体旋转角速度保护上限，单位 rad/s；须大于 0。 */
          wheel_limit_rpm, /* 电机转子转速保护上限，单位 rpm；须大于 0。 */
          resume_ratio; /* 速度保护解除时的回滞比例，无单位，须在 (0,1) 内。 */
    float input_deadzone, /* 归一化摇杆径向死区，无单位，须在 [0,1) 内。 */
          input_gamma; /* 归一化输入力曲线的幂指数，无单位；须大于 0。 */
    float brake_torque_nm, /* 退出制动请求转矩幅值，单位 N*m；启用制动时须为正且不超过自旋上限。 */
          brake_stop_rad_s; /* 退出制动的停转死区，单位 rad/s；启用时须为正且低于旋转保护上限。 */
    uint32_t brake_timeout_ms; /* 退出制动最多持续时间，单位 ms；启用时合法范围为 1～5000。 */
    float power_gain_nm_per_w_s, /* 功率误差到转矩变化率的增益，单位 N*m/(W*s)；须大于 0。 */
          power_deadband_w, /* 功率跟踪死区，单位 W；须大于 0。 */
          power_lpf_tau_s; /* 负载功率低通时间常数，单位 s；须大于 0。 */
    uint32_t power_recovery_samples; /* 恢复增长前连续新鲜功率样本数，无单位；合法范围 3～20。 */
    float torque_probe_step_nm, /* 申请功率时的候选转矩增量，单位 N*m；须为正且不超过自旋上限。 */
          power_request_step_w; /* 单次申请功率步长，单位 W；合法值 >0 且 <=20。 */
    uint32_t grant_timeout_ms; /* 等待能源批准的超时时间，单位 ms；合法范围 20～1000。 */
    float stall_current_command, /* 堵转判定的电流 CAN 命令门槛，单位命令计数；须 >0 且 <=16384。 */
          stall_rpm; /* 堵转判定的低转速门槛，单位转子 rpm；须大于 0。 */
    uint32_t stall_timeout_ms; /* 大电流且低转速连续成立的确认时间，单位 ms；合法范围 1～5000。 */
    float temp_derate_c, /* 开始温度降额的阈值，单位摄氏度；须为正且低于停止阈值。 */
          temp_stop_c; /* 温度停止阈值，单位摄氏度；须高于降额阈值且 <=100，实际设置仍需温升验证。 */
    float speed_prediction_rpm, /* 功率预测窗口内转子转速增长裕量，单位 rpm；须实测且大于 0。 */
          startup_current_limit; /* 起步时电流 CAN 命令幅值上限，单位命令计数；须 >0 且 <=16384。 */
} ChassisForceConfig_s;

/* 结构字段说明与软件校验范围见上方；范围只表示程序能接受，不能替代硬件标定。
 * 参数值在 chassis_force_vehicle_config.c 填写；通用默认函数刻意不提供可解除锁定的
 * 物理参数，未标定时保持零值与未验收状态。 */
/* 生成通用软件初值；轮组物理参数仍需在车辆配置中填写。 */
ChassisForceConfig_s ChassisForceDefaultConfig(void);
/* 计算配置摘要；填写完所有字段后调用，不能手填固定摘要。 */
uint32_t ChassisForceConfigChecksum(const ChassisForceConfig_s *config);
/* 检查标定标志、摘要和数值约束；返回 true 才允许使用此配置。 */
bool ChassisForceConfigValid(const ChassisForceConfig_s *config);
#endif
