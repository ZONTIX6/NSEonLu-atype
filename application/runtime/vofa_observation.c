#include "vofa_observation.h"
#include "vofa_schema.h"
#include "imu_capture_config.h"

#if VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0
#include "usart.h"
#include <limits.h>
#include <math.h>
#include <string.h>
#include "../../config/30_velocity_observation/chassis_remote_imu_config.h"
#if CHASSIS_IMU_CAN_RX_ENABLE
#include "chassis_imu_can.h"
#if VOFA_OBSERVATION_PROFILE == 2
#include "FreeRTOS.h"
#include "task.h"
#endif
#endif

static struct {
    float channels[96];
    uint8_t tail[4];
} frame = {.tail = {0, 0, 128, 127}};
_Static_assert(sizeof(float) == 4 && sizeof(frame) == 388, "Observation frame layout");
_Static_assert(VOFA_POWER_CHANNELS == 96 && VOFA_KALMAN_CHANNELS == 96, "Channel contract");
_Static_assert(sizeof(frame) * 10U * 1000U < BOARD_UART7_BAUDRATE * 20U, "Observation wire budget");

static float Age(uint32_t now, uint32_t tick, bool known)
{
    uint32_t age = now - tick;
    return known && age <= INT32_MAX ? (float)age : -1.0f;
}

static void U32(float *f, unsigned index, uint32_t value)
{
    f[index] = (float)(value >> 16);
    f[index + 1] = (float)(value & UINT16_MAX);
}

static bool Finite(const float *values, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) if (!isfinite(values[i])) return false;
    return true;
}

