#ifndef CHASSIS_FORCE_TYPES_H
#define CHASSIS_FORCE_TYPES_H
#include <stdint.h>
#define CHASSIS_FORCE_COMMAND_MAX_AGE_MS 10U
enum { CHASSIS_FORCE_ZERO=0, CHASSIS_FORCE_TRANSLATE=1, CHASSIS_FORCE_SPIN=2 };
enum { CHASSIS_FORCE_SOURCE_REMOTE=0, CHASSIS_FORCE_SOURCE_DBUS_KEYS=1, CHASSIS_FORCE_SOURCE_VTX=2 };
typedef struct {
    uint32_t command_seq;
    float input_x,input_y,force_scale,relative_yaw_rad;
    uint32_t source_age_ms,aux_age_ms,yaw_age_ms;
    uint8_t motion_mode,input_permitted,safety_reset,aux_stopped,bench_permitted;
    int8_t spin_direction;
    uint8_t source_id;
} ChassisForceIntent_s;
#endif
