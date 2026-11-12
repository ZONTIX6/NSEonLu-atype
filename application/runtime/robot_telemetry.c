#include "robot_telemetry.h"
#include "imu_capture_config.h"

#if IMU_CAPTURE_ONLY
#include "vofa_telemetry.h"
#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0
#include "vofa_observation.h"
#endif
#include "ins_task.h"
#include "usart.h"
#include <string.h>

void RobotTelemetryInit(void) {}

void RobotTelemetryTask(void)
{
    static ChassisFusedVelocitySnapshot_s velocity;
    static const USBTelemetrySample_s empty_sample;
    INSMotionSnapshot_s imu = {0};
    (void)INSGetMotionSnapshot(&imu);
    memset(&velocity, 0, sizeof velocity);
    ChassisVelocityDiagnostics_s *d = &velocity.diagnostics;
    d->tick_ms = HAL_GetTick();
    d->available = true;
    d->imu_tick_ms = imu.tick_ms;
    d->imu_sequence = imu.sequence;
    d->imu_valid = imu.valid;
    memcpy(d->imu_quaternion, imu.quaternion, sizeof d->imu_quaternion);
    memcpy(d->imu_accel_n_mps2, imu.accel_n_mps2, sizeof d->imu_accel_n_mps2);
    memcpy(d->imu_gyro_sensor_radps, imu.gyro_sensor_radps, sizeof d->imu_gyro_sensor_radps);
#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0
    VofaObservationSend(&empty_sample, NULL, &velocity, NULL, NULL, HAL_GetTick());
#else
    VofaTelemetrySendMeasurement(&empty_sample, NULL, &velocity, HAL_GetTick());
#endif
}

#else
#include "usb_telemetry.h"
#include "tuning_flash.h"
#include "runtime_tuning.h"
#include "chassis_control_config.h"
#include "board_config.h"

#if VOFA_TELEMETRY_ENABLE
#include "vofa_telemetry.h"
#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0
#include "vofa_observation.h"
#endif
#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED
#error VOFA telemetry requires CHASSIS_CONTROL_MODE=0 for speed-control snapshots.
#endif
#endif

#if USB_TELEMETRY_ENABLE
#include "usbd_cdc_if.h"
#endif

#if USB_TELEMETRY_ENABLE || VOFA_TELEMETRY_ENABLE
#include "chassis.h"
#include "remote_control.h"
#include "chassis_power.h"
#include "power_meter.h"
#include "rm_referee.h"
#include "super_cap.h"
#include "chassis_energy_config.h"
#include "usb_telemetry_imu.h"
#include <limits.h>
#include <math.h>
#include <string.h>

/* Only the low-priority telemetry task owns this scratch storage. */
static USBTelemetrySample_s sample;
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_SPEED
static ChassisPowerSnapshot_s model;
static ChassisTelemetrySnapshot_s chassis;
static RemoteControlSnapshot_s remote;
static RefereePowerSnapshot_s referee;
static SuperCapSnapshot_s capacitor;

static uint32_t Age(uint32_t now_ms, uint32_t tick_ms, bool known)
{
    return known ? (uint32_t)(now_ms - tick_ms) : UINT32_MAX;
}

