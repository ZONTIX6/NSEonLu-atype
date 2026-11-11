#ifndef CHASSIS_VELOCITY_H
#define CHASSIS_VELOCITY_H

#include <stdbool.h>
#include <stdint.h>

#include "../../config/30_velocity_observation/chassis_velocity_config.h"

typedef struct {
    bool valid;
    uint32_t tick_ms; /* Oldest of the four CAN feedback timestamps. */
    float vx_mps, vy_mps; /* Geometric center: forward/right positive, m/s. */
    float wz_rpm; /* Signed body revolutions/minute; same positive spin convention as commands. */
} ChassisVelocity_s;

typedef struct {
    ChassisVelocity_s target;
    ChassisVelocity_s actual;
} ChassisVelocityFeedback_s;

typedef struct {
    bool omni_x;
    float wheel_radius_m, reduction_ratio, wheel_base_m, track_width_m;
    float direction[4]; /* LF/RF/LB/RB, +/-1; used for omni only. */
} ChassisVelocityConfig_s;

typedef struct {
    float rpm[4]; /* Raw rotor CAN feedback, including while motors are stopped. */
    uint32_t feedback_tick_ms[4], now_ms;
    uint8_t online_mask; /* Bit set only after a received sample and while online. */
} ChassisVelocityInput_s;

/* On failure, clears output including valid. No filtering or command scaling. */
bool ChassisEstimateVelocity(const ChassisVelocityConfig_s *config,
                             const ChassisVelocityInput_s *input,
                             ChassisVelocity_s *output);

#endif
