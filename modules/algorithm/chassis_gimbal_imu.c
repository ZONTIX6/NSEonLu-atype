#include "chassis_gimbal_imu.h"

#include <math.h>

static bool RotationMatrix(const float q[4], float rotation[3][3])
{
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    const float norm_squared = w*w + x*x + y*y + z*z;
    if (!isfinite(norm_squared) || norm_squared < 0.98f || norm_squared > 1.02f)
        return false;

    const float scale = 2.0f / norm_squared;
    rotation[0][0] = 1.0f - scale*(y*y + z*z);
    rotation[0][1] = scale*(x*y - w*z);
    rotation[0][2] = scale*(x*z + w*y);
    rotation[1][0] = scale*(x*y + w*z);
    rotation[1][1] = 1.0f - scale*(x*x + z*z);
    rotation[1][2] = scale*(y*z - w*x);
    rotation[2][0] = scale*(x*z - w*y);
    rotation[2][1] = scale*(y*z + w*x);
    rotation[2][2] = 1.0f - scale*(x*x + y*y);
    return true;
}

static float Dot(const float a[3], const float b[3])
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

bool ChassisGimbalImuPose(const ChassisGimbalImuGeometry_s *geometry,
                         const float sensor_to_nav_quaternion[4],
                         float yaw_rad, float pitch_rad,
                         ChassisGimbalImuPose_s *pose)
{
    if (!geometry || !sensor_to_nav_quaternion || !pose ||
        !isfinite(yaw_rad) || !isfinite(pitch_rad))
        return false;

    float sensor_to_nav[3][3], sensor_to_head[3][3];
    if (!RotationMatrix(sensor_to_nav_quaternion, sensor_to_nav) ||
        !RotationMatrix(geometry->sensor_to_head_quaternion, sensor_to_head))
        return false;

    const float cy = cosf(yaw_rad), sy = sinf(yaw_rad);
    const float cp = cosf(pitch_rad), sp = sinf(pitch_rad);
    ChassisGimbalImuPose_s result;
    for (unsigned i = 0; i < 3; ++i) {
        /* Undo mounting, then intrinsic pitch and yaw, in reverse order. */
        const float head_to_nav[3] = {
            Dot(sensor_to_nav[i], sensor_to_head[0]),
            Dot(sensor_to_nav[i], sensor_to_head[1]),
            Dot(sensor_to_nav[i], sensor_to_head[2]),
        };
        const float yaw_to_nav[3] = {
            cp*head_to_nav[0] + sp*head_to_nav[2],
            head_to_nav[1],
            -sp*head_to_nav[0] + cp*head_to_nav[2],
        };
        float *body_to_nav = result.body_to_nav[i];
        body_to_nav[0] = cy*yaw_to_nav[0] - sy*yaw_to_nav[1];
        body_to_nav[1] = sy*yaw_to_nav[0] + cy*yaw_to_nav[1];
        body_to_nav[2] = yaw_to_nav[2];

        result.imu_offset_nav_m[i] = Dot(body_to_nav, geometry->yaw_origin_b_m) +
                                     Dot(yaw_to_nav, geometry->pitch_origin_yaw_m) +
                                     Dot(head_to_nav, geometry->imu_origin_head_m);
        if (!isfinite(result.imu_offset_nav_m[i]))
            return false;
    }
    *pose = result;
    return true;
}