static void Collect(uint32_t now_ms)
{
    memset(&sample, 0, sizeof sample);
    sample.rc_age_ms = sample.meter_age_ms = sample.buffer_age_ms =
        sample.cap_age_ms = sample.cap_input_age_ms = sample.imu_age_ms = UINT32_MAX;
    sample.power_source = CHASSIS_POWER_SOURCE_MODE;
    sample.power_tuning_source=RuntimeTuningSource(TUNING_POWER);
    sample.spin_tuning_source=RuntimeTuningSource(TUNING_SPIN);

    if (ChassisGetTelemetrySnapshot(&chassis)) {
        sample.chassis_valid = isfinite(chassis.vx_cmd) && isfinite(chassis.vy_cmd) &&
            isfinite(chassis.wz_cmd) && isfinite(chassis.nominal_budget_w) &&
            isfinite(chassis.target_power_w) && isfinite(chassis.offset_angle);
        sample.chassis_tick_ms = chassis.tick_ms;
        sample.chassis_mode = chassis.mode;
        sample.vx_cmd_milli = USBTelemetryFixed(chassis.vx_cmd, 1000);
        sample.vy_cmd_milli = USBTelemetryFixed(chassis.vy_cmd, 1000);
        sample.wz_cmd_milli = USBTelemetryFixed(chassis.wz_cmd, 1000);
        sample.chassis_offset_mdeg = USBTelemetryFixed(chassis.offset_angle, 1000);
        USBTelemetryEncodeVelocity(&sample,&chassis.velocity,HAL_GetTick());
        sample.nominal_budget_cw = USBTelemetryFixed(chassis.nominal_budget_w, 100);
        sample.target_power_cw = USBTelemetryFixed(chassis.target_power_w, 100);
        sample.buffer_control_enabled = chassis.buffer_control_enabled;
        sample.buffer_estimate_valid = chassis.buffer_estimate_valid &&
            isfinite(chassis.buffer_lower_j) && isfinite(chassis.buffer_predicted_j);
        sample.buffer_lower_cj = USBTelemetryFixed(chassis.buffer_lower_j, 100);
        sample.buffer_predicted_cj = USBTelemetryFixed(chassis.buffer_predicted_j, 100);
        sample.buffer_recovering = chassis.buffer_recovering;
        sample.spin_power_phase=chassis.spin_phase;
        sample.spin_block_reasons=chassis.spin_block_reasons;
        sample.spin_base_power_cw=USBTelemetryFixed(chassis.spin_base_power_w,100);
        sample.spin_buffer_request_cw=USBTelemetryFixed(chassis.spin_buffer_request_w,100);
        sample.spin_buffer_grant_cw=USBTelemetryFixed(chassis.spin_buffer_grant_w,100);
        sample.spin_base_confirm_ms=USBTelemetryFixed(chassis.spin_base_confirm_s,1000);
        sample.spin_filtered_power_cw=USBTelemetryFixed(chassis.spin_filtered_power_w,100);
        sample.spin_power_ceiling_cw=USBTelemetryFixed(chassis.spin_power_ceiling_w,100);
        sample.spin_slow_correction_cw=USBTelemetryFixed(chassis.slow_correction_w,100);
        sample.spin_gain_milli=USBTelemetryFixed(chassis.spin_gain,1000);
        sample.spin_rise_milli=USBTelemetryFixed(chassis.spin_rise,1000);
        sample.spin_fall_milli=USBTelemetryFixed(chassis.spin_fall,1000);
        sample.spin_lpf_ms=USBTelemetryFixed(chassis.spin_lpf_s,1000);
        sample.spin_settle_ms=USBTelemetryFixed(chassis.spin_settle_s,1000);
        sample.spin_soft_wz_milli=USBTelemetryFixed(chassis.spin_soft_wz,1000);
        sample.spin_hard_wz_milli=USBTelemetryFixed(chassis.spin_hard_wz,1000);
        sample.spin_effective_wz_milli=USBTelemetryFixed(chassis.spin_effective_wz,1000);
        sample.spin_body_hard_mrps=USBTelemetryFixed(chassis.spin_body_hard_rps,1000);
        sample.spin_wheel_hard_rpm=USBTelemetryFixed(chassis.spin_wheel_hard_rpm,1);
        sample.spin_limit_state=chassis.spin_limit_state;
        sample.spin_extension_enabled=chassis.spin_extension_enabled;
        sample.spin_extension_qualified=chassis.spin_extension_qualified;
        sample.spin_extension_confirm_ms=USBTelemetryFixed(chassis.spin_extension_confirm_s,1000);
        sample.spin_power_gap_cw=USBTelemetryFixed(chassis.spin_power_gap_w,100);
        sample.spin_max_tracking_error_rpm=USBTelemetryFixed(chassis.spin_max_tracking_error_rpm,1);
        sample.spin_body_tracking_error_mrps=USBTelemetryFixed(chassis.spin_body_tracking_error_rps,1000);
        sample.spin_capability_latched=chassis.spin_capability_latched;
        sample.spin_capability_release_ms=USBTelemetryFixed(chassis.spin_capability_release_s,1000);
        sample.spin_peak_wheel_rpm=USBTelemetryFixed(chassis.spin_peak_wheel_rpm,1);
        sample.spin_peak_body_mrps=USBTelemetryFixed(chassis.spin_peak_body_rps,1000);
    }
    if (RemoteControlGetSnapshot(&remote)) {
        sample.rc_online = remote.online;
        sample.rc_age_ms = Age(HAL_GetTick(), remote.last_rx_tick_ms, remote.has_sample);
        sample.rc_lx = remote.control.rc.rocker_l_;
        sample.rc_ly = remote.control.rc.rocker_l1;
        sample.rc_rx = remote.control.rc.rocker_r_;
        sample.rc_ry = remote.control.rc.rocker_r1;
        sample.rc_dial = remote.control.rc.dial;
        sample.rc_sw_left = remote.control.rc.switch_left;
        sample.rc_sw_right = remote.control.rc.switch_right;
        sample.mouse_x = remote.control.mouse.x;
        sample.mouse_y = remote.control.mouse.y;
        sample.mouse_left = remote.control.mouse.press_l != 0;
        sample.mouse_right = remote.control.mouse.press_r != 0;
        sample.keys = remote.control.key[KEY_PRESS].keys;
    }
    if (ChassisPowerGetSnapshot(&model) && model.registered) {
        now_ms = HAL_GetTick();
        sample.model_tick_ms = model.tick_ms;
        sample.motor_online_mask = model.online_mask & 15U;
        sample.motor_enabled_mask = model.telemetry.motor_enabled_mask & 15U;
        float unlimited_w = 0;
        bool finite_motors = true;
        for (unsigned i = 0; i < 4; ++i) {
            unlimited_w += model.power.predicted_unlimited_power_w[i];
            if ((uint32_t)(now_ms - model.feedback_tick_ms[i]) > 20U)
                sample.motor_online_mask &= ~(1U << i);
            if (!isfinite(model.telemetry.actual_speed_rpm[i]) ||
                !isfinite(model.telemetry.target_speed_rpm[i]) ||
                !isfinite(model.telemetry.current_command[i]) ||
                !isfinite(model.feedback_current[i])) {
                sample.motor_online_mask &= ~(1U << i);
                finite_motors = false;
            }
        }
        sample.rpm_lf = USBTelemetryFixed(model.telemetry.actual_speed_rpm[0], 1);
        sample.rpm_rf = USBTelemetryFixed(model.telemetry.actual_speed_rpm[1], 1);
        sample.rpm_lb = USBTelemetryFixed(model.telemetry.actual_speed_rpm[2], 1);
        sample.rpm_rb = USBTelemetryFixed(model.telemetry.actual_speed_rpm[3], 1);
        sample.target_lf_rpm = USBTelemetryFixed(model.telemetry.target_speed_rpm[0], 1);
        sample.target_rf_rpm = USBTelemetryFixed(model.telemetry.target_speed_rpm[1], 1);
        sample.target_lb_rpm = USBTelemetryFixed(model.telemetry.target_speed_rpm[2], 1);
        sample.target_rb_rpm = USBTelemetryFixed(model.telemetry.target_speed_rpm[3], 1);
        sample.i_lf_cmd = USBTelemetryFixed(model.telemetry.current_command[0], 1);
        sample.i_rf_cmd = USBTelemetryFixed(model.telemetry.current_command[1], 1);
        sample.i_lb_cmd = USBTelemetryFixed(model.telemetry.current_command[2], 1);
        sample.i_rb_cmd = USBTelemetryFixed(model.telemetry.current_command[3], 1);
        sample.i_lf_fb = USBTelemetryFixed(model.feedback_current[0], 1);
        sample.i_rf_fb = USBTelemetryFixed(model.feedback_current[1], 1);
        sample.i_lb_fb = USBTelemetryFixed(model.feedback_current[2], 1);
        sample.i_rb_fb = USBTelemetryFixed(model.feedback_current[3], 1);
        sample.power_valid = model.power.status == POWER_MODEL_STATUS_OK &&
            !model.numeric_error && finite_motors && isfinite(unlimited_w) &&
            isfinite(model.telemetry.predicted_total_power_w) &&
            isfinite(model.power.effective_limit_w);
        sample.p_unlimited_cw = USBTelemetryFixed(unlimited_w, 100);
        sample.p_limited_cw = USBTelemetryFixed(model.telemetry.predicted_total_power_w, 100);
        sample.effective_limit_cw = USBTelemetryFixed(model.power.effective_limit_w, 100);
    }
    PowerMeterStatus_s meter = {0};
    if (PowerMeterGetStatus(&meter)) {
        sample.meter_online = meter.online && isfinite(meter.power_w);
        sample.meter_age_ms = Age(HAL_GetTick(), meter.last_rx_tick_ms,
                                 meter.online || meter.last_rx_tick_ms != 0);
        sample.p_meter_cw = USBTelemetryFixed(meter.power_w, 100);
    }
    memset(&referee, 0, sizeof referee);
    if (RefereeGetPowerSnapshot(&referee)) {
        sample.referee_online = referee.valid;
        sample.referee_limit_cw = (int32_t)referee.chassis_power_limit_w * 100;
        sample.buffer_j = referee.buffer_energy_j;
        sample.buffer_age_ms = Age(HAL_GetTick(), referee.buffer_tick_ms,
                                  referee.valid || referee.buffer_tick_ms != 0);
    }
    memset(&capacitor, 0, sizeof capacitor);
    if (SuperCapGetSnapshot(HAL_GetTick(), &capacitor)) {
        const SuperCapStatus_s cap = capacitor.status;
        sample.cap_online = cap.online && isfinite(cap.energy_ratio) && isfinite(cap.chassis_power_w);
        sample.cap_output_enabled = cap.output_enabled;
        sample.cap_error = cap.error_code;
        sample.cap_energy_pm = USBTelemetryFixed(cap.energy_ratio, 1000);
        sample.cap_chassis_cw = USBTelemetryFixed(cap.chassis_power_w, 100);
        sample.cap_limit_cw = (int32_t)cap.available_power_limit_w * 100;
        sample.cap_age_ms = Age(HAL_GetTick(), cap.last_rx_tick_ms,
                               cap.online || cap.last_rx_tick_ms != 0);
    }
    if (capacitor.input_feedback_enabled) {
        const SuperCapInputPower_s cap_input = capacitor.input_power;
        sample.cap_input_online = cap_input.online && isfinite(cap_input.input_power_w);
        sample.cap_input_cw = USBTelemetryFixed(cap_input.input_power_w, 100);
        sample.cap_input_age_ms = Age(HAL_GetTick(), cap_input.last_rx_tick_ms,
                                     cap_input.online || cap_input.last_rx_tick_ms != 0);
    }
    INSSnapshot_s imu = {0};
    (void)INSGetSnapshot(&imu);
    USBTelemetryEncodeIMU(&sample, &imu, HAL_GetTick());
    USBTelemetryApplyFreshness(&sample, HAL_GetTick());
}
#endif
#endif

