#include "chassis_power_feedback.h"

#include <math.h>
#include <stddef.h>

static void ClearFeedback(Chassis_Power_Control_Data_s *feedback)
{
    feedback->predicted_unlimited_power_w = 0.0f;
    feedback->predicted_limited_power_w = 0.0f;
    feedback->effective_limit_w = 0.0f;
}

void ChassisPowerFeedbackUpdate(Chassis_Power_Control_Data_s *feedback,
                                bool registered,
                                const ChassisPowerOutput_s *power)
{
    if (feedback == NULL)
        return;

    ClearFeedback(feedback);
    if (!registered || power == NULL || power->status != POWER_MODEL_STATUS_OK)
        return;

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        feedback->predicted_unlimited_power_w +=
            power->predicted_unlimited_power_w[i];
        feedback->predicted_limited_power_w +=
            power->predicted_limited_power_w[i];
    }
    feedback->effective_limit_w = power->effective_limit_w;

    if (!isfinite(feedback->predicted_unlimited_power_w) ||
        !isfinite(feedback->predicted_limited_power_w) ||
        !isfinite(feedback->effective_limit_w))
        ClearFeedback(feedback);
}
