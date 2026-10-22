#include "chassis_kinematics.h"

#include <stddef.h>

#define OMNI_X_PROJECTION 0.70710678118654752440f

bool ChassisMecanumCalculate(const ChassisKinematicsInput_s *input,
                             ChassisWheelOutput_s *output)
{
    if (input == NULL || output == NULL)
        return false;

    output->speed_aps[CHASSIS_WHEEL_LEFT_FRONT] =
        -input->vx - input->vy - input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_LEFT_FRONT];
    output->speed_aps[CHASSIS_WHEEL_RIGHT_FRONT] =
        -input->vx + input->vy - input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_RIGHT_FRONT];
    output->speed_aps[CHASSIS_WHEEL_LEFT_BACK] =
        input->vx - input->vy - input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_LEFT_BACK];
    output->speed_aps[CHASSIS_WHEEL_RIGHT_BACK] =
        input->vx + input->vy - input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_RIGHT_BACK];
    return true;
}

bool ChassisOmniXCalculate(const ChassisKinematicsInput_s *input,
                           const float wheel_direction[CHASSIS_WHEEL_COUNT],
                           ChassisWheelOutput_s *output)
{
    if (input == NULL || wheel_direction == NULL || output == NULL)
        return false;

    output->speed_aps[CHASSIS_WHEEL_LEFT_FRONT] =
        (-input->vx * OMNI_X_PROJECTION + input->vy * OMNI_X_PROJECTION +
         input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_LEFT_FRONT]) *
        wheel_direction[CHASSIS_WHEEL_LEFT_FRONT];
    output->speed_aps[CHASSIS_WHEEL_RIGHT_FRONT] =
        (input->vx * OMNI_X_PROJECTION + input->vy * OMNI_X_PROJECTION +
         input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_RIGHT_FRONT]) *
        wheel_direction[CHASSIS_WHEEL_RIGHT_FRONT];
    output->speed_aps[CHASSIS_WHEEL_LEFT_BACK] =
        (-input->vx * OMNI_X_PROJECTION - input->vy * OMNI_X_PROJECTION +
         input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_LEFT_BACK]) *
        wheel_direction[CHASSIS_WHEEL_LEFT_BACK];
    output->speed_aps[CHASSIS_WHEEL_RIGHT_BACK] =
        (input->vx * OMNI_X_PROJECTION - input->vy * OMNI_X_PROJECTION +
         input->wz * input->rotation_lever_sum[CHASSIS_WHEEL_RIGHT_BACK]) *
        wheel_direction[CHASSIS_WHEEL_RIGHT_BACK];
    return true;
}
