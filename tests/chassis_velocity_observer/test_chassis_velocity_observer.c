#include "bsp_dwt.h"
#include "chassis_velocity_observer.h"
#include "gimbal.h"
#include "ins_task.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
#define STEP_US UINT32_C(20000)
#define CLOCK_HZ UINT32_C(168000000)

static uint64_t now_us = UINT64_C(25560000); /* Cross the 32-bit DWT wrap on frame 2. */
static uint32_t cycle_phase;
static unsigned frame;
static bool motor_online[4] = {true, true, true, true};
static DJIMotorInstance motors[4];
DJIMotorInstance *motor_lf = &motors[0], *motor_rf = &motors[1];
DJIMotorInstance *motor_lb = &motors[2], *motor_rb = &motors[3];
static INSMotionSnapshot_s imu;
static GimbalEncoderSample_s joints[2];
static bool gimbal_valid = true;

static const ChassisVelocityConfig_s wheel_config = {
    .omni_x = true, .wheel_radius_m = .06f, .reduction_ratio = 19,
    .wheel_base_m = .35f, .track_width_m = .30f, .direction = {1, 1, 1, 1},
};

bool DaemonIsOnline(const bool *online) { return *online; }
uint32_t HAL_GetTick(void) { return (uint32_t)(now_us / 1000U); }
uint32_t DWT_GetCycleCount(void) { return (uint32_t)(now_us * (CLOCK_HZ / 1000000U)) + cycle_phase; }
uint32_t DWT_GetFrequencyHz(void) { return CLOCK_HZ; }

bool INSGetMotionSnapshot(INSMotionSnapshot_s *snapshot)
{
    *snapshot = imu;
    return imu.valid;
}

bool GimbalGetEncoderSnapshot(GimbalEncoderSample_s samples[2])
{
    if (!gimbal_valid) return false;
    samples[0] = joints[0];
    samples[1] = joints[1];
    return true;
}

static void Near(float actual, float expected, float tolerance)
{
    assert(isfinite(actual));
    assert(fabsf(actual - expected) < tolerance);
}

static void NewFrame(int16_t right_rpm)
{
    const int yaw_count = 25 * (int)frame;
    const int pitch_count = 30 * (int)frame;
    const double yaw_speed = 25.0 * (2.0 * PI / 8192.0) / .020;
    const double pitch_speed = 30.0 * (2.0 * PI / 8192.0) / .020;
    /* The chassis is held at +90 degrees in N. Only the head rotates. */
    const double yaw = PI / 2.0 + yaw_count * (2.0 * PI / 8192.0);
    const double pitch = pitch_count * (2.0 * PI / 8192.0);
    const double radial = .10 + .20 * cos(pitch);
    const double radial_dot = -.20 * pitch_speed * sin(pitch);
    const double radial_ddot = -.20 * pitch_speed * pitch_speed * cos(pitch);
    /* Exact second derivative of r(t)[cos(yaw),sin(yaw)], the sensor's
       horizontal path about a fixed base. The chassis center acceleration is 0. */
    const double normal = radial_ddot - radial * yaw_speed * yaw_speed;
    const double tangent = 2.0 * radial_dot * yaw_speed;
    const double cy = cos(yaw / 2.0), sy = sin(yaw / 2.0);
    const double cp = cos(pitch / 2.0), sp = sin(pitch / 2.0);
    imu = (INSMotionSnapshot_s){
        .quaternion = {(float)(cy * cp), (float)(-sy * sp), (float)(cy * sp), (float)(sy * cp)},
        .accel_n_mps2 = {(float)(normal * cos(yaw) - tangent * sin(yaw)),
                         (float)(normal * sin(yaw) + tangent * cos(yaw)),
                         (float)(.20 * pitch_speed * pitch_speed * sin(pitch))},
        .tick_ms = HAL_GetTick(), .cycles = DWT_GetCycleCount(), .sequence = frame + 1, .valid = true,
    };
    joints[0] = (GimbalEncoderSample_s){ .tick_ms = imu.tick_ms, .cycles = imu.cycles,
        .ecd = (uint16_t)(1000 + yaw_count) };
    joints[1] = (GimbalEncoderSample_s){ .tick_ms = imu.tick_ms, .cycles = imu.cycles,
        .ecd = (uint16_t)(2000 - pitch_count) };
    for (unsigned wheel = 0; wheel < 4; ++wheel) {
        motors[wheel].measure.raw_rpm = wheel < 2 ? right_rpm : -right_rpm;
        motors[wheel].measure.rx_sequence = frame + 1;
        motors[wheel].measure.rx_tick_ms = imu.tick_ms;
        motors[wheel].measure.rx_cycles = imu.cycles;
        motors[wheel].daemon = &motor_online[wheel];
    }
    ++frame;
}

