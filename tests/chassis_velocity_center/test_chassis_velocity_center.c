#include "chassis_velocity_center.h"
#include <assert.h>
#include <math.h>

static ChassisVelocityCenterSample_s sample(uint32_t time_us, float x, float y)
{
    return (ChassisVelocityCenterSample_s){.imu_us=time_us, .wheel_us=time_us-500U,
        .imu_offset_n_m={x,y,0}, .accel_n_mps2={3,-2,0},
        .wheel_b_mps={1,2}, .body_to_nav_xy={{0,-1},{1,0}}};
}

int main(void)
{
    ChassisVelocityCenter_s state={0};
    ChassisVelocityCenterSample_s center;
    /* Offset moves as [1.5*t^2, -t^2, 0]. True center acceleration is zero.
       Use unequal spacing across uint32_t wrap to catch uniform-dt assumptions. */
    const uint32_t start=UINT32_MAX-10000U;
    ChassisVelocityCenterSample_s a=sample(start,0,0);
    ChassisVelocityCenterSample_s b=sample(start+20000U,0.0006f,-0.0004f);
    ChassisVelocityCenterSample_s c=sample(start+45000U,0.0030375f,-0.002025f);
    assert(ChassisVelocityCenterUpdate(&state,&a,&center)==CHASSIS_CENTER_WAIT);
    assert(ChassisVelocityCenterUpdate(&state,&a,&center)==CHASSIS_CENTER_WAIT);
    assert(ChassisVelocityCenterUpdate(&state,&b,&center)==CHASSIS_CENTER_WAIT);
    assert(ChassisVelocityCenterUpdate(&state,&c,&center)==CHASSIS_CENTER_READY);
    assert(center.imu_us==b.imu_us && center.wheel_us==b.wheel_us);
    assert(fabsf(center.accel_n_mps2[0])<0.0001f && fabsf(center.accel_n_mps2[1])<0.0001f);
    assert(center.wheel_b_mps[1]==2.0f && center.body_to_nav_xy[0][1]==-1.0f);

    assert(ChassisVelocityCenterUpdate(&state,&b,&center)==CHASSIS_CENTER_RESET);
    assert(ChassisVelocityCenterUpdate(&state,&c,&center)==CHASSIS_CENTER_WAIT);
    c.imu_us+=60000U;
    assert(ChassisVelocityCenterUpdate(&state,&c,&center)==CHASSIS_CENTER_RESET);
    c.accel_n_mps2[0]=NAN;
    assert(ChassisVelocityCenterUpdate(&state,&c,&center)==CHASSIS_CENTER_RESET);
}
