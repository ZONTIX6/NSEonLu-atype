#ifndef USB_TELEMETRY_FORMAT_H
#define USB_TELEMETRY_FORMAT_H

#include <stddef.h>
#include <stdint.h>
#include "chassis_velocity.h"

/* CSV v5 wire order; the first 111 fields retain their v4 order and units. */
#define USB_TELEMETRY_FIELDS(U, I) \
    U(version) U(seq) U(tick) U(tx_drop_count) \
    U(chassis_valid) U(chassis_tick_ms) U(chassis_mode) \
    I(vx_cmd_milli) I(vy_cmd_milli) I(wz_cmd_milli) \
    U(rc_online) U(rc_age_ms) I(rc_lx) I(rc_ly) I(rc_rx) I(rc_ry) I(rc_dial) \
    U(rc_sw_left) U(rc_sw_right) I(mouse_x) I(mouse_y) \
    U(mouse_left) U(mouse_right) U(keys) U(model_tick_ms) \
    U(motor_online_mask) U(motor_enabled_mask) \
    I(rpm_lf) I(rpm_rf) I(rpm_lb) I(rpm_rb) \
    I(target_lf_rpm) I(target_rf_rpm) I(target_lb_rpm) I(target_rb_rpm) \
    I(i_lf_cmd) I(i_rf_cmd) I(i_lb_cmd) I(i_rb_cmd) \
    I(i_lf_fb) I(i_rf_fb) I(i_lb_fb) I(i_rb_fb) \
    U(power_valid) I(p_unlimited_cw) I(p_limited_cw) I(effective_limit_cw) \
    I(p_meter_cw) U(meter_online) U(meter_age_ms) \
    I(nominal_budget_cw) I(target_power_cw) U(power_source) \
    U(referee_online) I(referee_limit_cw) U(buffer_j) U(buffer_age_ms) \
    U(buffer_control_enabled) U(buffer_estimate_valid) \
    I(buffer_lower_cj) I(buffer_predicted_cj) U(buffer_recovering) \
    U(cap_online) U(cap_output_enabled) U(cap_error) I(cap_energy_pm) \
    I(cap_chassis_cw) I(cap_limit_cw) U(cap_age_ms) \
    U(cap_input_online) I(cap_input_cw) U(cap_input_age_ms) \
    U(imu_valid) U(imu_age_ms) \
    I(imu_yaw_mdeg) I(imu_pitch_mdeg) I(imu_roll_mdeg) I(imu_yaw_total_mdeg) \
    I(imu_gx_mdps) I(imu_gy_mdps) I(imu_gz_mdps) \
    I(imu_ax_mmps2) I(imu_ay_mmps2) I(imu_az_mmps2) I(chassis_offset_mdeg) \
    U(spin_power_phase) U(spin_block_reasons) I(spin_base_power_cw) \
    I(spin_buffer_request_cw) I(spin_buffer_grant_cw) U(spin_base_confirm_ms) \
    I(spin_filtered_power_cw) I(spin_power_ceiling_cw) I(spin_slow_correction_cw) \
    I(spin_gain_milli) I(spin_rise_milli) I(spin_fall_milli) \
    U(spin_lpf_ms) U(spin_settle_ms) U(power_tuning_source) U(spin_tuning_source) \
    U(velocity_actual_valid) U(velocity_actual_tick_ms) \
    I(velocity_actual_vx_mmps) I(velocity_actual_vy_mmps) I(velocity_actual_wz_mrps) \
    U(velocity_target_valid) U(velocity_target_tick_ms) \
    I(velocity_target_vx_mmps) I(velocity_target_vy_mmps) I(velocity_target_wz_mrps) \
    I(spin_soft_wz_milli) I(spin_hard_wz_milli) I(spin_effective_wz_milli) \
    I(spin_body_hard_mrps) I(spin_wheel_hard_rpm) U(spin_limit_state) \
    U(spin_extension_enabled) U(spin_extension_qualified) U(spin_extension_confirm_ms) \
    I(spin_power_gap_cw) I(spin_max_tracking_error_rpm) I(spin_body_tracking_error_mrps) \
    U(spin_capability_latched) U(spin_capability_release_ms) \
    I(spin_peak_wheel_rpm) I(spin_peak_body_mrps)

typedef struct {
#define USB_FIELD_U(name) uint32_t name;
#define USB_FIELD_I(name) int32_t name;
    USB_TELEMETRY_FIELDS(USB_FIELD_U, USB_FIELD_I)
#undef USB_FIELD_U
#undef USB_FIELD_I
} USBTelemetrySample_s;

/* Nonfinite inputs encode zero; the caller must clear the source validity flag. */
int32_t USBTelemetryFixed(float value, float scale);
void USBTelemetryEncodeVelocity(USBTelemetrySample_s *sample,
    const ChassisVelocityFeedback_s *velocity, uint32_t now_ms);
/* Recheck source freshness at the instant a row is collected. */
void USBTelemetryApplyFreshness(USBTelemetrySample_s *sample, uint32_t now_ms);
/* Returns bytes excluding NUL, or zero with buffer[0] cleared on truncation. */
size_t USBTelemetryFormatHeader(char *buffer, size_t capacity);
size_t USBTelemetryFormatRow(const USBTelemetrySample_s *sample,
                            char *buffer, size_t capacity);

#endif