static ChassisFusedVelocitySnapshot_s Observe(bool expected_valid,
                                              ChassisFusedVelocityStatus_e status)
{
    ChassisFusedVelocitySnapshot_s out;
    ChassisObserveFusedVelocity(&wheel_config);
    assert(ChassisGetFusedVelocitySnapshot(&out) == expected_valid);
    assert(out.valid == expected_valid);
    assert(out.status == status);
    return out;
}

static ChassisFusedVelocitySnapshot_s RecoverWithRightMotion(void)
{
    ChassisFusedVelocitySnapshot_s out;
    for (unsigned i = 0; i < 3; ++i) {
        now_us += STEP_US;
        NewFrame(1900);
        out = Observe(true, i == 2 ? CHASSIS_VELOCITY_FUSED : CHASSIS_VELOCITY_WARMUP);
    }
    Near(out.vx_mps, 0, .00001f);
    /* 1900 rotor rpm with 19:1 gearing gives .62831853 m/s tangential
       at r=.06m. Divide by cos(45°) to get .88857659 m/s rightward. */
    Near(out.vy_mps, .88857659f, .00001f);
    return out;
}

int main(void)
{
    ChassisFusedVelocitySnapshot_s out;
    assert(!ChassisGetFusedVelocitySnapshot(NULL));
    assert(!ChassisGetFusedVelocitySnapshot(&out));
    for (unsigned i = 0; i < 8; ++i) {
        NewFrame(0);
        if (i == 7) --imu.cycles;
        out = Observe(true, i < 2 ? CHASSIS_VELOCITY_WARMUP : CHASSIS_VELOCITY_FUSED);
        if (i >= 2) {
            Near(out.vx_mps, 0, .0001f);
            Near(out.vy_mps, 0, .0001f);
            Near(out.center_accel_n_mps2[0], 0, .003f);
            Near(out.center_accel_n_mps2[1], 0, .003f);
            assert(out.tick_ms == HAL_GetTick() - STEP_US / 1000U);
        }
        if (i == 7) {
            /* Reading the same sample with a different DWT fractional phase
               can round its reconstructed time a microsecond earlier. */
            now_us += 1000;
            cycle_phase = 167;
            (void)Observe(true, CHASSIS_VELOCITY_FUSED);
            cycle_phase = 0;
            now_us += STEP_US - 1000;
        } else {
            now_us += STEP_US;
        }
    }

    NewFrame(1900);
    imu.valid = false;
    out = Observe(true, CHASSIS_VELOCITY_IMU_UNAVAILABLE);
    Near(out.vy_mps, .88857659f, .00001f);
    assert(out.tick_ms == HAL_GetTick());
    (void)RecoverWithRightMotion();

    now_us += STEP_US;
    NewFrame(1900);
    gimbal_valid = false;
    out = Observe(true, CHASSIS_VELOCITY_GIMBAL_UNAVAILABLE);
    Near(out.vy_mps, .88857659f, .00001f);
    gimbal_valid = true;

    now_us += STEP_US;
    NewFrame(1900);
    motor_online[3] = false;
    out = Observe(false, CHASSIS_VELOCITY_WHEEL_UNAVAILABLE);
    assert(!out.wheel.valid);
    motor_online[3] = true;

    now_us += STEP_US;
    NewFrame(1900);
    motors[3].measure.rx_cycles -= 4 * (CLOCK_HZ / 1000U);
    motors[3].measure.rx_tick_ms -= 4;
    out = Observe(true, CHASSIS_VELOCITY_NOT_ALIGNED);
    Near(out.vy_mps, .88857659f, .00001f);

    (void)RecoverWithRightMotion();
    now_us += 20000;
    assert(ChassisGetFusedVelocitySnapshot(&out));
    assert(out.wheel.valid);
    now_us += 1000;
    assert(!ChassisGetFusedVelocitySnapshot(&out));
    assert(!out.valid && !out.wheel.valid);
    puts("PASS: chassis velocity observer with rotating gimbal and source loss");
    return 0;
}
