#ifndef CHASSIS_VELOCITY_FUSION_H
#define CHASSIS_VELOCITY_FUSION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float wheel_variance;              /* (m/s)^2 */
    float accel_variance;              /* (m/s^2)^2 */
    float jerk_density;                /* White jerk spectral density, m^2/s^5. */
    float max_velocity_residual_mps;   /* Euclidean innovation/discrepancy limit. */
    uint32_t max_age_us, max_skew_us, max_gap_us;
} ChassisVelocityFusionConfig_s;

/* Both vectors are in the same fixed navigation frame. The acceleration has
 * already been corrected for gravity and referred to the chassis center. All
 * times use one wrapping microsecond clock. Repeated observations keep their
 * original timestamps and values. */
typedef struct {
    uint32_t now_us, wheel_us, imu_us;
    float wheel_n_mps[2], accel_n_mps2[2];
    bool wheel_valid, imu_valid;
} ChassisVelocityFusionInput_s;

typedef struct {
    float velocity_n_mps[2];
    bool valid, fused;
    uint32_t sample_us;  /* Filter epoch if fused, wheel observation otherwise. */
} ChassisVelocityFusionOutput_s;

typedef struct {
    float velocity, accel;
    float p_vv, p_va, p_aa;
} ChassisVelocityFusionAxis_s;

/* Statically allocated filter state. Call Reset at startup and on clock reset. */
typedef struct {
    ChassisVelocityFusionAxis_s axis[2];
    uint32_t state_us, last_wheel_us, last_imu_us;
    bool initialized, have_wheel_time, have_imu_time;
} ChassisVelocityFusion_s;

void ChassisVelocityFusionReset(ChassisVelocityFusion_s *state);
/* Falls back to fresh wheel odometry on IMU loss or filter rejection. Output
 * is invalid if wheels are unavailable. No prediction beyond the newest
 * observation is published. */
void ChassisVelocityFusionUpdate(ChassisVelocityFusion_s *state,
                                const ChassisVelocityFusionConfig_s *config,
                                const ChassisVelocityFusionInput_s *input,
                                ChassisVelocityFusionOutput_s *output);

#endif
