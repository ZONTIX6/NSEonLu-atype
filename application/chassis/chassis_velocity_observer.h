#ifndef CHASSIS_VELOCITY_OBSERVER_H
#define CHASSIS_VELOCITY_OBSERVER_H

#include "chassis_velocity.h"

typedef enum {
    CHASSIS_VELOCITY_NO_SAMPLE,
    CHASSIS_VELOCITY_WHEEL_UNAVAILABLE,
    CHASSIS_VELOCITY_UNCALIBRATED,
    CHASSIS_VELOCITY_CLOCK_INVALID,
    CHASSIS_VELOCITY_IMU_UNAVAILABLE,
    CHASSIS_VELOCITY_GIMBAL_UNAVAILABLE,
    CHASSIS_VELOCITY_NOT_ALIGNED,
    CHASSIS_VELOCITY_WARMUP,
    CHASSIS_VELOCITY_REJECTED,
    CHASSIS_VELOCITY_FUSED
} ChassisFusedVelocityStatus_e;

typedef struct {
    ChassisVelocity_s wheel; /* Latest raw wheel-only inverse kinematics. */
    float vx_mps, vy_mps;   /* Estimate in the chassis frame, forward/right positive. */
    float paired_wheel_mps[2]; /* Forward/right, aligned wheel sample when fused. */
    float center_accel_n_mps2[2]; /* Valid when status == FUSED. */
    uint32_t tick_ms; /* Epoch of the estimate; fused output has alignment latency. */
    ChassisFusedVelocityStatus_e status;
    bool valid;
} ChassisFusedVelocitySnapshot_s;

#if CHASSIS_VELOCITY_FUSION_ENABLE
/* Debugger can sample this symbol. Firmware readers should use the getter. */
extern volatile ChassisFusedVelocitySnapshot_s chassis_fused_velocity;
bool ChassisGetFusedVelocitySnapshot(ChassisFusedVelocitySnapshot_s *snapshot);
#endif

#endif
