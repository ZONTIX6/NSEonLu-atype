#ifndef GIMBAL_YAW_FEEDFORWARD_H
#define GIMBAL_YAW_FEEDFORWARD_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool active; /* Armed by spin; retained during its measured deceleration. */
    float output; /* Smoothed DJI command, including the normal-stop tail. */
    uint32_t last_tick_ms;
} GimbalYawFeedforward_s;

/* Positive rotation is the only direction calibrated so far. Invalid/stale
 * wheel feedback, reverse rotation or a disabled head immediately clears all
 * state. Normal entry/deceleration/stop use elapsed-time first-order smoothing. */
float GimbalYawFeedforwardUpdate(GimbalYawFeedforward_s *state,
    bool head_enabled, bool spin_requested, bool wheel_valid,
    float wheel_wz_radps, uint32_t wheel_tick_ms, uint32_t now_ms);

#endif
