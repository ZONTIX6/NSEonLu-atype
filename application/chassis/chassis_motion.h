#ifndef CHASSIS_MOTION_H
#define CHASSIS_MOTION_H

#include "robot_def.h"
#include "general_def.h"
#include "user_lib.h"
#include <math.h>

/* Resolve command motion only; power limits and motor output permission stay downstream. */
static inline void ChassisResolveMotion(Chassis_Ctrl_Cmd_s *cmd, float spin_wz,
                                       float *vx, float *vy)
{
    switch (cmd->chassis_mode)
    {
    case CHASSIS_NO_FOLLOW:
        cmd->wz = 0;
        break;
    case CHASSIS_FOLLOW_GIMBAL_YAW:
#if CHASSIS_FOLLOW_MODE == 0
        // 原跟随策略：角度误差比例控制，沿用原符号与限幅。
        cmd->wz = fmaxf(-4500.0f, fminf(4500.0f, -50.0f * theta_format(cmd->offset_angle)));
#else
        // 云台朝向为前方；每周期清零，避免切出小陀螺后残留旋转目标。
        cmd->wz = 0;
#endif
        break;
    case CHASSIS_ROTATE:
        cmd->wz = spin_wz;
        break;
    default:
        break;
    }

    // 实测云台相对角：将云台坐标平移命令只转换一次到底盘坐标。
    const float cos_theta = arm_cos_f32(cmd->offset_angle * DEGREE_2_RAD);
    const float sin_theta = arm_sin_f32(cmd->offset_angle * DEGREE_2_RAD);
    *vx = cmd->vx * cos_theta - cmd->vy * sin_theta;
    *vy = cmd->vx * sin_theta + cmd->vy * cos_theta;
}

#endif
