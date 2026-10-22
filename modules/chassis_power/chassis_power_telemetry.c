#include "chassis_power_internal.h"
#include "chassis_power_config.h"
#include "chassis_power_capture.h"
#if CHASSIS_POWER_CAPTURE_ENABLE
#include "bsp_dwt.h"
#include "bsp_uart_log.h"
#include "board_config.h"
static ChassisPowerCapture_s chassis_capture;
#endif

void ChassisPowerTelemetryFinal(ChassisPowerInstance_s *instance)
{
    const ChassisPowerSnapshot_s *state=&instance->working_state;
    const ChassisPowerOutput_s *power_output=&state->power;
    const ChassisPowerTelemetry_s *telemetry=&state->telemetry;
    ChassisPowerControlTelemetry_s control_telemetry=instance->control.telemetry;
    const ChassisPowerControlSnapshot_s cycle_control=instance->control;
    const uint32_t now_ms=state->tick_ms, meter_tick_ms=state->meter_tick_ms;
    const float measured_chassis_power_w=state->measured_power_w;
    const bool meter_online=state->meter_online;
    (void)cycle_control;
    if(instance->config.stage>=1) {
        control_telemetry.corrected_budget_w=instance->input.referee_power_limit_w;
        control_telemetry.correction_w=state->transient.slow_applied_w+state->transient.fast_w;
    }
#if CHASSIS_POWER_CAPTURE_ENABLE
    ChassisPowerCaptureSample_s sample = {.tick_ms = now_ms, .meter_tick_ms = meter_tick_ms,
        .budget_w = cycle_control.nominal_budget_w, .measured_w = measured_chassis_power_w,
        .slow_w = state->transient.slow_applied_w, .fast_w = state->transient.fast_w,
        .state = (uint8_t)state->transient.mode,
        .excess_j = state->transient.excess_j, .allowance_j = state->transient.allowance_j,
        .drive_mask = state->transient.drive_mask, .brake_mask = state->transient.brake_mask,
        .enabled_mask = telemetry->motor_enabled_mask,
        .meter_age_ms = (uint8_t)fminf(255, (float)(uint32_t)(now_ms - meter_tick_ms)),
        .flags = (uint8_t)((state->numeric_error ? 1 : 0) | (state->transient.guard_infeasible ? 2 : 0) |
                          (state->transient.growth_fallback ? 4 : 0) |
                          (state->transient.guard_search_exhausted ? 8 : 0) | (CHASSIS_POWER_CONTROL_STAGE << 4))};
    for (unsigned i = 0; i < 4; ++i) {
        sample.raw_current[i] = state->raw_current[i];
        sample.final_current[i] = power_output->limited_current[i];
        sample.feedback_current[i] = state->feedback_current[i];
        sample.actual_rpm[i] = telemetry->actual_speed_rpm[i];
        sample.target_rpm[i] = telemetry->target_speed_rpm[i];
        sample.motor_age_ms[i] = (uint8_t)fminf(255, (float)(uint32_t)(now_ms - state->feedback_tick_ms[i]));
        sample.actual_model_w += power_output->predicted_limited_power_w[i];
        sample.guard_model_w += PowerModelPredict(&instance->config.models[i],
                                                  power_output->limited_current[i], state->transient.guard_rpm[i]);
    }
    ChassisPowerCaptureUpdate(&chassis_capture, &sample);
#endif

    if (++instance->telemetry_count >= CHASSIS_POWER_RTT_DIVIDER)
    {
        instance->telemetry_count = 0U;
        RTTTelemetryWriteChassisPowerRow(
            power_output,
            telemetry,
            &control_telemetry,
            now_ms,
            measured_chassis_power_w,
            meter_online,
            meter_tick_ms);
    }
}

void ChassisPowerCaptureDrain(void)
{
#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED
    BSPUARTLogPoll();
    uint8_t frame[CHASSIS_POWER_CAPTURE_FRAME_BYTES];
    const uint32_t now = HAL_GetTick();
    POWER_LOCK();
    if (BSPUARTLogIsIdle()) ChassisPowerCaptureDrainComplete(&chassis_capture);
    const bool ready = chassis_power != NULL &&
        ChassisPowerCaptureNextFrame(&chassis_capture, now,
                                    !chassis_power->control.control_enabled, frame);
    POWER_UNLOCK();
    if (ready && BSPUARTLogWrite(frame, sizeof(frame)) == sizeof(frame)) {
        POWER_LOCK();
        ChassisPowerCaptureFrameAccepted(&chassis_capture, now);
        POWER_UNLOCK();
    }
#endif
}
