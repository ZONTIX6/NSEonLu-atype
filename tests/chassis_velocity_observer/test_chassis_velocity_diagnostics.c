#include "bsp_dwt.h"
#include "chassis_velocity_observer.h"
#include "gimbal.h"
#include "ins_task.h"

#include <assert.h>
#include <stdio.h>

static uint32_t now_ms = 1000;
static bool online[4] = {true, true, true, true};
static DJIMotorInstance motors[4];
DJIMotorInstance *motor_lf = &motors[0], *motor_rf = &motors[1];
DJIMotorInstance *motor_lb = &motors[2], *motor_rb = &motors[3];
static INSMotionSnapshot_s imu = {
    .quaternion = {.5f, -.5f, .5f, -.5f},
    .accel_n_mps2 = {1.25f, -2.5f, 3.75f},
    .gyro_sensor_radps = {.125f, -.25f, .375f},
    .tick_ms = 999, .cycles = 999U * 168000U, .sequence = 37, .valid = true,
};
static GimbalEncoderSample_s joints[2] = {
    {.tick_ms = 998, .cycles = 998U * 168000U, .ecd = 1234, .rpm = -42, .valid = true},
    {.tick_ms = 997, .cycles = 997U * 168000U, .ecd = 5678, .rpm = 17},
};
static bool joints_valid = true;

bool DaemonIsOnline(const bool *value) { return *value; }
uint32_t HAL_GetTick(void) { return now_ms; }
uint32_t DWT_GetCycleCount(void) { return now_ms * 168000U; }
uint32_t DWT_GetFrequencyHz(void) { return 168000000; }
bool INSGetMotionSnapshot(INSMotionSnapshot_s *out) { *out = imu; return imu.valid; }
bool INSGetSnapshot(INSSnapshot_s *out)
{ *out=(INSSnapshot_s){.attitude={.YawTotalAngle=721.25f},.tick_ms=999,.valid=true,.has_sample=true}; return true; }
bool GimbalGetYawControlSnapshot(DJIMotorControlSnapshot_s *out)
{
    *out=(DJIMotorControlSnapshot_s){.available=true,.tick_ms=998,.reference=731.5f,
        .speed_dps=-12.5f,.speed_iout=3000,.command=7000,
        .flags=DJI_CONTROL_ENABLED | DJI_CONTROL_SPEED_CALCULATED | DJI_CONTROL_INTEGRAL_AT_LIMIT};
    return true;
}
bool GimbalGetEncoderSnapshot(GimbalEncoderSample_s out[2])
{
    if (!joints_valid) return false;
    out[0] = joints[0]; out[1] = joints[1];
    return true;
}

static const ChassisVelocityConfig_s config = {
    .omni_x = true, .wheel_radius_m = .06f, .reduction_ratio = 19,
    .wheel_base_m = .35f, .track_width_m = .30f, .direction = {1, 1, 1, 1},
};

int main(void)
{
    ChassisFusedVelocitySnapshot_s out;
    assert(!ChassisGetFusedVelocitySnapshot(&out));
    assert(!out.diagnostics.available);
    const int16_t rpm[4] = {101, -202, 303, -404};
    for (unsigned w = 0; w < 4; ++w) {
        motors[w].daemon = &online[w];
        motors[w].measure.raw_rpm = rpm[w];
        motors[w].measure.rx_sequence = 10 + w;
        motors[w].measure.rx_tick_ms = 998 - w;
        motors[w].measure.rx_cycles = (998 - w) * 168000U;
    }

    ChassisObserveFusedVelocity(&config, 1, false, false);
    assert(ChassisGetFusedVelocitySnapshot(&out));
#if !CHASSIS_VELOCITY_FUSION_CALIBRATED
    assert(out.status == CHASSIS_VELOCITY_UNCALIBRATED);
#endif
    const ChassisVelocityDiagnostics_s *d = &out.diagnostics;
    assert(d->available && d->tick_ms == 1000 && d->wheel_online_mask == 15);
    assert(d->raw_rpm[0] == 101 && d->raw_rpm[1] == -202);
    assert(d->raw_rpm[2] == 303 && d->raw_rpm[3] == -404);
    assert(d->wheel_tick_ms[0] == 998 && d->wheel_tick_ms[1] == 997);
    assert(d->wheel_tick_ms[2] == 996 && d->wheel_tick_ms[3] == 995);
    assert(d->imu_valid && d->imu_tick_ms == 999 && d->imu_sequence == 37);
    assert(d->imu_quaternion[0] == .5f && d->imu_quaternion[1] == -.5f);
    assert(d->imu_quaternion[2] == .5f && d->imu_quaternion[3] == -.5f);
    assert(d->imu_accel_n_mps2[0] == 1.25f && d->imu_accel_n_mps2[1] == -2.5f);
    assert(d->imu_accel_n_mps2[2] == 3.75f);
    assert(d->imu_gyro_sensor_radps[0] == .125f && d->imu_gyro_sensor_radps[1] == -.25f);
    assert(d->imu_gyro_sensor_radps[2] == .375f);
    assert(d->joints_valid && d->joint_ecd[0] == 1234 && d->joint_ecd[1] == 5678);
    assert(d->joint_rpm[0] == -42 && d->joint_rpm[1] == 17);
    assert(d->joint_tick_ms[0] == 998 && d->joint_tick_ms[1] == 997);
    assert(d->yaw_encoder_valid && d->chassis_mode == 1);
    assert(d->gimbal_imu_known && d->gimbal_imu_valid && d->gimbal_imu_tick_ms == 999);
    assert(d->gimbal_yaw_total_deg == 721.25f);
    assert(d->yaw_control.available && d->yaw_control.tick_ms == 998);
    assert(d->yaw_control.reference == 731.5f && d->yaw_control.speed_dps == -12.5f);
    assert(d->yaw_control.speed_iout == 3000 && d->yaw_control.command == 7000);
    assert(d->yaw_control.flags & DJI_CONTROL_INTEGRAL_AT_LIMIT);

    /* A late getter invalidates velocity, but preserves the diagnostic epoch
       and values so the CSV consumer can identify stale observations. */
    now_ms = 1021;
    assert(!ChassisGetFusedVelocitySnapshot(&out));
    assert(d->available && d->tick_ms == 1000 && d->imu_sequence == 37);
    assert(d->raw_rpm[3] == -404 && d->joint_ecd[1] == 5678);

    imu.valid = false;
    joints_valid = false;
    online[1] = false;
    motors[2].measure.rx_sequence = 0;
    motor_rb = NULL;
    ChassisObserveFusedVelocity(&config, 1, false, false);
    assert(!ChassisGetFusedVelocitySnapshot(&out));
    assert(out.status == CHASSIS_VELOCITY_WHEEL_UNAVAILABLE);
    assert(d->available && d->tick_ms == 1021 && d->wheel_online_mask == 1);
    assert(d->raw_rpm[0] == 101 && d->wheel_tick_ms[0] == 998);
    for (unsigned w = 1; w < 4; ++w)
        assert(d->raw_rpm[w] == 0 && d->wheel_tick_ms[w] == 0);
    assert(!d->imu_valid && d->imu_sequence == 37 && d->imu_tick_ms == 999);
    assert(!d->joints_valid);
    assert(d->joint_ecd[0] == 0 && d->joint_ecd[1] == 0);
    assert(d->joint_tick_ms[0] == 0 && d->joint_tick_ms[1] == 0);
    puts("PASS: chassis raw diagnostics, stale getter and source loss");
    return 0;
}
