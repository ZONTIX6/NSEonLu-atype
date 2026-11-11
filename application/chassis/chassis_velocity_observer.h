#ifndef CHASSIS_VELOCITY_OBSERVER_H
#define CHASSIS_VELOCITY_OBSERVER_H

#include "chassis_velocity.h"
#include "chassis_remote_fusion.h"

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
    CHASSIS_VELOCITY_FUSED,
    CHASSIS_VELOCITY_CONFIG_INVALID
} ChassisFusedVelocityStatus_e;

/* Raw inputs captured together before the observer runs. Source timestamps are
 * receive/read-completion times; they do not imply simultaneous sensor samples.
 * Available also without calibration. Consumers check validity and freshness. */
typedef struct {
    uint32_t tick_ms;
    float raw_rpm[4]; /* LF/RF/LB/RB, motor rotor rpm. */
    uint32_t wheel_tick_ms[4];
    uint8_t wheel_online_mask;
    float imu_quaternion[4]; /* Unit [w,x,y,z], sensor to navigation frame. */
    float imu_accel_n_mps2[3]; /* Gravity-free sensor acceleration, before lever correction. */
    float imu_gyro_sensor_radps[3];
    uint32_t imu_tick_ms, imu_sequence; /* Remote mode: receiver estimate, not source-board time. */
    bool imu_valid;
    uint16_t joint_ecd[2]; /* Raw yaw/pitch motor encoders. */
    int16_t joint_rpm[2];
    uint32_t joint_tick_ms[2];
    bool joints_valid, available;
} ChassisVelocityDiagnostics_s;

typedef struct {
    ChassisVelocity_s wheel; /* Latest raw wheel-only inverse kinematics. */
    float vx_mps, vy_mps;   /* Estimate in the chassis frame, forward/right positive. */
    float paired_wheel_mps[2]; /* Forward/right, aligned wheel sample when fused. */
    float center_accel_n_mps2[2]; /* Valid when status == FUSED. */
    uint32_t tick_ms; /* Epoch of the estimate; fused output has alignment latency. */
    ChassisFusedVelocityStatus_e status;
    bool valid;
    ChassisVelocityDiagnostics_s diagnostics;
    ChassisRemoteFusionOutput_s remote; /* Remote timing/pairing diagnostics; zero in local modes. */
} ChassisFusedVelocitySnapshot_s;

#if CHASSIS_VELOCITY_FUSION_ENABLE
/* Debugger can sample this symbol. Firmware readers should use the getter. */
extern volatile ChassisFusedVelocitySnapshot_s chassis_fused_velocity;
bool ChassisGetFusedVelocitySnapshot(ChassisFusedVelocitySnapshot_s *snapshot);
#endif

#endif
