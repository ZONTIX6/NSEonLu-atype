#ifndef DJI_MOTOR_DIAGNOSTICS_H
#define DJI_MOTOR_DIAGNOSTICS_H

#include <stdbool.h>
#include <stdint.h>

enum {
    DJI_CONTROL_ENABLED = 1U << 0,
    DJI_CONTROL_CAN_ATTEMPTED = 1U << 1,
    DJI_CONTROL_CAN_ACCEPTED = 1U << 2,
    DJI_CONTROL_INTEGRAL_AT_LIMIT = 1U << 3,
    DJI_CONTROL_SPEED_AT_LIMIT = 1U << 4,
    DJI_CONTROL_ANGLE_AT_LIMIT = 1U << 5,
    DJI_CONTROL_SPEED_CALCULATED = 1U << 6
};

/* One completed motor cycle. CAN acceptance means mailbox submission, not
 * confirmation that the motor applied the command. Read via the locked getter. */
typedef struct {
    uint32_t tick_ms;
    float reference, speed_dps, speed_iout;
    int16_t command;
    uint16_t flags;
    bool available;
} DJIMotorControlSnapshot_s;

#endif
