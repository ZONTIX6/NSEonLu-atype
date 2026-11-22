#include "chassis_wheel_redundancy.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static const ChassisVelocityConfig_s kinematics = {
    .omni_x = true, .wheel_radius_m = .076f, .reduction_ratio = 19.0f,
    .wheel_base_m = .35f, .track_width_m = .30f,
    .direction = {1.0f, 1.0f, 1.0f, 1.0f},
};

static const ChassisWheelRedundancyConfig_s config = {
    .enabled = true, .residual_threshold_mps = .20f,
    .dominance_ratio = 1.75f, .max_reference_error_mps = .10f,
};

static void Near(float actual, float expected)
{
    assert(isfinite(actual) && fabsf(actual - expected) < .00002f);
}

static void WheelsFor(float vx, float vy, float wz, float wheels[4])
{
    const float k = 0.7071067811865475244f;
    const float lever = (kinematics.wheel_base_m + kinematics.track_width_m) * .5f;
    wheels[0] = (-vx + vy + wz * lever) * k;
    wheels[1] = (vx + vy + wz * lever) * k;
    wheels[2] = (-vx - vy + wz * lever) * k;
    wheels[3] = (vx - vy + wz * lever) * k;
}

static ChassisWheelRedundancyInput_s Input(float vx, float vy, float wz)
{
    return (ChassisWheelRedundancyInput_s){
        .valid = true, .online_mask = 15, .reference_valid = true,
        .reference_vx_mps = vx, .reference_vy_mps = vy,
        .reference_wz_radps = wz,
    };
}

static void NoSlipKeepsAllFour(void)
{
    float wheels[4];
    WheelsFor(.6f, -.2f, 1.1f, wheels);
    ChassisWheelRedundancyOutput_s output;
    const ChassisWheelRedundancyInput_s input = Input(.6f, -.2f, 1.1f);
    assert(ChassisEstimateVelocityWithWheelRedundancy(&kinematics, &config,
        &input, wheels, &output));
    assert(output.valid && !output.corrected && output.reason == CHASSIS_WHEEL_REDUNDANCY_NO_OUTLIER);
    Near(output.selected_vx_mps, .6f);
    Near(output.selected_vy_mps, -.2f);
    Near(output.selected_wz_radps, 1.1f);
    assert(output.used_wheel_mask == 15);
}

static void OneWheelSlipUsesTheOtherThree(void)
{
    float wheels[4];
    WheelsFor(.6f, -.2f, 1.1f, wheels);
    wheels[2] += .8f;
    ChassisWheelRedundancyInput_s input = Input(.6f, -.2f, 1.1f);
    ChassisWheelRedundancyOutput_s output;
    assert(ChassisEstimateVelocityWithWheelRedundancy(&kinematics, &config,
        &input, wheels, &output));
    assert(output.valid && output.corrected);
    assert(output.reason == CHASSIS_WHEEL_REDUNDANCY_CORRECTED);
    assert(output.excluded_wheel == 2 && output.used_wheel_mask == 11);
    Near(output.selected_vx_mps, .6f);
    Near(output.selected_vy_mps, -.2f);
    Near(output.selected_wz_radps, 1.1f);
    assert(output.wheel_residual_mps[2] > .7f);
}

static void MissingReferenceDoesNotGuess(void)
{
    float wheels[4];
    WheelsFor(.4f, .3f, -.7f, wheels);
    wheels[0] += 1.0f;
    ChassisWheelRedundancyInput_s input = {0};
    input.valid = true;
    input.online_mask = 15;
    ChassisWheelRedundancyOutput_s output;
    assert(ChassisEstimateVelocityWithWheelRedundancy(&kinematics, &config,
        &input, wheels, &output));
    assert(output.valid && !output.corrected);
    assert(output.reason == CHASSIS_WHEEL_REDUNDANCY_NO_REFERENCE);
    assert(output.used_wheel_mask == 15);
}

static void AmbiguousResidualDoesNotDropAHealthyWheel(void)
{
    float wheels[4];
    WheelsFor(.5f, .1f, .4f, wheels);
    wheels[0] += .45f;
    wheels[1] -= .40f;
    ChassisWheelRedundancyInput_s input = Input(.5f, .1f, .4f);
    ChassisWheelRedundancyOutput_s output;
    assert(ChassisEstimateVelocityWithWheelRedundancy(&kinematics, &config,
        &input, wheels, &output));
    assert(output.valid && !output.corrected);
    assert(output.reason == CHASSIS_WHEEL_REDUNDANCY_AMBIGUOUS);
}

static void InvalidInputFailsClosed(void)
{
    float wheels[4];
    WheelsFor(0, 0, 0, wheels);
    ChassisWheelRedundancyOutput_s output;
    ChassisWheelRedundancyInput_s input = Input(0, 0, 0);
    input.online_mask = 7;
    assert(!ChassisEstimateVelocityWithWheelRedundancy(&kinematics, &config,
        &input, wheels, &output));
    assert(!output.valid && output.reason == CHASSIS_WHEEL_REDUNDANCY_INVALID);
    input = Input(0, 0, 0);
    input.valid = false;
    assert(!ChassisEstimateVelocityWithWheelRedundancy(&kinematics, &config,
        &input, wheels, &output));
    assert(!output.valid && output.reason == CHASSIS_WHEEL_REDUNDANCY_INVALID);
}

int main(void)
{
    NoSlipKeepsAllFour();
    OneWheelSlipUsesTheOtherThree();
    MissingReferenceDoesNotGuess();
    AmbiguousResidualDoesNotDropAHealthyWheel();
    InvalidInputFailsClosed();
    puts("PASS: wheel redundancy estimator");
    return 0;
}
