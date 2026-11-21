#define main RemoteObserverRegressionMain
#include "test_chassis_remote_observer.c"
#undef main
#include "../../config/30_velocity_observation/chassis_traction_config.h"

int main(void)
{
    RemoteObserverRegressionMain();
    ChassisFusedVelocitySnapshot_s sample={.valid=true,.status=CHASSIS_VELOCITY_FUSED,
        .vx_mps=1,.vy_mps=-2,.tick_ms=tick-30,
        .diagnostics={.available=true,.imu_valid=true,.imu_tick_ms=tick-10,
            .imu_gyro_sensor_radps={.1f,.2f,3}}};
    chassis_fused_velocity=sample;
    ChassisTractionReference_s ref;
    assert(ChassisGetTractionReference(&ref));
    assert(ref.vx_mps==1 && ref.vy_mps==-2 && ref.tick_ms==tick-30 && ref.gyro_tick_ms==tick-10);
    assert(fabsf(ref.wz_radps-3*CHASSIS_TRACTION_GYRO_Z_SIGN)<.00001f);
    sample.status=CHASSIS_VELOCITY_IMU_UNAVAILABLE;chassis_fused_velocity=sample;
    assert(!ChassisGetTractionReference(&ref)); /* Wheel fallback is never a slip reference. */
    sample.status=CHASSIS_VELOCITY_FUSED;sample.tick_ms=tick-41;chassis_fused_velocity=sample;
    assert(!ChassisGetTractionReference(&ref));
    sample.tick_ms=tick;sample.diagnostics.imu_tick_ms=tick-41;chassis_fused_velocity=sample;
    assert(!ChassisGetTractionReference(&ref));
    sample.diagnostics.imu_tick_ms=tick;sample.diagnostics.imu_gyro_sensor_radps[0]=NAN;
    chassis_fused_velocity=sample;assert(!ChassisGetTractionReference(&ref));
    assert(!ChassisGetTractionReference(NULL));
    puts("PASS: traction uses fresh fused translation and independent, signed chassis gyro");return 0;
}
