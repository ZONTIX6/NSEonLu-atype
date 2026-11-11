#include "chassis_velocity_center.h"
#include <math.h>

/* Avoid magnifying encoder quantization at the motor/INS update intervals. */
#define CENTER_MIN_STEP_US 15000U
#define CENTER_MAX_STEP_US 30000U

ChassisVelocityCenterResult_e ChassisVelocityCenterUpdate(ChassisVelocityCenter_s *state,
    const ChassisVelocityCenterSample_s *sample, ChassisVelocityCenterSample_s *center)
{
    for (unsigned axis=0;axis<3;++axis) {
        if (!isfinite(sample->imu_offset_n_m[axis]) || !isfinite(sample->accel_n_mps2[axis])) {
            state->count=0;
            return CHASSIS_CENTER_RESET;
        }
    }
    if (state->count==0) {
        state->samples[0]=*sample;
        state->count=1;
        return CHASSIS_CENTER_WAIT;
    }
    const uint32_t elapsed=sample->imu_us-state->samples[state->count-1].imu_us;
    if (elapsed>CENTER_MAX_STEP_US) {
        state->count=0;
        return CHASSIS_CENTER_RESET;
    }
    if (elapsed<CENTER_MIN_STEP_US) return CHASSIS_CENTER_WAIT;
    if (state->count==1) {
        state->samples[1]=*sample;
        state->count=2;
        return CHASSIS_CENTER_WAIT;
    }

    const ChassisVelocityCenterSample_s *older=&state->samples[0];
    const ChassisVelocityCenterSample_s *middle=&state->samples[1];
    const float prev_s=(float)(middle->imu_us-older->imu_us)*1e-6f;
    const float next_s=(float)elapsed*1e-6f;
    *center=*middle;
    for (unsigned axis=0;axis<3;++axis) {
        const float prev_v=(middle->imu_offset_n_m[axis]-older->imu_offset_n_m[axis])/prev_s;
        const float next_v=(sample->imu_offset_n_m[axis]-middle->imu_offset_n_m[axis])/next_s;
        center->accel_n_mps2[axis]-=2.0f*(next_v-prev_v)/(prev_s+next_s);
        if (!isfinite(center->accel_n_mps2[axis])) {
            state->count=0;
            return CHASSIS_CENTER_RESET;
        }
    }
    state->samples[0]=*middle;
    state->samples[1]=*sample;
    return CHASSIS_CENTER_READY;
}
