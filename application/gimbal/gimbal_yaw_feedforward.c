#include "gimbal_yaw_feedforward.h"
#include "../../config/03_gimbal/gimbal_config.h"
#include <math.h>
#include <stddef.h>

float GimbalYawFeedforwardUpdate(GimbalYawFeedforward_s *state,
    bool head_enabled, bool spin_requested, bool wheel_valid,
    float wheel_wz_radps, uint32_t wheel_tick_ms, uint32_t now_ms)
{
    if (!state) return 0;
    const float gain=YAW_SPIN_FEEDFORWARD_GAIN, limit=YAW_SPIN_FEEDFORWARD_MAX_COMMAND;
    const float tau_ms=YAW_SPIN_FEEDFORWARD_TIME_CONSTANT_MS;
    if (!YAW_SPIN_FEEDFORWARD_ENABLE || !head_enabled || !wheel_valid ||
        (uint32_t)(now_ms-wheel_tick_ms)>YAW_SPIN_FEEDFORWARD_MAX_AGE_MS ||
        !isfinite(wheel_wz_radps) || wheel_wz_radps<0 ||
        !isfinite(gain) || gain<=0 || !isfinite(limit) || limit<=0 ||
        !isfinite(tau_ms) || tau_ms<=0) {
        *state=(GimbalYawFeedforward_s){0};
        return 0;
    }
    /* Do not resume a held command after a gap in task updates. */
    if ((uint32_t)(now_ms-state->last_tick_ms)>YAW_SPIN_FEEDFORWARD_MAX_AGE_MS)
        *state=(GimbalYawFeedforward_s){0};
    if (!state->active && state->output==0) state->last_tick_ms=now_ms;
    if (spin_requested) state->active=true;
    else if (wheel_wz_radps<=YAW_SPIN_FEEDFORWARD_STOP_RADPS) state->active=false;
    /* Clamp speed before multiplying, including for finite extreme inputs. */
    const float target=state->active ? fminf(wheel_wz_radps,limit/gain)*gain : 0;
    const uint32_t dt_ms=now_ms-state->last_tick_ms;
    state->last_tick_ms=now_ms;
    const float alpha=(float)dt_ms/(tau_ms+(float)dt_ms);
    state->output+=alpha*(target-state->output);
    /* Below one integer CAN command, finish the normal-stop decay. The latch
     * stays released even if ordinary follow motion resumes during the tail. */
    if (!state->active && state->output<1.0f) {
        *state=(GimbalYawFeedforward_s){0};
        return 0;
    }
    return state->output;
}
