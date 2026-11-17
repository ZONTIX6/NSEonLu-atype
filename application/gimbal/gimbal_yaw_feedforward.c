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
    if (!YAW_SPIN_FEEDFORWARD_ENABLE || !head_enabled || !wheel_valid ||
        (uint32_t)(now_ms-wheel_tick_ms)>YAW_SPIN_FEEDFORWARD_MAX_AGE_MS ||
        !isfinite(wheel_wz_radps) || !isfinite(gain) || gain<=0 ||
        !isfinite(limit) || limit<=0) {
        state->active=false;
        return 0;
    }
    if (spin_requested) state->active=true;
    else if (wheel_wz_radps<=YAW_SPIN_FEEDFORWARD_STOP_RADPS) state->active=false;
    if (!state->active || wheel_wz_radps<=0) return 0;
    /* Clamp speed before multiplying, including for finite extreme inputs. */
    return fminf(wheel_wz_radps,limit/gain)*gain;
}
