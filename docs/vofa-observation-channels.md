# VOFA 专用观测通道

由 `scripts/generate-vofa-schema.py` 从工程的 JSON 通道定义生成。
32 位字段以 `_hi` / `_lo` 拼接；未知年龄为 -1。有效位未置位的数据不可作为测量使用。

## Schema 4: power

| 通道 | 字段 | 单位 |
| --- | --- | --- |
| I0 | `schema` | 1 |
| I1 | `tick_ms_hi` | ms |
| I2 | `tick_ms_lo` | ms |
| I3 | `tx_sequence_hi` | 1 |
| I4 | `tx_sequence_lo` | 1 |
| I5 | `valid` | 1 |
| I6 | `state` | 1 |
| I7 | `wheel_masks` | 1 |
| I8 | `cycle_age_ms` | ms |
| I9 | `final_age_ms` | ms |
| I10 | `control_age_ms` | ms |
| I11 | `control_version_hi` | 1 |
| I12 | `control_version_lo` | 1 |
| I13 | `config_version_hi` | 1 |
| I14 | `config_version_lo` | 1 |
| I15 | `provenance` | 1 |
| I16 | `config_fingerprint_hi` | 1 |
| I17 | `config_fingerprint_lo` | 1 |
| I18 | `policy_age_ms` | ms |
| I19 | `policy_version_hi` | 1 |
| I20 | `policy_version_lo` | 1 |
| I21 | `policy_config_version_hi` | 1 |
| I22 | `policy_config_version_lo` | 1 |
| I23 | `policy_config_fingerprint_hi` | 1 |
| I24 | `policy_config_fingerprint_lo` | 1 |
| I25 | `target_rpm_lf` | rotor rpm |
| I26 | `target_rpm_rf` | rotor rpm |
| I27 | `target_rpm_lb` | rotor rpm |
| I28 | `target_rpm_rb` | rotor rpm |
| I29 | `actual_rpm_lf` | rotor rpm |
| I30 | `actual_rpm_rf` | rotor rpm |
| I31 | `actual_rpm_lb` | rotor rpm |
| I32 | `actual_rpm_rb` | rotor rpm |
| I33 | `raw_current_lf` | DJI count |
| I34 | `raw_current_rf` | DJI count |
| I35 | `raw_current_lb` | DJI count |
| I36 | `raw_current_rb` | DJI count |
| I37 | `final_current_lf` | DJI count |
| I38 | `final_current_rf` | DJI count |
| I39 | `final_current_lb` | DJI count |
| I40 | `final_current_rb` | DJI count |
| I41 | `feedback_current_lf` | DJI count |
| I42 | `feedback_current_rf` | DJI count |
| I43 | `feedback_current_lb` | DJI count |
| I44 | `feedback_current_rb` | DJI count |
| I45 | `wheel_age_ms_lf` | ms |
| I46 | `wheel_age_ms_rf` | ms |
| I47 | `wheel_age_ms_lb` | ms |
| I48 | `wheel_age_ms_rb` | ms |
| I49 | `meter_power_w` | W |
| I50 | `meter_sequence_hi` | 1 |
| I51 | `meter_sequence_lo` | 1 |
| I52 | `meter_age_ms` | ms |
| I53 | `model_unlimited_w` | W |
| I54 | `model_final_w` | W |
| I55 | `nominal_budget_w` | W |
| I56 | `target_power_w` | W |
| I57 | `power_ceiling_w` | W |
| I58 | `slow_requested_w` | W |
| I59 | `slow_applied_w` | W |
| I60 | `fast_correction_w` | W |
| I61 | `effective_limit_w` | W |
| I62 | `filtered_power_w` | W |
| I63 | `guard_allowed_w` | W |
| I64 | `guard_power_w` | W |
| I65 | `guard_candidate_w` | W |
| I66 | `guard_scale` | 1 |
| I67 | `excess_energy_j` | J |
| I68 | `startup_allowance_j` | J |
| I69 | `policy_nominal_budget_w` | W |
| I70 | `policy_target_power_w` | W |
| I71 | `policy_slow_correction_w` | W |
| I72 | `policy_filtered_power_w` | W |
| I73 | `policy_power_ceiling_w` | W |
| I74 | `cap_output_power_w` | W |
| I75 | `cap_input_power_w` | W |
| I76 | `cap_energy_ratio` | 1 |
| I77 | `cap_available_w` | W |
| I78 | `cap_error` | 1 |
| I79 | `cap_age_ms` | ms |
| I80 | `cap_input_age_ms` | ms |
| I81 | `cap_command_target_w` | W |
| I82 | `cap_echo_target_w` | W |
| I83 | `cap_command_age_ms` | ms |
| I84 | `cap_failures_hi` | 1 |
| I85 | `cap_failures_lo` | 1 |
| I86 | `referee_limit_w` | W |
| I87 | `referee_buffer_j` | J |
| I88 | `referee_robot_age_ms` | ms |
| I89 | `referee_buffer_age_ms` | ms |
| I90 | `buffer_lower_j` | J |
| I91 | `buffer_predicted_j` | J |
| I92 | `buffer_recovery_w` | W |
| I93 | `outer_kp` | 1 |
| I94 | `outer_ki` | 1/s |
| I95 | `outer_tau_s` | s |

