#ifndef TEST_OBSERVER_PLATFORM_H
#define TEST_OBSERVER_PLATFORM_H

#include "chassis_velocity.h"

/* Replace the hardware declarations that chassis_internal.h normally pulls in. */
#define CHASSIS_INTERNAL_H
typedef struct {
    struct {
        int16_t raw_rpm;
        uint32_t rx_sequence, rx_tick_ms, rx_cycles;
    } measure;
    const bool *daemon;
} DJIMotorInstance;

extern DJIMotorInstance *motor_lf, *motor_rf, *motor_lb, *motor_rb;
bool DaemonIsOnline(const bool *daemon);
uint32_t HAL_GetTick(void);
void ChassisObserveFusedVelocity(const ChassisVelocityConfig_s *config, uint8_t chassis_mode,
    bool trial_speed_limited, bool trial_feedback_active);

#endif
