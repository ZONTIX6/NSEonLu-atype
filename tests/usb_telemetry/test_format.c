#include "usb_telemetry_format.h"
#include "usb_telemetry_imu.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

static const char expected_header[] =
    "version,seq,tick,tx_drop_count,chassis_valid,chassis_tick_ms,chassis_mode,vx_cmd_milli,vy_cmd_milli,wz_cmd_milli,rc_online,rc_age_ms,rc_lx,rc_ly,rc_rx,rc_ry,rc_dial,rc_sw_left,rc_sw_right,mouse_x,mouse_y,mouse_left,mouse_right,keys,model_tick_ms,motor_online_mask,motor_enabled_mask,rpm_lf,rpm_rf,rpm_lb,rpm_rb,target_lf_rpm,target_rf_rpm,target_lb_rpm,target_rb_rpm,i_lf_cmd,i_rf_cmd,i_lb_cmd,i_rb_cmd,i_lf_fb,i_rf_fb,i_lb_fb,i_rb_fb,power_valid,p_unlimited_cw,p_limited_cw,effective_limit_cw,p_meter_cw,meter_online,meter_age_ms,nominal_budget_cw,target_power_cw,power_source,referee_online,referee_limit_cw,buffer_j,buffer_age_ms,buffer_control_enabled,buffer_estimate_valid,buffer_lower_cj,buffer_predicted_cj,buffer_recovering,cap_online,cap_output_enabled,cap_error,cap_energy_pm,cap_chassis_cw,cap_limit_cw,cap_age_ms,cap_input_online,cap_input_cw,cap_input_age_ms,imu_valid,imu_age_ms,imu_yaw_mdeg,imu_pitch_mdeg,imu_roll_mdeg,imu_yaw_total_mdeg,imu_gx_mdps,imu_gy_mdps,imu_gz_mdps,imu_ax_mmps2,imu_ay_mmps2,imu_az_mmps2,chassis_offset_mdeg,spin_power_phase,spin_block_reasons,spin_base_power_cw,spin_buffer_request_cw,spin_buffer_grant_cw,spin_base_confirm_ms,spin_filtered_power_cw,spin_power_ceiling_cw,spin_slow_correction_cw,spin_gain_milli,spin_rise_milli,spin_fall_milli,spin_lpf_ms,spin_settle_ms,power_tuning_source,spin_tuning_source,velocity_actual_valid,velocity_actual_tick_ms,velocity_actual_vx_mmps,velocity_actual_vy_mmps,velocity_actual_wz_mrps,velocity_target_valid,velocity_target_tick_ms,velocity_target_vx_mmps,velocity_target_vy_mmps,velocity_target_wz_mrps\r\n";

