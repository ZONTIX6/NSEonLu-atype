#include "chassis_gimbal_imu.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void AssertClose(float actual, float expected)
{
    assert(fabsf(actual - expected) < 1e-6f);
}

static void StationaryBodyWithMovingHead(void)
{
    const ChassisGimbalImuGeometry_s geometry = {
        .yaw_origin_b_m = {0.1f, 0.2f, 0.3f},
        .pitch_origin_yaw_m = {0.4f, 0.5f, 0.6f},
        .imu_origin_head_m = {0.7f, 0.8f, 0.9f},
        .sensor_to_head_quaternion = {1.0f, 0.0f, 0.0f, 0.0f},
    };
    const float half_pi = 1.57079632679f;
    const float half_sqrt2 = 0.70710678118f;
    const struct {
        float yaw, pitch;
        float sensor_to_nav[4], imu_offset_nav_m[3];
    } cases[] = {
        {0, 0, {1, 0, 0, 0}, {1.2f, 1.5f, 1.8f}},
        {half_pi, 0, {half_sqrt2, 0, 0, half_sqrt2}, {-1.2f, 1.3f, 1.8f}},
        {0, half_pi, {half_sqrt2, 0, half_sqrt2, 0}, {1.4f, 1.5f, 0.2f}},
        {half_pi, half_pi, {0.5f, -0.5f, 0.5f, 0.5f}, {-1.2f, 1.5f, 0.2f}},
    };
    for (unsigned k = 0; k < sizeof cases / sizeof cases[0]; ++k) {
        ChassisGimbalImuPose_s pose;
        assert(ChassisGimbalImuPose(&geometry, cases[k].sensor_to_nav,
                                    cases[k].yaw, cases[k].pitch, &pose));
        for (unsigned i = 0; i < 3; ++i) {
            AssertClose(pose.imu_offset_nav_m[i], cases[k].imu_offset_nav_m[i]);
            for (unsigned j = 0; j < 3; ++j)
                AssertClose(pose.body_to_nav[i][j], i == j ? 1.0f : 0.0f);
        }
    }
}

static void MountedSensorOnTiltedBody(void)
{
    const ChassisGimbalImuGeometry_s geometry = {
        .imu_origin_head_m = {0, 1, 1},
        .sensor_to_head_quaternion = {0.70710678118f, 0, 0, 0.70710678118f},
    };
    /* Sensor is turned 90 deg about head +Z, while the body is rolled 90 deg about +X. */
    const float sensor_to_nav[4] = {0.5f, 0.5f, -0.5f, 0.5f};
    const float body_to_nav[3][3] = {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}};
    const float imu_offset_nav_m[3] = {0, -1, 1};
    ChassisGimbalImuPose_s pose;
    assert(ChassisGimbalImuPose(&geometry, sensor_to_nav, 0, 0, &pose));
    for (unsigned i = 0; i < 3; ++i) {
        AssertClose(pose.imu_offset_nav_m[i], imu_offset_nav_m[i]);
        for (unsigned j = 0; j < 3; ++j)
            AssertClose(pose.body_to_nav[i][j], body_to_nav[i][j]);
    }
}

static void InvalidPoseRejected(void)
{
    ChassisGimbalImuGeometry_s geometry = {.sensor_to_head_quaternion = {1, 0, 0, 0}};
    const float q_ok[4] = {1, 0, 0, 0};
    const float q_zero[4] = {0};
    const float q_scaled[4] = {2, 0, 0, 0};
    const float q_nan[4] = {1, NAN, 0, 0};
    ChassisGimbalImuPose_s pose;
    assert(!ChassisGimbalImuPose(&geometry, q_zero, 0, 0, &pose));
    assert(!ChassisGimbalImuPose(&geometry, q_scaled, 0, 0, &pose));
    assert(!ChassisGimbalImuPose(&geometry, q_nan, 0, 0, &pose));
    assert(!ChassisGimbalImuPose(&geometry, q_ok, NAN, 0, &pose));
    assert(!ChassisGimbalImuPose(&geometry, q_ok, 0, INFINITY, &pose));
    geometry.yaw_origin_b_m[1] = INFINITY;
    assert(!ChassisGimbalImuPose(&geometry, q_ok, 0, 0, &pose));
    geometry.yaw_origin_b_m[1] = 0;
    geometry.sensor_to_head_quaternion[0] = 0;
    assert(!ChassisGimbalImuPose(&geometry, q_ok, 0, 0, &pose));
}

int main(void)
{
    StationaryBodyWithMovingHead();
    MountedSensorOnTiltedBody();
    InvalidPoseRejected();
    puts("chassis gimbal IMU geometry tests passed");
    return 0;
}
