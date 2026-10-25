#ifndef CHASSIS_FORCE_CONFIG_H
#define CHASSIS_FORCE_CONFIG_H
#include "chassis_control_config.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float x_m, y_m, dx, dy; /* Relative to measured center of mass, final CAN signs. */
    float radius_m, reduction, k_out_nm_a;
    float current_limit, force_limit_n, current_slew;
} ChassisForceWheel_s;

typedef struct {
    uint32_t backend, schema_version, checksum;
    bool calibration_valid;
    ChassisForceWheel_s wheel[4];
    float force_max_n, spin_torque_nm, spin_start_torque_nm, spin_torque_max_nm;
    float force_rise_n_s, force_fall_n_s, torque_rise_nm_s, torque_fall_nm_s;
    float velocity_limit_mps, omega_limit_rad_s, wheel_limit_rpm, resume_ratio;
    float input_deadzone, input_gamma;
    float brake_torque_nm, brake_stop_rad_s;
    uint32_t brake_timeout_ms;
    float power_gain_nm_per_w_s, power_deadband_w, power_lpf_tau_s;
    uint32_t power_recovery_samples;
    float torque_probe_step_nm, power_request_step_w;
    uint32_t grant_timeout_ms;
    float stall_current_command, stall_rpm;
    uint32_t stall_timeout_ms;
    float temp_derate_c, temp_stop_c;
    float speed_prediction_rpm, startup_current_limit;
} ChassisForceConfig_s;

/* Physical parameters deliberately have no enabling defaults. */
ChassisForceConfig_s ChassisForceDefaultConfig(void);
uint32_t ChassisForceConfigChecksum(const ChassisForceConfig_s *config);
bool ChassisForceConfigValid(const ChassisForceConfig_s *config);
#endif
