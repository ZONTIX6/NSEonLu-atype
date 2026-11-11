#ifndef CHASSIS_GIMBAL_IMU_H
#define CHASSIS_GIMBAL_IMU_H

#include <stdbool.h>

typedef struct {
    float yaw_origin_b_m[3];      /* Chassis center to yaw axis, in chassis axes. */
    float pitch_origin_yaw_m[3];  /* Yaw axis to pitch axis, in yaw frame axes. */
    float imu_origin_head_m[3];   /* Pitch axis to IMU, in head axes. */
    float sensor_to_head_quaternion[4];
} ChassisGimbalImuGeometry_s;

typedef struct {
    float body_to_nav[3][3];      /* R_NB, multiplying a column vector in chassis axes. */
    float imu_offset_nav_m[3];    /* Chassis center to IMU, in navigation axes. */
} ChassisGimbalImuPose_s;

/* Chassis and zero-angle gimbal axes are +X forward, +Y left, +Z up.
 * Yaw is about chassis +Z; pitch is about the yaw frame's +Y (nose down).
 * Quaternions are Hamilton wxyz and rotate vectors from source to destination.
 * A small norm error is normalized; nonfinite input or a quaternion with norm
 * squared outside [0.98, 1.02] returns false. Output is written only on success. */
bool ChassisGimbalImuPose(const ChassisGimbalImuGeometry_s *geometry,
                         const float sensor_to_nav_quaternion[4],
                         float yaw_rad, float pitch_rad,
                         ChassisGimbalImuPose_s *pose);

#endif
