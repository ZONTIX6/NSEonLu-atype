#ifndef CHASSIS_VELOCITY_CENTER_H
#define CHASSIS_VELOCITY_CENTER_H

#include <stdint.h>

typedef struct {
    uint32_t imu_us, wheel_us;
    float imu_offset_n_m[3];
    float accel_n_mps2[3];
    float wheel_b_mps[2]; /* forward, left */
    float body_to_nav_xy[2][2];
} ChassisVelocityCenterSample_s;

typedef struct {
    ChassisVelocityCenterSample_s samples[2];
    unsigned count;
} ChassisVelocityCenter_s;

typedef enum {
    CHASSIS_CENTER_WAIT,
    CHASSIS_CENTER_READY,
    CHASSIS_CENTER_RESET
} ChassisVelocityCenterResult_e;

/* Input acceleration is gravity-free at the sensor. On READY, output is the
 * middle sample with acceleration translated to the chassis center. */
ChassisVelocityCenterResult_e ChassisVelocityCenterUpdate(ChassisVelocityCenter_s *state,
    const ChassisVelocityCenterSample_s *sample, ChassisVelocityCenterSample_s *center);

#endif
