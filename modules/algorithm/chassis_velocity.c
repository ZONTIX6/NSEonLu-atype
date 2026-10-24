#include "chassis_velocity.h"
#include <stddef.h>
#include <math.h>

#define TWO_PI 6.2831853071795864769f
#define PROJECTION_45 0.7071067811865475244f

bool ChassisEstimateVelocity(const ChassisVelocityConfig_s *config,
                             const ChassisVelocityInput_s *input,
                             ChassisVelocity_s *output)
{
    if (output == NULL) return false;
    *output = (ChassisVelocity_s){0};
    if (config == NULL || input == NULL || (input->online_mask & 15U) != 15U)
        return false;
    const float geometry[] = {config->wheel_radius_m, config->reduction_ratio,
        config->wheel_base_m, config->track_width_m};
    for (unsigned i=0;i<4;++i)
        if (!isfinite(geometry[i]) || geometry[i]<=0) return false;

    float wheel[4];
    uint32_t oldest_age=0;
    const float scale=TWO_PI*config->wheel_radius_m/(60.0f*config->reduction_ratio);
    for (unsigned i=0;i<4;++i) {
        const uint32_t age=input->now_ms-input->feedback_tick_ms[i];
        if (age>CHASSIS_VELOCITY_MAX_AGE_MS || !isfinite(input->rpm[i])) return false;
        if (age>oldest_age) oldest_age=age;
        wheel[i]=input->rpm[i]*scale;
        if (config->omni_x) {
            if (config->direction[i]!=1.0f && config->direction[i]!=-1.0f) return false;
            wheel[i]*=config->direction[i];
        }
    }
    const float lf=wheel[0], rf=wheel[1], lb=wheel[2], rb=wheel[3];
    const float lever=(config->wheel_base_m+config->track_width_m)*.5f;
    ChassisVelocity_s result={.tick_ms=input->now_ms-oldest_age};
    if (config->omni_x) {
        result.vx_mps=(-lf+rf-lb+rb)/(4.0f*PROJECTION_45);
        result.vy_mps=(lf+rf-lb-rb)/(4.0f*PROJECTION_45);
        result.wz_rps=(lf+rf+lb+rb)/(4.0f*PROJECTION_45*lever*TWO_PI);
    } else {
        result.vx_mps=(-lf-rf+lb+rb)*.25f;
        result.vy_mps=(-lf+rf-lb+rb)*.25f;
        result.wz_rps=-(lf+rf+lb+rb)/(4.0f*lever*TWO_PI);
    }
    if (!isfinite(result.vx_mps) || !isfinite(result.vy_mps) || !isfinite(result.wz_rps))
        return false;
    result.valid=true;
    *output=result;
    return true;
}