#if VOFA_OBSERVATION_PROFILE == 1
#define P(name) f[VOFA_POWER_##name]
static void Power(float *f, const USBTelemetrySample_s *sample,
    const ChassisPowerSnapshot_s *p, const SuperCapSnapshot_s *cap,
    const RefereePowerSnapshot_s *r, uint32_t now)
{
    static const ChassisPowerSnapshot_s empty_power;
    static const SuperCapSnapshot_s empty_cap;
    static const RefereePowerSnapshot_s empty_referee;
    if (!p) p = &empty_power;
    if (!cap) cap = &empty_cap;
    if (!r) r = &empty_referee;
    const ChassisPowerControlSnapshot_s *c = &p->cycle_control;
    const ChassisPowerTransientOutput_s *t = &p->transient;
    const ChassisPowerPolicyOutput_s *policy = &p->policy;
    const bool known = p->registered && p->final_valid;
    const bool final = known && (uint32_t)(now - p->final_tick_ms) <= 20U;
    const bool cycle = final && p->cycle_valid && !p->numeric_error &&
        (uint32_t)(now - p->tick_ms) <= 20U && Finite(p->raw_current, 4) &&
        Finite(p->feedback_current, 4) && Finite(p->telemetry.actual_speed_rpm, 4) &&
        Finite(p->telemetry.target_speed_rpm, 4) &&
        isfinite(c->nominal_budget_w) && isfinite(c->target_power_w) &&
        isfinite(c->slow_correction_w) && isfinite(c->power_ceiling_w) &&
        isfinite(c->telemetry.filtered_power_w) &&
        Finite(p->power.predicted_unlimited_power_w, 4) &&
        isfinite(p->telemetry.predicted_total_power_w) && isfinite(p->power.effective_limit_w) &&
        isfinite(t->fast_w) && isfinite(t->slow_applied_w) && isfinite(t->guard_power_w) &&
        isfinite(t->allowed_guard_w) && isfinite(t->candidate_guard_w) && isfinite(t->guard_scale) &&
        isfinite(t->excess_j) && isfinite(t->allowance_j);
    const bool meter_known = known && (p->meter_online || p->meter_sequence || p->meter_tick_ms);
    const bool meter = final && p->meter_online && isfinite(p->measured_power_w) &&
        (uint32_t)(now - p->meter_tick_ms) <= 20U;
    const bool policy_valid = p->registered && policy->valid &&
        (uint32_t)(now - p->policy_tick_ms) <= 20U &&
        isfinite(policy->control.nominal_budget_w) && isfinite(policy->control.target_power_w) &&
        isfinite(policy->control.slow_correction_w) && isfinite(policy->control.power_ceiling_w) &&
        isfinite(policy->outer.filtered_power_w);
    const bool cap_status = cap->status.online && (uint32_t)(now - cap->status.last_rx_tick_ms) <= 100U &&
        isfinite(cap->status.energy_ratio) && isfinite(cap->status.chassis_power_w);
    const bool cap_input = cap->input_feedback_enabled && cap->input_power.online &&
        (uint32_t)(now - cap->input_power.last_rx_tick_ms) <= 100U && isfinite(cap->input_power.input_power_w);
    const bool buffer = policy_valid && policy->buffer_estimate_valid &&
        isfinite(policy->budget.buffer_lower_j) && isfinite(policy->budget.buffer_predicted_j) &&
        isfinite(policy->budget.recovery_reduction_w);
    const bool config = known && p->cycle_config.available && isfinite(p->cycle_config.outer.kp) &&
        isfinite(p->cycle_config.outer.ki) && isfinite(p->cycle_config.outer.measurement_lpf_tau_s);
    P(SCHEMA) = VOFA_POWER_SCHEMA_ID;
    P(VALID) = (cycle ? 1U : 0U) | (final ? 2U : 0U) | (final && p->can_submitted ? 4U : 0U) |
        (meter ? 8U : 0U) | (policy_valid ? 16U : 0U) | (cap_status ? 32U : 0U) |
        (cap_input ? 64U : 0U) | (r->valid ? 128U : 0U) | (buffer ? 256U : 0U) |
        (config ? 512U : 0U) | (cap->command_attempted ? 1024U : 0U) |
        (cap->command_attempted && cap->command_submitted ? 2048U : 0U) |
        (cap_status && cap->status.output_enabled ? 4096U : 0U) |
        (cap_input && cap->input_power.input_control_active ? 8192U : 0U) |
        (cycle && c->control_enabled ? 16384U : 0U) |
        (cap->command_attempted && cap->last_command.input_control_enabled ? 32768U : 0U);
    P(STATE) = ((unsigned)t->mode & 7U) | ((c->telemetry.outer_mode & 3U) << 3) |
        (((unsigned)policy->budget.mode & 3U) << 5) | (policy->budget.recovering ? 128U : 0U) |
        ((sample ? sample->power_source & 3U : 0U) << 8);
    uint32_t online = p->online_mask & 15U;
    for (unsigned i = 0; i < 4; ++i) {
        if (!known || (uint32_t)(now - p->feedback_tick_ms[i]) > 20U ||
            !isfinite(p->telemetry.actual_speed_rpm[i]) || !isfinite(p->feedback_current[i])) online &= ~(1U << i);
        f[VOFA_POWER_TARGET_RPM_LF + i] = p->telemetry.target_speed_rpm[i];
        f[VOFA_POWER_ACTUAL_RPM_LF + i] = p->telemetry.actual_speed_rpm[i];
        f[VOFA_POWER_RAW_CURRENT_LF + i] = p->raw_current[i];
        f[VOFA_POWER_FINAL_CURRENT_LF + i] = p->final_current[i];
        f[VOFA_POWER_FEEDBACK_CURRENT_LF + i] = p->feedback_current[i];
        f[VOFA_POWER_WHEEL_AGE_MS_LF + i] = Age(now, p->feedback_tick_ms[i], known && (p->online_mask & (1U << i)));
    }
    P(WHEEL_MASKS) = online | ((p->telemetry.motor_enabled_mask & 15U) << 4) |
        ((t->drive_mask & 15U) << 8) | ((t->brake_mask & 15U) << 12);
    P(CYCLE_AGE_MS) = Age(now, p->tick_ms, known);
    P(FINAL_AGE_MS) = Age(now, p->final_tick_ms, known);
    P(CONTROL_AGE_MS) = Age(now, c->tick_ms, known);
    U32(f, VOFA_POWER_CONTROL_VERSION_HI, p->control_version);
    U32(f, VOFA_POWER_CONFIG_VERSION_HI, p->config_version);
    P(PROVENANCE) = (p->cycle_config.tuning_source & 3U) | ((policy->config.tuning_source & 3U) << 2) |
        ((p->submitted_groups & 63U) << 4) | ((p->successful_groups & 63U) << 10);
    U32(f, VOFA_POWER_CONFIG_FINGERPRINT_HI, p->cycle_config.fingerprint);
    P(POLICY_AGE_MS) = Age(now, p->policy_tick_ms, p->registered && policy->config.available);
    U32(f, VOFA_POWER_POLICY_VERSION_HI, policy->control.version);
    U32(f, VOFA_POWER_POLICY_CONFIG_VERSION_HI, policy->control.config_version);
    U32(f, VOFA_POWER_POLICY_CONFIG_FINGERPRINT_HI, policy->config.fingerprint);
    P(METER_POWER_W) = p->measured_power_w;
    U32(f, VOFA_POWER_METER_SEQUENCE_HI, p->meter_sequence);
    P(METER_AGE_MS) = Age(now, p->meter_tick_ms, meter_known);
    for (unsigned i = 0; i < 4; ++i) P(MODEL_UNLIMITED_W) += p->power.predicted_unlimited_power_w[i];
    P(MODEL_FINAL_W) = p->telemetry.predicted_total_power_w;
    P(NOMINAL_BUDGET_W) = c->nominal_budget_w;
    P(TARGET_POWER_W) = c->target_power_w;
    P(POWER_CEILING_W) = c->power_ceiling_w;
    P(SLOW_REQUESTED_W) = c->slow_correction_w;
    P(SLOW_APPLIED_W) = t->slow_applied_w;
    P(FAST_CORRECTION_W) = t->fast_w;
    P(EFFECTIVE_LIMIT_W) = p->power.effective_limit_w;
    P(FILTERED_POWER_W) = c->telemetry.filtered_power_w;
    P(GUARD_ALLOWED_W) = t->allowed_guard_w;
    P(GUARD_POWER_W) = t->guard_power_w;
    P(GUARD_CANDIDATE_W) = t->candidate_guard_w;
    P(GUARD_SCALE) = t->guard_scale;
    P(EXCESS_ENERGY_J) = t->excess_j;
    P(STARTUP_ALLOWANCE_J) = t->allowance_j;
    P(POLICY_NOMINAL_BUDGET_W) = policy->control.nominal_budget_w;
    P(POLICY_TARGET_POWER_W) = policy->control.target_power_w;
    P(POLICY_SLOW_CORRECTION_W) = policy->control.slow_correction_w;
    P(POLICY_FILTERED_POWER_W) = policy->outer.filtered_power_w;
    P(POLICY_POWER_CEILING_W) = policy->control.power_ceiling_w;
    P(CAP_OUTPUT_POWER_W) = cap->status.chassis_power_w;
    P(CAP_INPUT_POWER_W) = cap->input_power.input_power_w;
    P(CAP_ENERGY_RATIO) = cap->status.energy_ratio;
    P(CAP_AVAILABLE_W) = cap->status.available_power_limit_w;
    P(CAP_ERROR) = cap->status.error_code;
    P(CAP_AGE_MS) = Age(now, cap->status.last_rx_tick_ms, cap->received_frame_count != 0);
    P(CAP_INPUT_AGE_MS) = Age(now, cap->input_power.last_rx_tick_ms, cap->input_received_frame_count != 0);
    P(CAP_COMMAND_TARGET_W) = cap->last_command.input_power_target_w;
    P(CAP_ECHO_TARGET_W) = cap->input_power.input_power_target_w;
    P(CAP_COMMAND_AGE_MS) = Age(now, cap->command_attempt_tick_ms, cap->command_attempted);
    U32(f, VOFA_POWER_CAP_FAILURES_HI, cap->command_fail_count);
    P(REFEREE_LIMIT_W) = r->chassis_power_limit_w;
    P(REFEREE_BUFFER_J) = r->buffer_energy_j;
    P(REFEREE_ROBOT_AGE_MS) = Age(now, r->robot_state_tick_ms, r->has_robot_state);
    P(REFEREE_BUFFER_AGE_MS) = Age(now, r->buffer_tick_ms, r->has_power_heat);
    P(BUFFER_LOWER_J) = policy->budget.buffer_lower_j;
    P(BUFFER_PREDICTED_J) = policy->budget.buffer_predicted_j;
    P(BUFFER_RECOVERY_W) = policy->budget.recovery_reduction_w;
    P(OUTER_KP) = p->cycle_config.outer.kp;
    P(OUTER_KI) = p->cycle_config.outer.ki;
    P(OUTER_TAU_S) = p->cycle_config.outer.measurement_lpf_tau_s;
}
#undef P
#else
#define K(name) f[VOFA_KALMAN_##name]
static uint32_t HashWord(uint32_t hash, uint32_t word)
{
    for (unsigned i = 0; i < 4; ++i) { hash ^= (word >> (i * 8)) & 255U; hash *= 16777619U; }
    return hash;
}
static uint32_t ConfigFingerprint(const ChassisRemoteFusionConfig_s *c)
{
    uint32_t hash = 2166136261U, word;
    const float values[] = {c->sensor_to_body_quaternion[0], c->sensor_to_body_quaternion[1],
        c->sensor_to_body_quaternion[2], c->sensor_to_body_quaternion[3],
        c->imu_offset_body_m[0], c->imu_offset_body_m[1], c->imu_offset_body_m[2],
        c->filter.wheel_variance, c->filter.accel_variance, c->filter.jerk_density,
        c->filter.max_velocity_residual_mps};
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; ++i) {
        memcpy(&word, &values[i], sizeof word); hash = HashWord(hash, word);
    }
    hash = HashWord(hash, c->filter.max_age_us); hash = HashWord(hash, c->filter.max_skew_us);
    hash = HashWord(hash, c->filter.max_gap_us); hash = HashWord(hash, c->max_pair_span_ms);
    hash = HashWord(hash, c->max_wheel_skew_ms); return HashWord(hash, c->max_timing_jitter_ms);
}
static void Kalman(float *f, const USBTelemetrySample_s *sample,
    const ChassisFusedVelocitySnapshot_s *v, uint32_t now)
{
#if CHASSIS_IMU_CAN_RX_ENABLE
    static ChassisImuCanSnapshot_s remote;
    /* CAN may have published a newer group since the caller sampled its clock.
     * Match the observer's nested critical section; encode outside the lock. */
    taskENTER_CRITICAL();
    now = HAL_GetTick();
    const bool remote_usable = ChassisImuCanReadSnapshot(&remote, now);
    taskEXIT_CRITICAL();
    /* Frame time and every age/validity check share this refreshed clock.
     * The caller's original clock still governs the 20 ms transmit slots. */
    U32(f, VOFA_KALMAN_TICK_MS_HI, now);
#endif
    static const ChassisFusedVelocitySnapshot_s empty;
    if (!v) v = &empty;
    const ChassisVelocityDiagnostics_s *d = &v->diagnostics;
    const ChassisRemoteFusionAttempt_s *a = &v->remote.attempt;
    const ChassisRemoteFusionConfig_s *c = &a->config;
    uint32_t online = d->available ? d->wheel_online_mask & 15U : 0U;
    for (unsigned i = 0; i < 4; ++i) {
        if ((uint32_t)(now - d->wheel_tick_ms[i]) > 20U || !isfinite(d->raw_rpm[i])) online &= ~(1U << i);
        f[VOFA_KALMAN_RAW_RPM_LF + i] = d->raw_rpm[i];
        f[VOFA_KALMAN_WHEEL_AGE_MS_LF + i] = Age(now, d->wheel_tick_ms[i], d->available && (d->wheel_online_mask & (1U << i)));
    }
    const bool wheel = online == 15 && v->wheel.valid && (uint32_t)(now - v->wheel.tick_ms) <= 20U &&
        isfinite(v->wheel.vx_mps) && isfinite(v->wheel.vy_mps) && isfinite(v->wheel.wz_rpm);
    const bool target = sample && sample->chassis_valid && sample->velocity_target_valid &&
        (uint32_t)(now - sample->chassis_tick_ms) <= 20U && (uint32_t)(now - sample->velocity_target_tick_ms) <= 20U;
    const bool estimate = wheel && v->valid && isfinite(v->vx_mps) && isfinite(v->vy_mps) &&
        (uint32_t)(now - v->tick_ms) <= (v->status == CHASSIS_VELOCITY_FUSED ? 60U : 20U);
    const float *quaternion = d->imu_quaternion, *accel = d->imu_accel_n_mps2, *gyro = d->imu_gyro_sensor_radps;
    bool imu_known = d->available && (d->imu_valid || d->imu_sequence != 0);
    bool imu_usable = d->imu_valid;
    uint32_t imu_tick = d->imu_tick_ms, imu_sequence = d->imu_sequence;
    float imu_age = Age(now, imu_tick, imu_known), receive_age = imu_age, source_age = -1;
#if CHASSIS_IMU_CAN_RX_ENABLE
    imu_usable = remote_usable;
    imu_known = remote.available;
    quaternion = remote.sample.quaternion; accel = remote.sample.accel_n_mps2; gyro = remote.sample.gyro_sensor_radps;
    imu_sequence = remote.sample.sequence;
    imu_tick = remote.first_rx_tick_ms - remote.sample.source_age_ms;
    imu_age = Age(now, imu_tick, imu_known && remote.sample.source_age_ms != UINT16_MAX);
    receive_age = Age(now, remote.last_rx_tick_ms, imu_known);
    source_age = imu_known && remote.sample.source_age_ms != UINT16_MAX ? remote.sample.source_age_ms : -1;
#endif
    const bool imu = imu_known && imu_usable && imu_age >= 0 && imu_age <= 60 &&
        Finite(quaternion, 4) && Finite(accel, 3) && Finite(gyro, 3);
    const bool attempt = a->available && Finite(a->input.wheel_n_mps, 2) && Finite(a->input.accel_n_mps2, 2) &&
        Finite(a->output.velocity_n_mps, 2) && Finite(c->sensor_to_body_quaternion, 4) && Finite(c->imu_offset_body_m, 3) &&
        isfinite(c->filter.wheel_variance) && isfinite(c->filter.accel_variance) &&
        isfinite(c->filter.jerk_density) && isfinite(c->filter.max_velocity_residual_mps);
    K(SCHEMA) = VOFA_KALMAN_SCHEMA_ID;
    K(VALID) = (wheel ? 1U : 0U) | (target ? 2U : 0U) | (estimate ? 4U : 0U) | (imu ? 8U : 0U) |
        (attempt ? 16U : 0U) | (CHASSIS_REMOTE_IMU_CONFIG_VERIFIED ? 32U : 0U);
    K(OBSERVER_STATUS) = v == &empty ? -1.0f : (float)v->status;
    K(CONFIG_SOURCE) = 0; /* Fixed A-board configuration; no runtime filter override. */
    if (a->available) U32(f, VOFA_KALMAN_CONFIG_FINGERPRINT_HI, ConfigFingerprint(c));
    K(IMU_SOURCE) = CHASSIS_IMU_CAN_RX_ENABLE ? 1 : 0; /* 1=remote A board; legacy 2=DM. */
    K(WHEEL_ONLINE_MASK) = online;
    K(WHEEL_VX_MPS) = v->wheel.vx_mps; K(WHEEL_VY_MPS) = v->wheel.vy_mps;
    K(WHEEL_WZ_RADPS) = v->wheel.wz_rpm * (6.283185307179586f / 60.0f);
    if (sample) {
        K(TARGET_VX_MPS) = sample->velocity_target_vx_mmps * 0.001f;
        K(TARGET_VY_MPS) = sample->velocity_target_vy_mmps * 0.001f;
        K(TARGET_WZ_RADPS) = sample->velocity_target_wz_mrps * 0.006283185307179586f;
    }
    K(VELOCITY_VX_MPS) = v->vx_mps; K(VELOCITY_VY_MPS) = v->vy_mps;
    K(VELOCITY_AGE_MS) = Age(now, v->tick_ms, v->valid);
    for (unsigned i = 0; i < 4; ++i) f[VOFA_KALMAN_IMU_QW + i] = quaternion[i];
    for (unsigned i = 0; i < 3; ++i) {
        f[VOFA_KALMAN_IMU_AX_MPS2 + i] = accel[i];
        f[VOFA_KALMAN_IMU_GX_RADPS + i] = gyro[i];
    }
    K(IMU_AGE_MS) = imu_age;
    /* A-board source age plus first reception estimates the receiver epoch;
     * the boards' absolute clocks are not synchronized. */
    K(IMU_RECEIVE_AGE_MS) = receive_age;
    K(IMU_SOURCE_AGE_MS) = source_age;
    U32(f, VOFA_KALMAN_IMU_SEQUENCE_HI, imu_sequence);
    U32(f, VOFA_KALMAN_IMU_TICK_MS_HI, imu_tick);
    U32(f, VOFA_KALMAN_ATTEMPT_SEQUENCE_HI, a->sequence);
    K(ATTEMPT_FLAGS) = (a->available ? 1U : 0U) | (a->input.wheel_valid ? 2U : 0U) |
        (a->input.imu_valid ? 4U : 0U) | (a->initialized_before ? 8U : 0U) |
        (a->output.valid ? 16U : 0U) | (a->output.fused ? 32U : 0U);
    U32(f, VOFA_KALMAN_CALL_RECEIVER_MS_HI, a->receiver_call_ms);
    U32(f, VOFA_KALMAN_CALL_US_HI, a->input.now_us);
    U32(f, VOFA_KALMAN_WHEEL_US_HI, a->input.wheel_us);
    U32(f, VOFA_KALMAN_IMU_US_HI, a->input.imu_us);
    U32(f, VOFA_KALMAN_MIDDLE_RECEIVER_MS_HI, a->middle_receiver_ms);
    K(WHEEL_N_X_MPS) = a->input.wheel_n_mps[0]; K(WHEEL_N_Y_MPS) = a->input.wheel_n_mps[1];
    K(ACCEL_N_X_MPS2) = a->input.accel_n_mps2[0]; K(ACCEL_N_Y_MPS2) = a->input.accel_n_mps2[1];
    K(OUTPUT_N_X_MPS) = a->output.velocity_n_mps[0]; K(OUTPUT_N_Y_MPS) = a->output.velocity_n_mps[1];
    U32(f, VOFA_KALMAN_OUTPUT_SAMPLE_US_HI, a->output.sample_us);
    U32(f, VOFA_KALMAN_UPDATE_COUNT_HI, v->remote.updates);
    U32(f, VOFA_KALMAN_RESET_COUNT_HI, v->remote.resets);
    U32(f, VOFA_KALMAN_RESETS_BEFORE_HI, a->resets_before);
    K(PAIR_SPAN_MS) = a->pair_span_ms; K(SOURCE_STEP_MS) = a->source_step_ms; K(RECEIVE_STEP_MS) = a->receive_step_ms;
    K(FUSION_WHEEL_VARIANCE) = c->filter.wheel_variance;
    K(FUSION_ACCEL_VARIANCE) = c->filter.accel_variance;
    K(FUSION_JERK_DENSITY) = c->filter.jerk_density;
    K(FUSION_MAX_RESIDUAL_MPS) = c->filter.max_velocity_residual_mps;
    U32(f, VOFA_KALMAN_FUSION_MAX_AGE_US_HI, c->filter.max_age_us);
    U32(f, VOFA_KALMAN_FUSION_MAX_SKEW_US_HI, c->filter.max_skew_us);
    U32(f, VOFA_KALMAN_FUSION_MAX_GAP_US_HI, c->filter.max_gap_us);
    for (unsigned i = 0; i < 4; ++i) f[VOFA_KALMAN_MOUNT_QW + i] = c->sensor_to_body_quaternion[i];
    for (unsigned i = 0; i < 3; ++i) f[VOFA_KALMAN_OFFSET_X_M + i] = c->imu_offset_body_m[i];
    K(MAX_PAIR_SPAN_MS) = c->max_pair_span_ms; K(MAX_WHEEL_SKEW_MS) = c->max_wheel_skew_ms;
    K(MAX_TIMING_JITTER_MS) = c->max_timing_jitter_ms;
}
#undef K
#endif