static USBTelemetrySample_s fixture(void)
{
    USBTelemetrySample_s s = {0};
    s.version = 4; s.seq = 42; s.tick = 1000; s.tx_drop_count = 3;
    s.chassis_valid = 1; s.chassis_tick_ms = 995; s.chassis_mode = 2;
    s.vx_cmd_milli = -1250; s.vy_cmd_milli = 2500; s.wz_cmd_milli = -3000;
    s.rc_online = 1; s.rc_age_ms = 5; s.rc_lx = -660; s.rc_ly = 330;
    s.rc_rx = 660; s.rc_ry = -330; s.rc_dial = -100;
    s.rc_sw_left = 1; s.rc_sw_right = 3; s.mouse_x = -20; s.mouse_y = 30;
    s.mouse_left = 1; s.keys = 32769; s.model_tick_ms = 999;
    s.motor_online_mask = 15; s.motor_enabled_mask = 7;
    s.rpm_lf = -1234; s.rpm_rf = 1200; s.rpm_lb = -900; s.rpm_rb = 800;
    s.target_lf_rpm = -1500; s.target_rf_rpm = 1500;
    s.target_lb_rpm = -1000; s.target_rb_rpm = 1000;
    s.i_lf_cmd = -120; s.i_rf_cmd = 121; s.i_lb_cmd = -122; s.i_rb_cmd = 123;
    s.i_lf_fb = -110; s.i_rf_fb = 111; s.i_lb_fb = -112; s.i_rb_fb = 113;
    s.power_valid = 1; s.p_unlimited_cw = 2000; s.p_limited_cw = 1500;
    s.effective_limit_cw = 1800; s.p_meter_cw = 1234; s.meter_online = 1;
    s.meter_age_ms = 2; s.nominal_budget_cw = 8000; s.target_power_cw = 7840;
    s.power_source = 1; s.referee_online = 1; s.referee_limit_cw = 6000;
    s.buffer_j = 42; s.buffer_age_ms = 7; s.buffer_control_enabled = 1;
    s.buffer_estimate_valid = 1; s.buffer_lower_cj = 4123;
    s.buffer_predicted_cj = 3999; s.buffer_recovering = 1;
    s.cap_online = 1; s.cap_output_enabled = 1; s.cap_energy_pm = 750;
    s.cap_chassis_cw = 1278; s.cap_limit_cw = 12000; s.cap_age_ms = 3;
    s.cap_input_online = 1; s.cap_input_cw = 5678; s.cap_input_age_ms = 4;
    s.imu_valid = 1; s.imu_age_ms = 3;
    s.imu_yaw_mdeg = -12345; s.imu_pitch_mdeg = 1250; s.imu_roll_mdeg = -2500;
    s.imu_yaw_total_mdeg = 721234; s.imu_gx_mdps = -180500; s.imu_gy_mdps = 250;
    s.imu_gz_mdps = 90000; s.imu_ax_mmps2 = -1500; s.imu_ay_mmps2 = 2000;
    s.imu_az_mmps2 = 9807; s.chassis_offset_mdeg = -30500;
    s.spin_power_phase=2;s.spin_block_reasons=4;s.spin_base_power_cw=7840;
    s.spin_buffer_request_cw=6000;s.spin_buffer_grant_cw=2000;s.spin_base_confirm_ms=100;
    s.spin_filtered_power_cw=7800;s.spin_power_ceiling_cw=10000;s.spin_slow_correction_cw=-100;
    s.spin_gain_milli=80000;s.spin_rise_milli=1200000;s.spin_fall_milli=2400000;
    s.spin_lpf_ms=50;s.spin_settle_ms=50;s.power_tuning_source=2;s.spin_tuning_source=3;
    const ChassisVelocityFeedback_s velocity={
        .actual={.valid=true,.tick_ms=995,.vx_mps=1.234f,.vy_mps=-.567f,.wz_rps=-1.25f},
        .target={.valid=true,.tick_ms=1000,.vx_mps=1.5f,.vy_mps=-.6f,.wz_rps=-1.5f}};
    USBTelemetryEncodeVelocity(&s,&velocity,1000);
    return s;
}

