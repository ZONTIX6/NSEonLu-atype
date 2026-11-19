#ifndef CHASSIS_MOTION_H
#define CHASSIS_MOTION_H

#include "robot_def.h"
#include "general_def.h"
#include "user_lib.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

typedef struct {
    float angle_deg, rate_dps;
    uint32_t tick_ms;
    bool initialized;
} ChassisSpinPhase_s;

/* Differentiate the same relative angle used by the transform: raw gyro/Wz
 * has a different sign and does not include motion of the gimbal itself. */
static inline float ChassisSpinPhaseUpdate(ChassisSpinPhase_s *state,
    const Chassis_Ctrl_Cmd_s *cmd, uint32_t now_ms)
{
    const float prediction_ms = CHASSIS_SPIN_PHASE_PREDICTION_MS;
    const float limit = CHASSIS_SPIN_PHASE_MAX_DEG;
    const float filter_ms = CHASSIS_SPIN_PHASE_FILTER_MS;
    if (!CHASSIS_SPIN_PHASE_COMPENSATION_ENABLE || cmd->chassis_mode != CHASSIS_ROTATE ||
        !isfinite(cmd->offset_angle) || !isfinite(prediction_ms) || prediction_ms < 0 ||
        !isfinite(limit) || limit <= 0 || !isfinite(filter_ms) || filter_ms < 0) {
        *state = (ChassisSpinPhase_s){0};
        return 0;
    }
    const float angle = remainderf(cmd->offset_angle, 360.0f);
    const uint32_t dt_ms = now_ms - state->tick_ms;
    /* Entry or a missed control interval starts without a retained lead. */
    if (!state->initialized || dt_ms > 30U) {
        *state = (ChassisSpinPhase_s){.angle_deg=angle, .tick_ms=now_ms, .initialized=true};
        return 0;
    }
    if (dt_ms != 0U) {
        const float delta = remainderf(angle - state->angle_deg, 360.0f);
        const float rate = delta * (1000.0f / (float)dt_ms);
        const float alpha = (float)dt_ms / (filter_ms + (float)dt_ms);
        state->rate_dps += alpha * (rate - state->rate_dps);
        state->angle_deg = angle;
        state->tick_ms = now_ms;
    }
    return fmaxf(-limit, fminf(limit, state->rate_dps * (prediction_ms * .001f)));
}

/* Resolve command motion only; power limits and motor output permission stay downstream. */
static inline void ChassisResolveMotion(Chassis_Ctrl_Cmd_s *cmd, float spin_wz,
                                       float phase_lead_deg, float *vx, float *vy)
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

    float command_vx = cmd->vx, command_vy = cmd->vy;
#if CHASSIS_SPIN_PHASE_COMPENSATION_ENABLE
    const float limit = CHASSIS_SPIN_PHASE_MAX_DEG;
    if (cmd->chassis_mode == CHASSIS_ROTATE && isfinite(phase_lead_deg) &&
        isfinite(limit) && limit > 0) {
        /* Lead only the operator vector. The already calibrated static trim
         * keeps its original direction when the operator releases the stick. */
        const float lead = fmaxf(-limit, fminf(limit, phase_lead_deg)) * DEGREE_2_RAD;
        const float co = arm_cos_f32(lead), si = arm_sin_f32(lead);
        command_vx = cmd->vx * co - cmd->vy * si;
        command_vy = cmd->vx * si + cmd->vy * co;
    }
#else
    (void)phase_lead_deg;
#endif
    /* Static drift is fixed in the gimbal/world frame, before the existing
     * one-time transform by the measured relative angle. */
#if CHASSIS_SPIN_DRIFT_COMPENSATION_ENABLE
    if (cmd->chassis_mode == CHASSIS_ROTATE) {
        command_vx += CHASSIS_SPIN_DRIFT_COMPENSATION_VX;
        command_vy += CHASSIS_SPIN_DRIFT_COMPENSATION_VY;
    }
#endif

    // 实测云台相对角：将云台坐标平移命令只转换一次到底盘坐标。
    const float cos_theta = arm_cos_f32(cmd->offset_angle * DEGREE_2_RAD);
    const float sin_theta = arm_sin_f32(cmd->offset_angle * DEGREE_2_RAD);
    *vx = command_vx * cos_theta - command_vy * sin_theta;
    *vy = command_vx * sin_theta + command_vy * cos_theta;
}

#endif