有效位（I5）：

- bit 0: `cycle`
- bit 1: `final`
- bit 2: `can_submitted`
- bit 3: `meter`
- bit 4: `policy`
- bit 5: `cap_status`
- bit 6: `cap_input`
- bit 7: `referee`
- bit 8: `buffer`
- bit 9: `config`
- bit 10: `cap_command`
- bit 11: `cap_submitted`
- bit 12: `cap_output_enabled`
- bit 13: `cap_input_active`
- bit 14: `control_enabled`
- bit 15: `cap_input_command_enabled`

`state_encoding`: transient bits0..2, outer bits3..4, budget bits5..6, recovering bit7, power_source bits8..9

`provenance_encoding`: cycle source bits0..1, policy source bits2..3, submitted_groups bits4..9, successful_groups bits10..15

`wheel_masks_encoding`: online bits0..3, enabled bits4..7, drive bits8..11, brake bits12..15

## Schema 5: kalman

| 通道 | 字段 | 单位 |
| --- | --- | --- |
| I0 | `schema` | 1 |
| I1 | `tick_ms_hi` | ms |
| I2 | `tick_ms_lo` | ms |
| I3 | `tx_sequence_hi` | 1 |
| I4 | `tx_sequence_lo` | 1 |
| I5 | `valid` | 1 |
| I6 | `observer_status` | 1 |
| I7 | `config_source` | 1 |
| I8 | `config_fingerprint_hi` | 1 |
| I9 | `config_fingerprint_lo` | 1 |
| I10 | `imu_source` | 1 |
| I11 | `wheel_online_mask` | 1 |
| I12 | `raw_rpm_lf` | rotor rpm |
| I13 | `raw_rpm_rf` | rotor rpm |
| I14 | `raw_rpm_lb` | rotor rpm |
| I15 | `raw_rpm_rb` | rotor rpm |
| I16 | `wheel_age_ms_lf` | ms |
| I17 | `wheel_age_ms_rf` | ms |
| I18 | `wheel_age_ms_lb` | ms |
| I19 | `wheel_age_ms_rb` | ms |
| I20 | `wheel_vx_mps` | m/s |
| I21 | `wheel_vy_mps` | m/s |
| I22 | `wheel_wz_radps` | rad/s |
| I23 | `target_vx_mps` | m/s |
| I24 | `target_vy_mps` | m/s |
| I25 | `target_wz_radps` | rad/s |
| I26 | `velocity_vx_mps` | m/s |
| I27 | `velocity_vy_mps` | m/s |
| I28 | `velocity_age_ms` | ms |
| I29 | `imu_qw` | 1 |
| I30 | `imu_qx` | 1 |
| I31 | `imu_qy` | 1 |
| I32 | `imu_qz` | 1 |
| I33 | `imu_ax_mps2` | m/s^2 |
| I34 | `imu_ay_mps2` | m/s^2 |
| I35 | `imu_az_mps2` | m/s^2 |
| I36 | `imu_gx_radps` | rad/s |
| I37 | `imu_gy_radps` | rad/s |
| I38 | `imu_gz_radps` | rad/s |
| I39 | `imu_age_ms` | ms |
| I40 | `imu_receive_age_ms` | ms |
| I41 | `imu_source_age_ms` | ms |
| I42 | `imu_sequence_hi` | 1 |
| I43 | `imu_sequence_lo` | 1 |
| I44 | `imu_tick_ms_hi` | ms |
| I45 | `imu_tick_ms_lo` | ms |
| I46 | `attempt_sequence_hi` | 1 |
| I47 | `attempt_sequence_lo` | 1 |
| I48 | `attempt_flags` | 1 |
| I49 | `call_receiver_ms_hi` | ms |
| I50 | `call_receiver_ms_lo` | ms |
| I51 | `call_us_hi` | us |
| I52 | `call_us_lo` | us |
| I53 | `wheel_us_hi` | us |
| I54 | `wheel_us_lo` | us |
| I55 | `imu_us_hi` | us |
| I56 | `imu_us_lo` | us |
| I57 | `middle_receiver_ms_hi` | ms |
| I58 | `middle_receiver_ms_lo` | ms |
| I59 | `wheel_n_x_mps` | m/s |
| I60 | `wheel_n_y_mps` | m/s |
| I61 | `accel_n_x_mps2` | m/s^2 |
| I62 | `accel_n_y_mps2` | m/s^2 |
| I63 | `output_n_x_mps` | m/s |
| I64 | `output_n_y_mps` | m/s |
| I65 | `output_sample_us_hi` | us |
| I66 | `output_sample_us_lo` | us |
| I67 | `update_count_hi` | 1 |
| I68 | `update_count_lo` | 1 |
| I69 | `reset_count_hi` | 1 |
| I70 | `reset_count_lo` | 1 |
| I71 | `resets_before_hi` | 1 |
| I72 | `resets_before_lo` | 1 |
| I73 | `pair_span_ms` | ms |
| I74 | `source_step_ms` | ms |
| I75 | `receive_step_ms` | ms |
| I76 | `fusion_wheel_variance` | (m/s)^2 |
| I77 | `fusion_accel_variance` | (m/s^2)^2 |
| I78 | `fusion_jerk_density` | m^2/s^5 |
| I79 | `fusion_max_residual_mps` | m/s |
| I80 | `fusion_max_age_us_hi` | us |
| I81 | `fusion_max_age_us_lo` | us |
| I82 | `fusion_max_skew_us_hi` | us |
| I83 | `fusion_max_skew_us_lo` | us |
| I84 | `fusion_max_gap_us_hi` | us |
| I85 | `fusion_max_gap_us_lo` | us |
| I86 | `mount_qw` | 1 |
| I87 | `mount_qx` | 1 |
| I88 | `mount_qy` | 1 |
| I89 | `mount_qz` | 1 |
| I90 | `offset_x_m` | m |
| I91 | `offset_y_m` | m |
| I92 | `offset_z_m` | m |
| I93 | `max_pair_span_ms` | ms |
| I94 | `max_wheel_skew_ms` | ms |
| I95 | `max_timing_jitter_ms` | ms |