int main(int argc, char **argv)
{
    char header[2048], row[2048];
    USBTelemetrySample_s s = fixture();
    assert(USBTelemetryFormatHeader(header, sizeof header) == strlen(expected_header));
    assert(strcmp(header, expected_header) == 0);
    assert(strstr(header,"spin_power_phase,spin_block_reasons,spin_base_power_cw")!=NULL);
    assert(USBTelemetryFormatRow(&s, row, sizeof row) > 0);
    if (argc > 1 && strcmp(argv[1], "--fixture") == 0) {
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
        fputs(header, stdout); fputs(row, stdout); return 0;
    }
    assert(strcmp(row,
        "4,42,1000,3,1,995,2,-1250,2500,-3000,1,5,-660,330,660,-330,-100,1,3,-20,30,1,0,32769,999,15,7,-1234,1200,-900,800,-1500,1500,-1000,1000,-120,121,-122,123,-110,111,-112,113,1,2000,1500,1800,1234,1,2,8000,7840,1,1,6000,42,7,1,1,4123,3999,1,1,1,0,750,1278,12000,3,1,5678,4,1,3,-12345,1250,-2500,721234,-180500,250,90000,-1500,2000,9807,-30500,2,4,7840,6000,2000,100,7800,10000,-100,80000,1200000,2400000,50,50,2,3,1,995,1234,-567,-1250,1,1000,1500,-600,-1500\r\n") == 0);
    unsigned commas = 0;
    for (const char *p = row; *p; ++p) commas += *p == ',';
    assert(commas == 110);
    assert(USBTelemetryFixed(12.34f, 100) == 1234);
    assert(USBTelemetryFixed(-12.345f, 100) == -1235);
    assert(USBTelemetryFixed(1.25f, 1000) == 1250);
    assert(USBTelemetryFixed(INFINITY, 100) == 0);
    assert(USBTelemetryFixed(NAN, 100) == 0);
    assert(USBTelemetryFixed(1e30f, 100) == INT32_MAX);
    assert(USBTelemetryFixed(-1e30f, 100) == INT32_MIN);
    s.tick = UINT32_MAX; s.rc_age_ms = UINT32_MAX; s.vx_cmd_milli = INT32_MIN;
    assert(USBTelemetryFormatRow(&s, row, sizeof row));
    assert(strstr(row, "4294967295") != NULL);
    assert(strstr(row, "-2147483648") != NULL);
#define TEST_MAX_U(name) s.name = UINT32_MAX;
#define TEST_MAX_I(name) s.name = INT32_MIN;
    USB_TELEMETRY_FIELDS(TEST_MAX_U, TEST_MAX_I)
#undef TEST_MAX_U
#undef TEST_MAX_I
    assert(USBTelemetryFormatRow(&s, row, sizeof row) > 0);
    char short_row[8] = "before";
    assert(USBTelemetryFormatRow(&s, short_row, sizeof short_row) == 0);
    assert(short_row[0] == '\0');
    assert(USBTelemetryFormatHeader(short_row, sizeof short_row) == 0);
    assert(short_row[0] == '\0');
    assert(USBTelemetryFormatRow(NULL, row, sizeof row) == 0 && row[0] == '\0');
    assert(USBTelemetryFormatHeader(NULL, 0) == 0);
    s = fixture();
    USBTelemetryApplyFreshness(&s, 1000);
    assert(s.chassis_valid && s.power_valid && s.motor_online_mask == 15);
    assert(s.velocity_actual_valid && s.velocity_target_valid);
    USBTelemetryApplyFreshness(&s,1016);
    assert(!s.velocity_actual_valid && s.velocity_target_valid);
    USBTelemetryApplyFreshness(&s,1021);assert(!s.velocity_target_valid);
    const ChassisVelocityFeedback_s invalid={.actual={.valid=true,.wz_rps=NAN}};
    USBTelemetryEncodeVelocity(&s,&invalid,0);assert(!s.velocity_actual_valid);
    s=fixture();
    s.imu_age_ms = 10; USBTelemetryApplyFreshness(&s, 1000); assert(s.imu_valid);
    s.imu_age_ms = 11; USBTelemetryApplyFreshness(&s, 1000); assert(!s.imu_valid);
    s.imu_valid = 1; s.imu_age_ms = UINT32_MAX;
    USBTelemetryApplyFreshness(&s, 1000); assert(!s.imu_valid);
    s.rc_age_ms = 101; s.meter_age_ms = 21; s.buffer_age_ms = 301;
    s.cap_age_ms = 21; s.cap_input_age_ms = UINT32_MAX;
    USBTelemetryApplyFreshness(&s, 1200);
    assert(!s.chassis_valid && !s.power_valid && !s.motor_online_mask);
    assert(!s.motor_enabled_mask && !s.rc_online && !s.meter_online);
    assert(!s.referee_online && !s.buffer_estimate_valid && !s.cap_online);
    assert(!s.cap_output_enabled && !s.cap_input_online);
    s = fixture(); s.chassis_tick_ms = UINT32_MAX - 2; s.model_tick_ms = UINT32_MAX;
    USBTelemetryApplyFreshness(&s, 3);
    assert(s.chassis_valid && s.power_valid); /* Normal HAL tick wrap is fresh. */
    INSSnapshot_s imu = {.tick_ms = 995, .valid = false, .has_sample = true,
        .attitude = {.Yaw = -12.345f, .Gyro = {-180.5f, 0, 0}, .Accel = {0, 0, 9.807f}}};
    USBTelemetryEncodeIMU(&s, &imu, 1010);
    assert(!s.imu_valid && s.imu_age_ms == 15);
    assert(s.imu_yaw_mdeg == -12345 && s.imu_gx_mdps == -180500 && s.imu_az_mmps2 == 9807);
    imu.valid = true;
    USBTelemetryEncodeIMU(&s, &imu, 1000);
    assert(s.imu_valid && s.imu_age_ms == 5);
    imu.attitude.Accel[0] = NAN;
    USBTelemetryEncodeIMU(&s, &imu, 1000);
    assert(!s.imu_valid);
    imu = (INSSnapshot_s){0};
    USBTelemetryEncodeIMU(&s, &imu, 1000);
    assert(!s.imu_valid && s.imu_age_ms == UINT32_MAX);
    imu.has_sample = true; imu.valid = true; imu.tick_ms = UINT32_MAX - 2;
    USBTelemetryEncodeIMU(&s, &imu, 3);
    assert(s.imu_valid && s.imu_age_ms == 6);
    imu.tick_ms = 0; imu.valid = false;
    USBTelemetryEncodeIMU(&s, &imu, 15);
    assert(!s.imu_valid && s.imu_age_ms == 15);
    puts("USB telemetry format tests passed");
    return 0;
}