void VofaObservationSend(const USBTelemetrySample_s *sample,
    const ChassisPowerSnapshot_s *power, const ChassisFusedVelocitySnapshot_s *velocity,
    const SuperCapSnapshot_s *cap, const RefereePowerSnapshot_s *referee, uint32_t now_ms)
{
    static bool started;
    static uint32_t last_attempt_ms, next_sequence;
    uint32_t elapsed = now_ms - last_attempt_ms;
    if (started && elapsed < 20U) return;
    last_attempt_ms = started ? now_ms - elapsed % 20U : now_ms;
    started = true;
    uint32_t sequence = next_sequence++;
    UART_HandleTypeDef *uart = BOARD_VOFA_UART;
    if (uart->gState != HAL_UART_STATE_READY) return;
    float *f = frame.channels;
    memset(f, 0, sizeof frame.channels);
    U32(f, 1, now_ms); U32(f, 3, sequence);
#if VOFA_OBSERVATION_PROFILE == 1
    (void)velocity;
    Power(f, sample, power, cap, referee, now_ms);
#else
    (void)power; (void)cap; (void)referee;
    Kalman(f, sample, velocity, now_ms);
#endif
    for (unsigned i = 0; i < 96; ++i) if (!isfinite(f[i])) f[i] = 0;
    uint8_t *bytes = (uint8_t *)&frame;
    for (size_t i = 0; i + 4 <= sizeof frame.channels; ++i) {
        if (memcmp(bytes + i, frame.tail, 4) == 0) {
            size_t last = i + 3;
            bytes[last - last % 4] ^= 1U;
        }
    }
    (void)HAL_UART_Transmit_IT(uart, bytes, sizeof frame);
}
#endif
