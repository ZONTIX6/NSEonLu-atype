#include "chassis_wheel_redundancy.h"

#include <math.h>
#include <stddef.h>

#define TWO_PI 6.2831853071795864769f
#define INV_SQRT_TWO 0.7071067811865475244f
#define NO_WHEEL 0xffU

typedef struct {
    float vx_mps, vy_mps, wz_radps;
} BodyVelocity_s;

static bool FiniteBody(const BodyVelocity_s *body)
{
    return body && isfinite(body->vx_mps) && isfinite(body->vy_mps) &&
        isfinite(body->wz_radps);
}

static bool ConfigValid(const ChassisVelocityConfig_s *kinematics,
    const ChassisWheelRedundancyConfig_s *config)
{
    if (!kinematics || !config || !config->enabled || !kinematics->omni_x ||
        !isfinite(kinematics->wheel_base_m) || kinematics->wheel_base_m <= 0.0f ||
        !isfinite(kinematics->track_width_m) || kinematics->track_width_m <= 0.0f ||
        !isfinite(config->residual_threshold_mps) || config->residual_threshold_mps <= 0.0f ||
        !isfinite(config->dominance_ratio) || config->dominance_ratio <= 1.0f ||
        !isfinite(config->max_reference_error_mps) || config->max_reference_error_mps <= 0.0f)
        return false;
    return true;
}

static bool BuildMatrix(const ChassisVelocityConfig_s *kinematics,
    float matrix[CHASSIS_WHEEL_COUNT][3])
{
    if (!kinematics || !kinematics->omni_x) return false;
    const float lever = (kinematics->wheel_base_m + kinematics->track_width_m) * .5f;
    if (!isfinite(lever) || lever <= 0.0f) return false;
    const float k = INV_SQRT_TWO;
    matrix[0][0] = -k; matrix[0][1] = k;  matrix[0][2] = k * lever;
    matrix[1][0] = k;  matrix[1][1] = k;  matrix[1][2] = k * lever;
    matrix[2][0] = -k; matrix[2][1] = -k; matrix[2][2] = k * lever;
    matrix[3][0] = k;  matrix[3][1] = -k; matrix[3][2] = k * lever;
    return true;
}

static bool SolveThree(float matrix[CHASSIS_WHEEL_COUNT][3],
    const float wheel[CHASSIS_WHEEL_COUNT], unsigned excluded, BodyVelocity_s *body)
{
    float a[3][4];
    unsigned row = 0;
    for (unsigned source = 0; source < CHASSIS_WHEEL_COUNT; ++source) {
        if (source == excluded) continue;
        for (unsigned column = 0; column < 3; ++column)
            a[row][column] = matrix[source][column];
        a[row][3] = wheel[source];
        ++row;
    }
    for (unsigned pivot = 0; pivot < 3; ++pivot) {
        unsigned best = pivot;
        for (unsigned candidate = pivot + 1; candidate < 3; ++candidate)
            if (fabsf(a[candidate][pivot]) > fabsf(a[best][pivot])) best = candidate;
        if (!isfinite(a[best][pivot]) || fabsf(a[best][pivot]) < 1e-5f) return false;
        if (best != pivot) {
            for (unsigned column = pivot; column < 4; ++column) {
                const float value = a[pivot][column];
                a[pivot][column] = a[best][column];
                a[best][column] = value;
            }
        }
        for (unsigned candidate = pivot + 1; candidate < 3; ++candidate) {
            const float factor = a[candidate][pivot] / a[pivot][pivot];
            for (unsigned column = pivot; column < 4; ++column)
                a[candidate][column] -= factor * a[pivot][column];
        }
    }
    float x[3] = {0};
    for (int pivot = 2; pivot >= 0; --pivot) {
        float value = a[pivot][3];
        for (unsigned column = (unsigned)pivot + 1; column < 3; ++column)
            value -= a[pivot][column] * x[column];
        x[pivot] = value / a[pivot][pivot];
    }
    body->vx_mps = x[0];
    body->vy_mps = x[1];
    body->wz_radps = x[2];
    return FiniteBody(body);
}

static BodyVelocity_s SolveAll(float matrix[CHASSIS_WHEEL_COUNT][3],
    const float wheel[CHASSIS_WHEEL_COUNT])
{
    BodyVelocity_s body = {0};
    /* The Omni-X rows are orthogonal, so this is the exact four-wheel LS fit. */
    for (unsigned wheel_index = 0; wheel_index < CHASSIS_WHEEL_COUNT; ++wheel_index) {
        body.vx_mps += matrix[wheel_index][0] * wheel[wheel_index];
        body.vy_mps += matrix[wheel_index][1] * wheel[wheel_index];
        body.wz_radps += matrix[wheel_index][2] * wheel[wheel_index];
    }
    body.vx_mps *= .5f;
    body.vy_mps *= .5f;
    const float lever = matrix[0][2];
    body.wz_radps /= 4.0f * lever * lever;
    return body;
}