有效位（I5）：

- bit 0: `wheels`
- bit 1: `target`
- bit 2: `estimate`
- bit 3: `imu`
- bit 4: `attempt`
- bit 5: `verified`
- bit 6: `capture_fifo`
- bit 7: `capture_overflow`

`attempt_flags`：

- bit 0: `available`
- bit 1: `wheel_valid`
- bit 2: `imu_valid`
- bit 3: `initialized_before`
- bit 4: `output_valid`
- bit 5: `output_fused`

## Schema 6: kalman_gimbal

| 通道 | 字段 | 单位 |
| --- | --- | --- |
| I0 | `schema` | 1 |
| I1 | `tick_ms_hi` | ms |
| I2 | `tick_ms_lo` | ms |
| I3 | `tx_sequence_hi` | 1 |
| I4 | `tx_sequence_lo` | 1 |
| I5 | `valid` | 1 |
| I6 | `observer_status` | 1 |
| I7 | `config_source` | 1 |
| I8 | `config_fingerprint_hi` | 1 |
| I9 | `config_fingerprint_lo` | 1 |
| I10 | `imu_source` | 1 |
| I11 | `wheel_online_mask` | 1 |
| I12 | `raw_rpm_lf` | rotor rpm |
| I13 | `raw_rpm_rf` | rotor rpm |
| I14 | `raw_rpm_lb` | rotor rpm |
| I15 | `raw_rpm_rb` | rotor rpm |
| I16 | `wheel_age_ms_lf` | ms |
| I17 | `wheel_age_ms_rf` | ms |
| I18 | `wheel_age_ms_lb` | ms |
| I19 | `wheel_age_ms_rb` | ms |
| I20 | `wheel_vx_mps` | m/s |
| I21 | `wheel_vy_mps` | m/s |
| I22 | `wheel_wz_radps` | rad/s |
| I23 | `target_vx_mps` | m/s |
| I24 | `target_vy_mps` | m/s |
| I25 | `target_wz_radps` | rad/s |
| I26 | `velocity_vx_mps` | m/s |
| I27 | `velocity_vy_mps` | m/s |
| I28 | `velocity_age_ms` | ms |
| I29 | `imu_qw` | 1 |
| I30 | `imu_qx` | 1 |
| I31 | `imu_qy` | 1 |
| I32 | `imu_qz` | 1 |
| I33 | `imu_ax_mps2` | m/s^2 |
| I34 | `imu_ay_mps2` | m/s^2 |
| I35 | `imu_az_mps2` | m/s^2 |
| I36 | `imu_gx_radps` | rad/s |
| I37 | `imu_gy_radps` | rad/s |
| I38 | `imu_gz_radps` | rad/s |
| I39 | `imu_age_ms` | ms |
| I40 | `imu_receive_age_ms` | ms |
| I41 | `imu_source_age_ms` | ms |
| I42 | `imu_sequence_hi` | 1 |
| I43 | `imu_sequence_lo` | 1 |
| I44 | `imu_tick_ms_hi` | ms |
| I45 | `imu_tick_ms_lo` | ms |
| I46 | `attempt_sequence_hi` | 1 |
| I47 | `attempt_sequence_lo` | 1 |
| I48 | `attempt_flags` | 1 |
| I49 | `call_receiver_ms_hi` | ms |
| I50 | `call_receiver_ms_lo` | ms |
| I51 | `call_us_hi` | us |
| I52 | `call_us_lo` | us |
| I53 | `wheel_us_hi` | us |
| I54 | `wheel_us_lo` | us |
| I55 | `imu_us_hi` | us |
| I56 | `imu_us_lo` | us |
| I57 | `middle_receiver_ms_hi` | ms |
| I58 | `middle_receiver_ms_lo` | ms |
| I59 | `wheel_n_x_mps` | m/s |
| I60 | `wheel_n_y_mps` | m/s |
| I61 | `accel_n_x_mps2` | m/s^2 |
| I62 | `accel_n_y_mps2` | m/s^2 |
| I63 | `output_n_x_mps` | m/s |
| I64 | `output_n_y_mps` | m/s |
| I65 | `output_sample_us_hi` | us |
| I66 | `output_sample_us_lo` | us |
| I67 | `update_count_hi` | 1 |
| I68 | `update_count_lo` | 1 |
| I69 | `reset_count_hi` | 1 |
| I70 | `reset_count_lo` | 1 |
| I71 | `resets_before_hi` | 1 |
| I72 | `resets_before_lo` | 1 |
| I73 | `pair_span_ms` | ms |
| I74 | `source_step_ms` | ms |
| I75 | `receive_step_ms` | ms |
| I76 | `fusion_wheel_variance` | (m/s)^2 |
| I77 | `fusion_accel_variance` | (m/s^2)^2 |
| I78 | `fusion_jerk_density` | m^2/s^5 |
| I79 | `fusion_max_residual_mps` | m/s |
| I80 | `fusion_max_age_us_hi` | us |
| I81 | `fusion_max_age_us_lo` | us |
| I82 | `fusion_max_skew_us_hi` | us |
| I83 | `fusion_max_skew_us_lo` | us |
| I84 | `fusion_max_gap_us_hi` | us |
| I85 | `fusion_max_gap_us_lo` | us |
| I86 | `mount_qw` | 1 |
| I87 | `mount_qx` | 1 |
| I88 | `mount_qy` | 1 |
| I89 | `mount_qz` | 1 |
| I90 | `offset_x_m` | m |
| I91 | `offset_y_m` | m |
| I92 | `offset_z_m` | m |
| I93 | `max_pair_span_ms` | ms |
| I94 | `max_wheel_skew_ms` | ms |
| I95 | `max_timing_jitter_ms` | ms |
| I96 | `yaw_ecd` | encoder count |
| I97 | `yaw_offset_deg` | deg |
| I98 | `yaw_age_ms` | ms |
| I99 | `chassis_mode` | 1 |
| I100 | `gimbal_yaw_total_deg` | deg |
| I101 | `gimbal_imu_age_ms` | ms |

