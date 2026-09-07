#include "chassis_power_feedback.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

static int failures;

#define ASSERT_NEAR(expected, actual, tolerance)                              \
    do                                                                        \
    {                                                                         \
        const float expected_ = (expected);                                   \
        const float actual_ = (actual);                                       \
        const float tolerance_ = (tolerance);                                 \
        if (!isfinite(actual_) || fabsf(expected_ - actual_) > tolerance_)    \
        {                                                                     \
            printf("FAIL %s:%d: expected %.3f, got %.3f\n",                 \
                   __FILE__, __LINE__, (double)expected_, (double)actual_);   \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

_Static_assert(sizeof(Chassis_Power_Control_Data_s) == 3U * sizeof(float),
               "power feedback must contain exactly three floats");
_Static_assert(offsetof(Chassis_Power_Control_Data_s, predicted_unlimited_power_w) == 0U,
               "unexpected unlimited prediction offset");
_Static_assert(offsetof(Chassis_Power_Control_Data_s, predicted_limited_power_w) == sizeof(float),
               "unexpected limited prediction offset");
_Static_assert(offsetof(Chassis_Power_Control_Data_s, effective_limit_w) == 2U * sizeof(float),
               "unexpected effective limit offset");
_Static_assert(_Generic(((Chassis_Power_Control_Data_s *)0)->predicted_unlimited_power_w,
                        float: 1,
                        default: 0),
               "unlimited prediction must be float");
_Static_assert(_Generic(((Chassis_Power_Control_Data_s *)0)->predicted_limited_power_w,
                        float: 1,
                        default: 0),
               "limited prediction must be float");
_Static_assert(_Generic(((Chassis_Power_Control_Data_s *)0)->effective_limit_w,
                        float: 1,
                        default: 0),
               "effective limit must be float");

static int TestUploadContainsPowerControl(void)
{
    Chassis_Upload_Data_s upload = {0};
    upload.power_control.predicted_unlimited_power_w = 80.0f;
    upload.power_control.predicted_limited_power_w = 55.0f;
    upload.power_control.effective_limit_w = 60.0f;

    return upload.power_control.predicted_unlimited_power_w == 80.0f &&
           upload.power_control.predicted_limited_power_w == 55.0f &&
           upload.power_control.effective_limit_w == 60.0f;
}

static void TestValidPowerStateIsSummed(void)
{
    const ChassisPowerOutput_s power = {
        .effective_limit_w = 60.0f,
        .predicted_unlimited_power_w = {10.0f, 20.0f, 30.0f, 40.0f},
        .predicted_limited_power_w = {8.0f, 12.0f, 16.0f, 20.0f},
        .status = POWER_MODEL_STATUS_OK,
    };
    Chassis_Power_Control_Data_s feedback = {0};

    ChassisPowerFeedbackUpdate(&feedback, true, &power);

    ASSERT_NEAR(100.0f, feedback.predicted_unlimited_power_w, 1.0e-6f);
    ASSERT_NEAR(56.0f, feedback.predicted_limited_power_w, 1.0e-6f);
    ASSERT_NEAR(60.0f, feedback.effective_limit_w, 1.0e-6f);
}

static void TestInvalidPowerStateClearsFeedback(void)
{
    const ChassisPowerOutput_s power = {
        .effective_limit_w = 60.0f,
        .predicted_unlimited_power_w = {10.0f, 20.0f, 30.0f, 40.0f},
        .predicted_limited_power_w = {8.0f, 12.0f, 16.0f, 20.0f},
        .status = POWER_MODEL_STATUS_INVALID_BUDGET,
    };
    Chassis_Power_Control_Data_s feedback = {1.0f, 2.0f, 3.0f};

    ChassisPowerFeedbackUpdate(&feedback, true, &power);

    ASSERT_NEAR(0.0f, feedback.predicted_unlimited_power_w, 1.0e-6f);
    ASSERT_NEAR(0.0f, feedback.predicted_limited_power_w, 1.0e-6f);
    ASSERT_NEAR(0.0f, feedback.effective_limit_w, 1.0e-6f);
}

static void TestUnregisteredGroupClearsFeedback(void)
{
    const ChassisPowerOutput_s power = {.status = POWER_MODEL_STATUS_OK};
    Chassis_Power_Control_Data_s feedback = {1.0f, 2.0f, 3.0f};

    ChassisPowerFeedbackUpdate(&feedback, false, &power);

    ASSERT_NEAR(0.0f, feedback.predicted_unlimited_power_w, 1.0e-6f);
    ASSERT_NEAR(0.0f, feedback.predicted_limited_power_w, 1.0e-6f);
    ASSERT_NEAR(0.0f, feedback.effective_limit_w, 1.0e-6f);
}

static void TestNonFinitePowerStateClearsFeedback(void)
{
    const ChassisPowerOutput_s power = {
        .effective_limit_w = NAN,
        .predicted_unlimited_power_w = {10.0f, 20.0f, 30.0f, 40.0f},
        .predicted_limited_power_w = {8.0f, 12.0f, 16.0f, 20.0f},
        .status = POWER_MODEL_STATUS_OK,
    };
    Chassis_Power_Control_Data_s feedback = {1.0f, 2.0f, 3.0f};

    ChassisPowerFeedbackUpdate(&feedback, true, &power);

    ASSERT_NEAR(0.0f, feedback.predicted_unlimited_power_w, 1.0e-6f);
    ASSERT_NEAR(0.0f, feedback.predicted_limited_power_w, 1.0e-6f);
    ASSERT_NEAR(0.0f, feedback.effective_limit_w, 1.0e-6f);
}

int main(void)
{
    if (!TestUploadContainsPowerControl())
        return 1;
    TestValidPowerStateIsSummed();
    TestInvalidPowerStateClearsFeedback();
    TestUnregisteredGroupClearsFeedback();
    TestNonFinitePowerStateClearsFeedback();
    if (failures != 0)
        return 1;
    puts("PASS: chassis power feedback tests");
    return 0;
}
