#include "chassis_motion.h"

#include <math.h>
#include <stdio.h>

static int failures;

static void Near(float want, float got, float tolerance, int line)
{
    if (!isfinite(got) || fabsf(want - got) > tolerance) {
        if (failures < 20)
            printf("FAIL line %d: want %.6f, got %.6f\n", line, (double)want, (double)got);
        ++failures;
    }
}
#define NEAR(want, got) Near((want), (got), 0.001f, __LINE__)

/* Catches a changed gain/sign/clamp/wrap or a missing per-cycle zero write. */
static void FollowRotation(void)
{
    const struct { float angle, original_wz; } cases[] = {
        {0, 0}, {30, -1500}, {-30, 1500}, {90, -4500}, {-90, 4500},
        {179.9f, -4500}, {-179.9f, 4500}, {180, -4500}, {-180, 4500},
        {330, 1500}, {-330, -1500}, {360, 0}, {-360, 0},
        {181, 4500}, {-181, -4500}, {720, 0},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=CHASSIS_FOLLOW_GIMBAL_YAW,
            .offset_angle=cases[i].angle, .wz=1234};
        float vx, vy;
        ChassisResolveMotion(&cmd, 4000, &vx, &vy);
        NEAR(EXPECT_GIMBAL_FORWARD ? 0 : cases[i].original_wz, cmd.wz);
        NEAR(cases[i].angle, cmd.offset_angle);
        NEAR(0, vx);
        NEAR(0, vy);
    }
#if EXPECT_GIMBAL_FORWARD
    for (int angle = -720; angle <= 720; ++angle) {
        Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=CHASSIS_FOLLOW_GIMBAL_YAW,
            .offset_angle=(float)angle, .wz=-4000};
        float vx, vy;
        ChassisResolveMotion(&cmd, 4000, &vx, &vy);
        NEAR(0, cmd.wz);
    }
#endif
}

/* Literal fixtures catch skipped/double rotation and either input sign error. */
static void Translation(void)
{
    const struct { float angle, vx, vy, want_x, want_y; } cases[] = {
        {0, 10, 0, 10, 0}, {0, -10, 0, -10, 0},
        {0, 0, 10, 0, 10}, {0, 0, -10, 0, -10},
        {90, 10, 0, 0, 10}, {90, -10, 0, 0, -10},
        {90, 0, 10, -10, 0}, {90, 0, -10, 10, 0},
        {-90, 10, 0, 0, -10}, {-90, -10, 0, 0, 10},
        {-90, 0, 10, 10, 0}, {-90, 0, -10, -10, 0},
        {180, 10, 0, -10, 0}, {180, -10, 0, 10, 0},
        {180, 0, 10, 0, -10}, {180, 0, -10, 0, 10},
        {-180, 10, 0, -10, 0}, {-180, -10, 0, 10, 0},
        {-180, 0, 10, 0, -10}, {-180, 0, -10, 0, 10},
        {360, 10, 0, 10, 0}, {270, 10, 0, 0, -10},
        {45, 10, 0, 7.071068f, 7.071068f},
        {45, 10, 10, 0, 14.142136f},
    };
    const chassis_mode_e modes[] = {CHASSIS_FOLLOW_GIMBAL_YAW, CHASSIS_NO_FOLLOW, CHASSIS_ROTATE};
    for (unsigned m = 0; m < sizeof modes / sizeof modes[0]; ++m) {
        for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
            Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=modes[m], .offset_angle=cases[i].angle,
                .vx=cases[i].vx, .vy=cases[i].vy};
            float vx, vy;
            ChassisResolveMotion(&cmd, 4000, &vx, &vy);
            const float expected_gimbal_vx = cases[i].vx +
                (modes[m] == CHASSIS_ROTATE ? CHASSIS_SPIN_DRIFT_COMPENSATION_VX : 0);
            const float expected_gimbal_vy = cases[i].vy +
                (modes[m] == CHASSIS_ROTATE ? CHASSIS_SPIN_DRIFT_COMPENSATION_VY : 0);
            const float co = arm_cos_f32(cases[i].angle * DEGREE_2_RAD);
            const float si = arm_sin_f32(cases[i].angle * DEGREE_2_RAD);
            NEAR(expected_gimbal_vx * co - expected_gimbal_vy * si, vx);
            NEAR(expected_gimbal_vx * si + expected_gimbal_vy * co, vy);
            NEAR(cases[i].vx, cmd.vx);
            NEAR(cases[i].vy, cmd.vy);
        }
    }
}

/* Across encoder/wrap boundaries the actual translation must stay continuous. */
static void ContinuousTranslation(void)
{
    const float angles[] = {359.99f, -0.01f, 0, 0.01f, 360.01f,
        179.99f, -180.01f, 180, -180, 180.01f, -179.99f};
    for (unsigned i = 0; i < sizeof angles / sizeof angles[0]; ++i) {
        Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=CHASSIS_FOLLOW_GIMBAL_YAW,
            .offset_angle=angles[i], .vx=10, .vy=5};
        float vx, vy;
        ChassisResolveMotion(&cmd, 4000, &vx, &vy);
        const float sign = i < 5 ? 1 : -1;
        Near(sign * 10, vx, 0.002f, __LINE__);
        Near(sign * 5, vy, 0.002f, __LINE__);
    }
    Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=CHASSIS_FOLLOW_GIMBAL_YAW, .vx=10};
    float last_x=10, last_y=0;
    for (int step = 1; step <= 720; ++step) {
        cmd.offset_angle = step * 0.5f;
        float vx, vy;
        ChassisResolveMotion(&cmd, 4000, &vx, &vy);
        Near(100, vx*vx + vy*vy, 0.001f, __LINE__);
        if (hypotf(vx-last_x, vy-last_y) > 0.09f) ++failures;
        last_x=vx; last_y=vy;
    }
}

/* Catches stale spin commands and changes to other mode selection semantics. */
static void ModeTransitions(void)
{
    Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=CHASSIS_ROTATE, .offset_angle=30};
    float vx, vy;
    ChassisResolveMotion(&cmd, -1234, &vx, &vy);
    NEAR(-1234, cmd.wz);
    cmd.chassis_mode=CHASSIS_FOLLOW_GIMBAL_YAW;
    ChassisResolveMotion(&cmd, 4000, &vx, &vy);
    NEAR(EXPECT_GIMBAL_FORWARD ? 0 : -1500, cmd.wz);
    cmd.chassis_mode=CHASSIS_ROTATE;
    ChassisResolveMotion(&cmd, 2345, &vx, &vy);
    NEAR(2345, cmd.wz);
    cmd.chassis_mode=CHASSIS_NO_FOLLOW;
    ChassisResolveMotion(&cmd, 4000, &vx, &vy);
    NEAR(0, cmd.wz);
    cmd.chassis_mode=CHASSIS_ZERO_FORCE;
    cmd.wz=321;
    ChassisResolveMotion(&cmd, 4000, &vx, &vy);
    NEAR(321, cmd.wz); /* Output disable remains downstream, as before. */
}

int main(void)
{
    FollowRotation();
    Translation();
    ContinuousTranslation();
    ModeTransitions();
    if (failures) return 1;
    puts("PASS: chassis motion policy, transforms and transitions");
    return 0;
}
