#ifndef SUPER_CAP_TYPES_H
#define SUPER_CAP_TYPES_H

#include "super_cap_protocol.h"

typedef struct
{
    SuperCapStatus_s status;
    SuperCapInputPower_s input_power;
    uint32_t received_frame_count;
    uint32_t rejected_frame_count;
    uint32_t input_received_frame_count;
    uint32_t input_rejected_frame_count;
    uint32_t command_fail_count;
    uint32_t snapshot_tick_ms;
    bool input_feedback_enabled;
    SuperCapCommand_s last_command;
    uint32_t command_attempt_tick_ms, command_attempt_sequence;
    bool command_attempted, command_submitted;
} SuperCapSnapshot_s;

#endif
