#include "chassis_velocity_fusion.h"
#include <stddef.h>

/* Opaque state and primitive arrays avoid duplicating C structure layouts in Python. */
size_t VofaFusionStateSize(void) { return sizeof(ChassisVelocityFusion_s); }
void VofaFusionReset(void *state) { ChassisVelocityFusionReset(state); }
void VofaFusionUpdate(void *state, const float config[4], const uint32_t limits[3],
                      const uint32_t times[3], const float values[4], uint32_t flags,
                      float velocity[2], uint32_t result[3])
{
    ChassisVelocityFusionConfig_s c = {
        .wheel_variance=config[0], .accel_variance=config[1], .jerk_density=config[2],
        .max_velocity_residual_mps=config[3], .max_age_us=limits[0],
        .max_skew_us=limits[1], .max_gap_us=limits[2]};
    ChassisVelocityFusionInput_s input = {
        .now_us=times[0], .wheel_us=times[1], .imu_us=times[2],
        .wheel_n_mps={values[0],values[1]}, .accel_n_mps2={values[2],values[3]},
        .wheel_valid=(flags&2U)!=0, .imu_valid=(flags&4U)!=0};
    ChassisVelocityFusionOutput_s output;
    ChassisVelocityFusionUpdate(state,&c,&input,&output);
    velocity[0]=output.velocity_n_mps[0]; velocity[1]=output.velocity_n_mps[1];
    result[0]=(output.valid?1U:0U)|(output.fused?2U:0U);
    result[1]=output.sample_us;
    result[2]=((ChassisVelocityFusion_s *)state)->initialized?1U:0U;
}
