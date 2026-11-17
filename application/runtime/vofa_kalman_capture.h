#ifndef VOFA_KALMAN_CAPTURE_H
#define VOFA_KALMAN_CAPTURE_H

#include "board_config.h"
#include "imu_capture_config.h"

#define VOFA_KALMAN_CAPTURE_ENABLED (VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE == 2 && \
    VOFA_OBSERVATION_PROFILE == 2 && CHASSIS_REMOTE_IMU_FUSION_ENABLE)

#if VOFA_KALMAN_CAPTURE_ENABLED
#include "chassis_velocity_observer.h"
#include "chassis_imu_sample.h"

#define VOFA_KALMAN_CAPTURE_CAPACITY 16U

typedef struct {
    uint32_t captured, sent, dropped, send_errors, pending, high_water;
} VofaKalmanCaptureStats_s;

/* Single producer: the chassis observer, after each observation. Copies a new
 * actual filter call (including rejection), status/chassis mode changes and
 * 100 ms heartbeats.
 * Bounded, nonblocking, no allocation or encoding. Never resets the queue. */
void VofaKalmanCapture(const ChassisFusedVelocitySnapshot_s *velocity,
    const ChassisImuCanSnapshot_s *remote, bool remote_usable,
    const ChassisVelocity_s *target, uint32_t now_ms);

/* Single consumer: telemetry task, polled every 1 ms. UART owns the separate
 * transmit buffer until ready. A failed submission retains the queue head. */
void VofaKalmanCaptureDrain(void);
void VofaKalmanCaptureGetStats(VofaKalmanCaptureStats_s *out);
#endif

#endif