void RobotTelemetryInit(void)
{
#if USB_TELEMETRY_ENABLE
    USBTelemetryStreamInit();
#endif
}

void RobotTelemetryTask(void)
{
#if USB_TELEMETRY_ENABLE
    /* Let the preceding USB reply finish before a potentially long sector erase. */
    if (!CDC_IsPortOpen_FS() || CDC_IsTransmitIdle_FS()) TuningFlashPoll();
#endif
#if (USB_TELEMETRY_ENABLE || VOFA_TELEMETRY_ENABLE) && CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_SPEED
    Collect(HAL_GetTick());
#endif
#if VOFA_TELEMETRY_ENABLE
#if VOFA_TELEMETRY_MODE == 1 || VOFA_TELEMETRY_MODE == 2
#if CHASSIS_VELOCITY_FUSION_ENABLE
    static ChassisFusedVelocitySnapshot_s velocity;
    /* The getter also copies source diagnostics when the estimate is invalid. */
    (void)ChassisGetFusedVelocitySnapshot(&velocity);
    const ChassisFusedVelocitySnapshot_s *velocity_snapshot = &velocity;
#else
    const ChassisFusedVelocitySnapshot_s *velocity_snapshot = NULL;
#endif
#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0
    VofaObservationSend(&sample, model.registered ? &model : NULL,
                        velocity_snapshot, &capacitor, &referee, HAL_GetTick());
#else
    VofaTelemetrySendMeasurement(&sample, model.registered ? &model : NULL,
                                 velocity_snapshot, HAL_GetTick());
#endif
#else
    VofaTelemetrySend(&sample);
#endif
#endif
#if USB_TELEMETRY_ENABLE
    USBTelemetryStreamSend(&sample, HAL_GetTick());
#endif
}
#endif /* IMU_CAPTURE_ONLY */