有效位（I5）：

- bit 0: `wheels`
- bit 1: `target`
- bit 2: `estimate`
- bit 3: `imu`
- bit 4: `attempt`
- bit 5: `verified`
- bit 6: `capture_fifo`
- bit 7: `capture_overflow`
- bit 8: `yaw_encoder`
- bit 9: `gimbal_imu`

`attempt_flags`：

- bit 0: `available`
- bit 1: `wheel_valid`
- bit 2: `imu_valid`
- bit 3: `initialized_before`
- bit 4: `output_valid`
- bit 5: `output_fused`

## Schema 7: kalman_gimbal_control

| 通道 | 字段 | 单位 |
| --- | --- | --- |
| I0 | `schema` | 1 |
| I1 | `tick_ms_hi` | ms |
| I2 | `tick_ms_lo` | ms |
| I3 | `tx_sequence_hi` | 1 |
| I4 | `tx_sequence_lo` | 1 |
| I5 | `valid` | 1 |
| I6 | `observer_status` | 1 |
| I7 | `config_source` | 1 |
| I8 | `config_fingerprint_hi` | 1 |
| I9 | `config_fingerprint_lo` | 1 |
| I10 | `imu_source` | 1 |
| I11 | `wheel_online_mask` | 1 |
| I12 | `raw_rpm_lf` | rotor rpm |
| I13 | `raw_rpm_rf` | rotor rpm |
| I14 | `raw_rpm_lb` | rotor rpm |
| I15 | `raw_rpm_rb` | rotor rpm |
| I16 | `wheel_age_ms_lf` | ms |
| I17 | `wheel_age_ms_rf` | ms |
| I18 | `wheel_age_ms_lb` | ms |
| I19 | `wheel_age_ms_rb` | ms |
| I20 | `wheel_vx_mps` | m/s |
| I21 | `wheel_vy_mps` | m/s |
| I22 | `wheel_wz_radps` | rad/s |
| I23 | `target_vx_mps` | m/s |
| I24 | `target_vy_mps` | m/s |
| I25 | `target_wz_radps` | rad/s |
| I26 | `velocity_vx_mps` | m/s |
| I27 | `velocity_vy_mps` | m/s |
| I28 | `velocity_age_ms` | ms |
| I29 | `imu_qw` | 1 |
| I30 | `imu_qx` | 1 |
| I31 | `imu_qy` | 1 |
| I32 | `imu_qz` | 1 |
| I33 | `imu_ax_mps2` | m/s^2 |
| I34 | `imu_ay_mps2` | m/s^2 |
| I35 | `imu_az_mps2` | m/s^2 |
| I36 | `imu_gx_radps` | rad/s |
| I37 | `imu_gy_radps` | rad/s |
| I38 | `imu_gz_radps` | rad/s |
| I39 | `imu_age_ms` | ms |
| I40 | `imu_receive_age_ms` | ms |
| I41 | `imu_source_age_ms` | ms |
| I42 | `imu_sequence_hi` | 1 |
| I43 | `imu_sequence_lo` | 1 |
| I44 | `imu_tick_ms_hi` | ms |
| I45 | `imu_tick_ms_lo` | ms |
| I46 | `attempt_sequence_hi` | 1 |
| I47 | `attempt_sequence_lo` | 1 |
| I48 | `attempt_flags` | 1 |
| I49 | `call_receiver_ms_hi` | ms |
| I50 | `call_receiver_ms_lo` | ms |
| I51 | `call_us_hi` | us |
| I52 | `call_us_lo` | us |
| I53 | `wheel_us_hi` | us |
| I54 | `wheel_us_lo` | us |
| I55 | `imu_us_hi` | us |
| I56 | `imu_us_lo` | us |
| I57 | `middle_receiver_ms_hi` | ms |
| I58 | `middle_receiver_ms_lo` | ms |
| I59 | `wheel_n_x_mps` | m/s |
| I60 | `wheel_n_y_mps` | m/s |
| I61 | `accel_n_x_mps2` | m/s^2 |
| I62 | `accel_n_y_mps2` | m/s^2 |
| I63 | `output_n_x_mps` | m/s |
| I64 | `output_n_y_mps` | m/s |
| I65 | `output_sample_us_hi` | us |
| I66 | `output_sample_us_lo` | us |
| I67 | `update_count_hi` | 1 |
| I68 | `update_count_lo` | 1 |
| I69 | `reset_count_hi` | 1 |
| I70 | `reset_count_lo` | 1 |
| I71 | `resets_before_hi` | 1 |
| I72 | `resets_before_lo` | 1 |
| I73 | `pair_span_ms` | ms |
| I74 | `source_step_ms` | ms |
| I75 | `receive_step_ms` | ms |
| I76 | `fusion_wheel_variance` | (m/s)^2 |
| I77 | `fusion_accel_variance` | (m/s^2)^2 |
| I78 | `fusion_jerk_density` | m^2/s^5 |
| I79 | `fusion_max_residual_mps` | m/s |
| I80 | `fusion_max_age_us_hi` | us |
| I81 | `fusion_max_age_us_lo` | us |
| I82 | `fusion_max_skew_us_hi` | us |
| I83 | `fusion_max_skew_us_lo` | us |
| I84 | `fusion_max_gap_us_hi` | us |
| I85 | `fusion_max_gap_us_lo` | us |
| I86 | `mount_qw` | 1 |
| I87 | `mount_qx` | 1 |
| I88 | `mount_qy` | 1 |
| I89 | `mount_qz` | 1 |
| I90 | `offset_x_m` | m |
| I91 | `offset_y_m` | m |
| I92 | `offset_z_m` | m |
| I93 | `max_pair_span_ms` | ms |
| I94 | `max_wheel_skew_ms` | ms |
| I95 | `max_timing_jitter_ms` | ms |
| I96 | `yaw_ecd` | encoder count |
| I97 | `yaw_offset_deg` | deg |
| I98 | `yaw_age_ms` | ms |
| I99 | `chassis_mode` | 1 |
| I100 | `gimbal_yaw_total_deg` | deg |
| I101 | `gimbal_imu_age_ms` | ms |
| I102 | `yaw_ref_deg` | deg |
| I103 | `yaw_speed_dps` | deg/s |
| I104 | `yaw_speed_iout` | DJI command |
| I105 | `yaw_command` | DJI command |
| I106 | `yaw_control_flags` | bitmask |
| I107 | `yaw_control_age_ms` | ms |

有效位（I5）：

- bit 0: `wheels`
- bit 1: `target`
- bit 2: `estimate`
- bit 3: `imu`
- bit 4: `attempt`
- bit 5: `verified`
- bit 6: `capture_fifo`
- bit 7: `capture_overflow`
- bit 8: `yaw_encoder`
- bit 9: `gimbal_imu`
- bit 10: `yaw_control`

`attempt_flags`：

- bit 0: `available`
- bit 1: `wheel_valid`
- bit 2: `imu_valid`
- bit 3: `initialized_before`
- bit 4: `output_valid`
- bit 5: `output_fused`

`yaw_control_flags`：

- bit 0: `enabled_at_submit`
- bit 1: `can_attempted`
- bit 2: `can_mailbox_accepted`
- bit 3: `speed_integral_at_limit`
- bit 4: `speed_output_at_limit`
- bit 5: `angle_output_at_limit`
- bit 6: `speed_pid_calculated`
- bit 7: `output_feedforward_applied`
- bit 8: `combined_output_at_limit`
