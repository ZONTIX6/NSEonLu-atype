#include "referee_power.h"

#include <stddef.h>
#include <string.h>

static uint16_t ReadU16LE(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] |
                      ((uint16_t)bytes[1] << 8U));
}

void RefereePowerStateReset(RefereePowerState_s *state)
{
    if (state != NULL)
    {
        memset(state, 0, sizeof(*state));
    }
}

bool RefereePowerStateAcceptFrame(RefereePowerState_s *state,
                                  uint16_t command_id,
                                  const uint8_t *payload,
                                  uint16_t payload_length,
                                  uint32_t now_ms)
{
    if (state == NULL || payload == NULL)
    {
        return false;
    }

    if (command_id == REFEREE_ROBOT_STATE_COMMAND_ID)
    {
        if (payload_length != REFEREE_ROBOT_STATE_PAYLOAD_LENGTH)
        {
            return false;
        }

        state->chassis_power_limit_w = ReadU16LE(&payload[10]);
        state->chassis_output_enabled = (payload[12] & 0x02U) != 0U;
        state->robot_state_tick_ms = now_ms;
        state->has_robot_state = true;
        return true;
    }

    if (command_id == REFEREE_POWER_HEAT_COMMAND_ID)
    {
        if (payload_length != REFEREE_POWER_HEAT_PAYLOAD_LENGTH)
        {
            return false;
        }

        state->buffer_energy_j = ReadU16LE(&payload[8]);
        state->power_heat_tick_ms = now_ms;
        state->has_power_heat = true;
        return true;
    }

    return false;
}

bool RefereePowerBuildSnapshot(const RefereePowerState_s *state,
                               uint32_t now_ms,
                               RefereePowerSnapshot_s *snapshot)
{
    if (snapshot == NULL)
    {
        return false;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (state == NULL)
    {
        return false;
    }

    if (!state->has_robot_state || !state->has_power_heat ||
        (uint32_t)(now_ms - state->robot_state_tick_ms) >
            REFEREE_POWER_DATA_TIMEOUT_MS ||
        (uint32_t)(now_ms - state->power_heat_tick_ms) >
            REFEREE_POWER_DATA_TIMEOUT_MS)
    {
        return true;
    }

    snapshot->valid = true;
    snapshot->chassis_output_enabled = state->chassis_output_enabled;
    snapshot->chassis_power_limit_w = state->chassis_power_limit_w;
    snapshot->buffer_energy_j = state->buffer_energy_j;
    return true;
}
