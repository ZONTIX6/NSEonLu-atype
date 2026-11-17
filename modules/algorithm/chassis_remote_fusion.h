#ifndef CHASSIS_REMOTE_FUSION_H
#define CHASSIS_REMOTE_FUSION_H

#include "chassis_velocity.h"
#include "chassis_velocity_center.h"
#include "chassis_velocity_fusion.h"
#include "../imu/chassis_imu_sample.h"

#define CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY 32U

typedef enum {
    CHASSIS_REMOTE_NO_SAMPLE,
    CHASSIS_REMOTE_WHEEL_UNAVAILABLE,
    CHASSIS_REMOTE_IMU_UNAVAILABLE,
    CHASSIS_REMOTE_NOT_ALIGNED,
    CHASSIS_REMOTE_WARMUP,
    CHASSIS_REMOTE_REJECTED,
    CHASSIS_REMOTE_FUSED,
    CHASSIS_REMOTE_CONFIG_INVALID
} ChassisRemoteFusionStatus_e;

typedef struct {
    /* Fixed body frame follows native A-board axes, centered on the chassis.
     * Wheel X/Y enter this frame with their public signs (no X reflection).
     * Longitudinal mapping checked against the 2.4 m and 3.2 m captures;
     * full installation/axis acceptance remains pending. */
    float sensor_to_body_quaternion[4]; /* Hamilton wxyz, sensor to A-board body. */
    float imu_offset_body_m[3]; /* Chassis center to sensor in A-board body axes, m. */
    float spin_accel_uncertainty_fraction; /* Fraction of predicted centripetal acceleration treated as uncertainty. */
    float max_accel_variance; /* Per-axis IMU variance ceiling, (m/s^2)^2. */
    ChassisVelocityFusionConfig_s filter;
    uint32_t max_pair_span_ms, max_wheel_skew_ms, max_timing_jitter_ms;
} ChassisRemoteFusionConfig_s;

/* Last actual production-filter call, including rejected calls. The input,
 * static config and per-call effective acceleration variance are copied before
 * the filter runs. Pipeline resets and
 * repeated source samples preserve this record; only a new call replaces it.
 * All us times are the filter's wrapping uint32 clock, not receiver uptime.
 * source/receive steps describe the newest source that completed this attempt;
 * middle_receiver_ms and pair_span_ms describe the paired middle sample. */
typedef struct {
    uint32_t sequence; /* Wrapping call count; available remains true at zero. */
    uint32_t receiver_call_ms, middle_receiver_ms, resets_before;
    uint32_t pair_span_ms, source_step_ms, receive_step_ms;
    ChassisVelocityFusionInput_s input;
    ChassisVelocityFusionOutput_s output;
    ChassisRemoteFusionConfig_s config;
    float effective_accel_variance; /* Actual measurement variance used for this attempt. */
    bool available, initialized_before;
} ChassisRemoteFusionAttempt_s;

typedef struct {
    bool valid, timing_estimated;
    ChassisRemoteFusionStatus_e status;
    float vx_mps, vy_mps; /* Chassis forward/right; current wheel fallback if not fused. */
    float paired_wheel_mps[2], center_accel_n_mps2[2];
    uint32_t tick_ms; /* Receiver-clock estimate of the middle sample epoch when fused. */
    /* Latest examined source identity/timing, not necessarily the middle sample. */
    uint32_t source_sequence, source_tick_ms, estimated_sample_ms;
    uint32_t source_step_ms, receive_step_ms, pair_span_ms;
    uint32_t updates, resets; /* Saturating counters; updates counts accepted filter pairs. */
    ChassisRemoteFusionAttempt_s attempt;
} ChassisRemoteFusionOutput_s;

/* Fixed storage, no allocation. Reset on startup or explicit configuration change. */
typedef struct {
    ChassisVelocity_s wheels[CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY];
    unsigned wheel_head, wheel_count;
    ChassisVelocityCenter_s center;
    ChassisVelocityFusion_s filter;
    uint32_t center_local_ms[2], center_pair_span_ms[2];
    ChassisRemoteFusionOutput_s held;
    uint32_t last_sequence, last_source_ms, last_receive_ms, last_estimated_ms;
    uint32_t source_step_ms, receive_step_ms, pair_span_ms;
    uint32_t updates, resets, last_now_ms;
    bool have_source, have_now;
    ChassisRemoteFusionAttempt_s attempt;
} ChassisRemoteFusion_s;

void ChassisRemoteFusionReset(ChassisRemoteFusion_s *state);
/* Wheel input and velocity output retain the public wheel convention;
 * rotation to/from navigation axes happens inside this adapter.
 * Source age <=20 ms, known total age <=60 ms, wheel age <=20 ms.
 * Source-board intervals supply the physical dt. first_rx-source_age pairs
 * wheel history and labels the output on the receiver clock; transport delay
 * remains unknown, and the two boards are not hardware synchronized.
 * Repeated source identities never refresh the estimate or reapply measurements.
 * A missing wheel invalidates everything; other failures return current wheels. */
void ChassisRemoteFusionUpdate(ChassisRemoteFusion_s *state,
    const ChassisRemoteFusionConfig_s *config, const ChassisVelocity_s *wheel,
    uint32_t wheel_skew_ms, const ChassisImuCanSnapshot_s *remote,
    bool remote_usable, uint32_t now_ms, ChassisRemoteFusionOutput_s *output);

#endif