static float PredictedWheel(const float matrix[3], const BodyVelocity_s *body)
{
    return matrix[0] * body->vx_mps + matrix[1] * body->vy_mps +
        matrix[2] * body->wz_radps;
}

static float BodyDistance(const BodyVelocity_s *a, const BodyVelocity_s *b,
    float lever)
{
    const float dx = a->vx_mps - b->vx_mps;
    const float dy = a->vy_mps - b->vy_mps;
    const float dw = (a->wz_radps - b->wz_radps) * lever;
    return hypotf(hypotf(dx, dy), dw);
}

bool ChassisEstimateVelocityWithWheelRedundancy(
    const ChassisVelocityConfig_s *kinematics,
    const ChassisWheelRedundancyConfig_s *config,
    const ChassisWheelRedundancyInput_s *input,
    const float wheel_mps[CHASSIS_WHEEL_COUNT],
    ChassisWheelRedundancyOutput_s *output)
{
    if (!output) return false;
    *output = (ChassisWheelRedundancyOutput_s){.excluded_wheel = NO_WHEEL,
        .reason = CHASSIS_WHEEL_REDUNDANCY_INVALID};
    if (!kinematics || !config || !input || !input->valid || !wheel_mps ||
        (input->online_mask & ((1U << CHASSIS_WHEEL_COUNT) - 1U)) !=
            ((1U << CHASSIS_WHEEL_COUNT) - 1U) ||
        !ConfigValid(kinematics, config))
        return false;

    float matrix[CHASSIS_WHEEL_COUNT][3];
    float signed_wheel[CHASSIS_WHEEL_COUNT];
    if (!BuildMatrix(kinematics, matrix)) return false;
    for (unsigned wheel = 0; wheel < CHASSIS_WHEEL_COUNT; ++wheel) {
        if (!isfinite(wheel_mps[wheel])) return false;
        signed_wheel[wheel] = wheel_mps[wheel];
    }
    const BodyVelocity_s all = SolveAll(matrix, signed_wheel);
    if (!FiniteBody(&all)) return false;
    output->valid = true;
    output->all_vx_mps = output->selected_vx_mps = all.vx_mps;
    output->all_vy_mps = output->selected_vy_mps = all.vy_mps;
    output->all_wz_radps = output->selected_wz_radps = all.wz_radps;
    output->used_wheel_mask = (uint8_t)((1U << CHASSIS_WHEEL_COUNT) - 1U);
    if (!input->reference_valid || !isfinite(input->reference_vx_mps) ||
        !isfinite(input->reference_vy_mps) || !isfinite(input->reference_wz_radps)) {
        output->reason = CHASSIS_WHEEL_REDUNDANCY_NO_REFERENCE;
        return true;
    }

    const BodyVelocity_s reference = {.vx_mps = input->reference_vx_mps,
        .vy_mps = input->reference_vy_mps, .wz_radps = input->reference_wz_radps};
    const float lever = (kinematics->wheel_base_m + kinematics->track_width_m) * .5f;
    unsigned largest = 0;
    float second = 0.0f;
    for (unsigned wheel = 0; wheel < CHASSIS_WHEEL_COUNT; ++wheel) {
        const float expected = PredictedWheel(matrix[wheel], &reference);
        output->wheel_residual_mps[wheel] = fabsf(signed_wheel[wheel] - expected);
        if (output->wheel_residual_mps[wheel] > output->wheel_residual_mps[largest]) {
            second = output->wheel_residual_mps[largest];
            largest = wheel;
        } else if (output->wheel_residual_mps[wheel] > second && wheel != largest) {
            second = output->wheel_residual_mps[wheel];
        }
    }
    if (output->wheel_residual_mps[largest] < config->residual_threshold_mps) {
        output->reason = CHASSIS_WHEEL_REDUNDANCY_NO_OUTLIER;
        return true;
    }
    if (output->wheel_residual_mps[largest] < second * config->dominance_ratio) {
        output->reason = CHASSIS_WHEEL_REDUNDANCY_AMBIGUOUS;
        return true;
    }

    BodyVelocity_s selected;
    if (!SolveThree(matrix, signed_wheel, largest, &selected)) return false;
    output->selected_reference_error_mps = BodyDistance(&selected, &reference, lever);
    if (output->selected_reference_error_mps > config->max_reference_error_mps) {
        output->reason = CHASSIS_WHEEL_REDUNDANCY_AMBIGUOUS;
        return true;
    }
    output->corrected = true;
    output->excluded_wheel = (uint8_t)largest;
    output->used_wheel_mask = (uint8_t)(((1U << CHASSIS_WHEEL_COUNT) - 1U) & ~(1U << largest));
    output->selected_vx_mps = selected.vx_mps;
    output->selected_vy_mps = selected.vy_mps;
    output->selected_wz_radps = selected.wz_radps;
    output->reason = CHASSIS_WHEEL_REDUNDANCY_CORRECTED;
    return true;
}
