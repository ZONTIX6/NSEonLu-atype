#include "vofa_telemetry.h"
#include "imu_capture_config.h"
#if CHASSIS_IMU_CAN_RX_ENABLE
#include "chassis_imu_can.h"
#endif
#if CHASSIS_REMOTE_IMU_FUSION_ENABLE
#include "../../config/30_velocity_observation/chassis_remote_imu_config.h"
#endif

#if VOFA_TELEMETRY_ENABLE
#include "usart.h"
#if VOFA_TELEMETRY_MODE == 0
#include <string.h>

/* STM32F427 stores IEEE 754 floats in JustFloat's little-endian wire order. */
static struct {
    float channels[19];
    uint8_t tail[4];
} frame = {.tail = {0x00, 0x00, 0x80, 0x7f}};
_Static_assert(sizeof(float) == 4 && sizeof(frame) == 80, "JustFloat frame layout");

void VofaTelemetrySend(const USBTelemetrySample_s *sample)
{
    UART_HandleTypeDef *uart = BOARD_VOFA_UART;
    /* The IRQ reads this buffer until TX completion. Only this task starts TX. */
    if (uart->gState != HAL_UART_STATE_READY) return;

    const uint32_t motor_mask = sample->motor_online_mask & 0x0fU;
    const int32_t actual_rpm[] = {sample->rpm_lf, sample->rpm_rf, sample->rpm_lb, sample->rpm_rb};
    const int32_t target_rpm[] = {sample->target_lf_rpm, sample->target_rf_rpm,
                                  sample->target_lb_rpm, sample->target_rb_rpm};
    float *channels = frame.channels;
    for (unsigned wheel = 0; wheel < 4; ++wheel) {
        const bool online = (motor_mask & (1U << wheel)) != 0;
        channels[wheel] = online ? (float)actual_rpm[wheel] : 0.0f;
        channels[4 + wheel] = online ? (float)target_rpm[wheel] : 0.0f;
    }
    channels[8] = sample->meter_online ? sample->p_meter_cw * 0.01f : 0.0f;
    channels[9] = sample->power_valid ? sample->p_limited_cw * 0.01f : 0.0f;
    channels[10] = sample->power_valid ? sample->effective_limit_cw * 0.01f : 0.0f;
    channels[11] = sample->cap_online ? sample->cap_chassis_cw * 0.01f : 0.0f;
    channels[12] = sample->cap_input_online ? sample->cap_input_cw * 0.01f : 0.0f;
    channels[13] = sample->cap_online ? sample->cap_energy_pm * 0.1f : 0.0f;
    channels[14] = sample->imu_valid ? sample->imu_yaw_mdeg * 0.001f : 0.0f;
    channels[15] = sample->imu_valid ? sample->imu_pitch_mdeg * 0.001f : 0.0f;
    channels[16] = sample->imu_valid ? sample->imu_roll_mdeg * 0.001f : 0.0f;
    channels[17] = (float)motor_mask;
    channels[18] = (float)((sample->power_valid ? 1U : 0U) |
                           (sample->meter_online ? 2U : 0U) |
                           (sample->cap_online ? 4U : 0U) |
                           (sample->cap_input_online ? 8U : 0U) |
                           (sample->imu_valid ? 16U : 0U));

    /* JustFloat scans for the delimiter even across adjacent float boundaries.
     * In a collision, flip the lowest mantissa bit of the float ending the match. */
    uint8_t *bytes = (uint8_t *)&frame;
    for (size_t i = 0; i + sizeof(frame.tail) <= sizeof(frame.channels); ++i) {
        if (memcmp(bytes + i, frame.tail, sizeof(frame.tail)) == 0) {
            const size_t last = i + sizeof(frame.tail) - 1;
            bytes[last - last % sizeof(float)] ^= 1U;
        }
    }

    (void)HAL_UART_Transmit_IT(uart, bytes, sizeof(frame));
}
#else
#include <math.h>
#if VOFA_TELEMETRY_MODE == 1
#include <stdio.h>
#else
#include <string.h>
#endif

enum {
    MEASUREMENT_PERIOD_MS = VOFA_TELEMETRY_MODE == 2 ? 40 : 80,
    MEASUREMENT_MAX_AGE_MS = 20,
    MEASUREMENT_FUSED_MAX_AGE_MS = 60,
    MEASUREMENT_CHANNELS = CHASSIS_IMU_CAN_RX_ENABLE ? 72 : 64,
    // An int32 takes at most 11 characters plus one delimiter.
    MEASUREMENT_ROW_CAPACITY = MEASUREMENT_CHANNELS * 12 + 1
};

/* Only the telemetry task writes this buffer, while TX is idle. */
#if VOFA_TELEMETRY_MODE == 1
static char row[MEASUREMENT_ROW_CAPACITY];
_Static_assert((sizeof row - 1) * 10U * 1000U < 115200U * MEASUREMENT_PERIOD_MS,
               "FireWater row must fit in one send interval");
#else
/* STM32F427 uses the little-endian IEEE 754 float32 wire representation. */
static struct {
    float channels[MEASUREMENT_CHANNELS];
    uint8_t tail[4];
} measurement_frame = {.tail = {0x00, 0x00, 0x80, 0x7f}};
_Static_assert(sizeof(float) == 4 && sizeof(measurement_frame) == (MEASUREMENT_CHANNELS + 1) * 4,
               "JustFloat measurement frame layout");
_Static_assert(sizeof(measurement_frame) * 10U * 1000U < 115200U * MEASUREMENT_PERIOD_MS,
               "JustFloat measurement frame must fit in one send interval");
#endif

static int32_t SampleAge(uint32_t now_ms, uint32_t tick_ms, bool known)
{
    const uint32_t age = now_ms - tick_ms;
    return known && age <= INT32_MAX ? (int32_t)age : -1;
}

static bool FiniteVector(const float *values, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
        if (!isfinite(values[i])) return false;
    return true;
}

void VofaTelemetrySendMeasurement(const USBTelemetrySample_s *sample,
                                  const ChassisPowerSnapshot_s *cycle,
                                  const ChassisFusedVelocitySnapshot_s *velocity, uint32_t now_ms)
{
    static bool started;
    static uint32_t last_attempt_ms, next_sequence;
    if (started && (uint32_t)(now_ms - last_attempt_ms) < MEASUREMENT_PERIOD_MS) return;
    started = true;
    last_attempt_ms = now_ms;
    const uint32_t sequence = next_sequence++;

    UART_HandleTypeDef *uart = BOARD_VOFA_UART;
    if (uart->gState != HAL_UART_STATE_READY) return;

    const bool final_valid = cycle && cycle->registered && cycle->final_valid &&
                             (uint32_t)(now_ms - cycle->tick_ms) <= MEASUREMENT_MAX_AGE_MS;
    const bool power_valid = final_valid && sample->power_valid;
    const bool meter_known = cycle && cycle->registered && cycle->final_valid &&
        (cycle->meter_online || cycle->meter_sequence || cycle->meter_tick_ms);
    const bool meter_valid = final_valid && meter_known && cycle->meter_online &&
        (uint32_t)(now_ms - cycle->meter_tick_ms) <= MEASUREMENT_MAX_AGE_MS &&
        isfinite(cycle->measured_power_w);

    static const ChassisFusedVelocitySnapshot_s empty_velocity;
#if !IMU_CAPTURE_ONLY
    const bool observer_enabled = velocity != NULL;
#endif
    if (!velocity) velocity = &empty_velocity;
    const ChassisVelocityDiagnostics_s *d = &velocity->diagnostics;
    const bool observed = d->available &&
        (uint32_t)(now_ms - d->tick_ms) <= MEASUREMENT_MAX_AGE_MS;
    uint32_t online = 0;
    if (observed) {
        online = d->wheel_online_mask & 15U;
        for (unsigned wheel = 0; wheel < 4; ++wheel) {
            if ((uint32_t)(now_ms - d->wheel_tick_ms[wheel]) > MEASUREMENT_MAX_AGE_MS ||
                !isfinite(d->raw_rpm[wheel]))
                online &= ~(1U << wheel);
        }
    }
    const bool wheel_valid = online == 15U && velocity->wheel.valid &&
        (uint32_t)(now_ms - velocity->wheel.tick_ms) <= MEASUREMENT_MAX_AGE_MS &&
        isfinite(velocity->wheel.vx_mps) && isfinite(velocity->wheel.vy_mps) &&
        isfinite(velocity->wheel.wz_rpm);
    const bool estimate_valid = wheel_valid && velocity->valid &&
        (uint32_t)(now_ms - velocity->tick_ms) <=
            (velocity->status == CHASSIS_VELOCITY_FUSED ? MEASUREMENT_FUSED_MAX_AGE_MS : MEASUREMENT_MAX_AGE_MS) &&
        isfinite(velocity->vx_mps) && isfinite(velocity->vy_mps);
    const bool fused = estimate_valid && velocity->status == CHASSIS_VELOCITY_FUSED &&
        FiniteVector(velocity->paired_wheel_mps, 2) && FiniteVector(velocity->center_accel_n_mps2, 2);
    const float *imu_quaternion = d->imu_quaternion;
    const float *imu_accel = d->imu_accel_n_mps2;
    const float *imu_gyro = d->imu_gyro_sensor_radps;
    uint32_t imu_sequence = d->available ? d->imu_sequence : 0U;
    int32_t imu_age = SampleAge(now_ms, d->imu_tick_ms, d->available && (d->imu_valid || d->imu_sequence));
#if CHASSIS_IMU_CAN_RX_ENABLE
    static ChassisImuCanSnapshot_s remote;
    const bool remote_usable = ChassisImuCanReadSnapshot(&remote, now_ms);
    imu_quaternion = remote.sample.quaternion;
    imu_accel = remote.sample.accel_n_mps2;
    imu_gyro = remote.sample.gyro_sensor_radps;
    imu_sequence = remote.available ? remote.sample.sequence : 0U;
    /* Known age only: sender sample age plus time since the first frame reached
     * this MCU. This is not a synchronized sensor timestamp or total latency. */
    const uint64_t known_age = (uint64_t)(uint32_t)(now_ms - remote.first_rx_tick_ms) +
                                remote.sample.source_age_ms;
    imu_age = remote.available && remote.sample.source_age_ms != UINT16_MAX && known_age <= INT32_MAX ?
        (int32_t)known_age : -1;
    const bool imu_valid = remote_usable && FiniteVector(imu_quaternion, 4) &&
        FiniteVector(imu_accel, 3) && FiniteVector(imu_gyro, 3);
#else
    const bool imu_valid = observed && d->imu_valid &&
        (uint32_t)(now_ms - d->imu_tick_ms) <= MEASUREMENT_MAX_AGE_MS &&
        FiniteVector(imu_quaternion, 4) && FiniteVector(imu_accel, 3) && FiniteVector(imu_gyro, 3);
#endif
    const bool joints_valid = observed && d->joints_valid &&
        d->joint_ecd[0] < 8192U && d->joint_ecd[1] < 8192U &&
        (uint32_t)(now_ms - d->joint_tick_ms[0]) <= MEASUREMENT_MAX_AGE_MS &&
        (uint32_t)(now_ms - d->joint_tick_ms[1]) <= MEASUREMENT_MAX_AGE_MS;
    const bool chassis_valid = sample->chassis_valid &&
        (uint32_t)(now_ms - sample->chassis_tick_ms) <= MEASUREMENT_MAX_AGE_MS;
    const bool target_valid = chassis_valid && sample->velocity_target_valid &&
        (uint32_t)(now_ms - sample->velocity_target_tick_ms) <= MEASUREMENT_MAX_AGE_MS;
    const uint32_t valid = (observed ? 1U : 0U) | (wheel_valid ? 2U : 0U) |
        (estimate_valid ? 4U : 0U) | (fused ? 8U : 0U) | (imu_valid ? 16U : 0U) |
        (joints_valid ? 32U : 0U) | (chassis_valid ? 64U : 0U) | (power_valid ? 128U : 0U) |
        (final_valid ? 256U : 0U) | (final_valid && cycle->cycle_valid ? 512U : 0U) |
        (final_valid && cycle->can_submitted ? 1024U : 0U) | (target_valid ? 2048U : 0U) |
        (meter_valid ? 4096U : 0U) | (sample->power_source == 1U ? 8192U : 0U)
#if IMU_CAPTURE_ONLY
        | 16384U
#endif
#if CHASSIS_IMU_CAN_RX_ENABLE
        | 32768U
        | 524288U /* DM-IMU-L1, receiver-generated sequence and estimated timestamp. */
#endif
#if CHASSIS_REMOTE_IMU_FUSION_ENABLE
        /* Mode/provenance bits, not sample validity or precise synchronization. */
        | 65536U | 131072U
#if CHASSIS_REMOTE_IMU_CONFIG_VERIFIED
        | 262144U
#endif
#endif
        ;

    /* Fixed-point units and order are documented in docs/vofa-telemetry.md. */
    const int32_t fields[] = {
        CHASSIS_IMU_CAN_RX_ENABLE ? 3 : 2,
        now_ms >> 16, now_ms & UINT16_MAX, sequence >> 16, sequence & UINT16_MAX,
        valid,
#if IMU_CAPTURE_ONLY
        -2,
#else
        observer_enabled ? (int32_t)velocity->status : -1,
#endif
        chassis_valid ? (int32_t)sample->chassis_mode : -1,
        online, final_valid ? sample->motor_enabled_mask & 15U : 0U,
        SampleAge(now_ms, d->tick_ms, d->available),
        SampleAge(now_ms, velocity->tick_ms, velocity->valid),
        SampleAge(now_ms, velocity->wheel.tick_ms, velocity->wheel.valid),
        online & 1U ? USBTelemetryFixed(d->raw_rpm[0], 1) : 0,
        online & 2U ? USBTelemetryFixed(d->raw_rpm[1], 1) : 0,
        online & 4U ? USBTelemetryFixed(d->raw_rpm[2], 1) : 0,
        online & 8U ? USBTelemetryFixed(d->raw_rpm[3], 1) : 0,
        SampleAge(now_ms, d->wheel_tick_ms[0], d->available && (d->wheel_online_mask & 1U)),
        SampleAge(now_ms, d->wheel_tick_ms[1], d->available && (d->wheel_online_mask & 2U)),
        SampleAge(now_ms, d->wheel_tick_ms[2], d->available && (d->wheel_online_mask & 4U)),
        SampleAge(now_ms, d->wheel_tick_ms[3], d->available && (d->wheel_online_mask & 8U)),
        wheel_valid ? USBTelemetryFixed(velocity->wheel.vx_mps, 1000) : 0,
        wheel_valid ? USBTelemetryFixed(velocity->wheel.vy_mps, 1000) : 0,
        wheel_valid ? USBTelemetryFixed(velocity->wheel.wz_rpm, 1000.0f / 60.0f) : 0,
        target_valid ? sample->velocity_target_vx_mmps : 0,
        target_valid ? sample->velocity_target_vy_mmps : 0,
        target_valid ? sample->velocity_target_wz_mrps : 0,
        estimate_valid ? USBTelemetryFixed(velocity->vx_mps, 1000) : 0,
        estimate_valid ? USBTelemetryFixed(velocity->vy_mps, 1000) : 0,
        fused ? USBTelemetryFixed(velocity->paired_wheel_mps[0], 1000) : 0,
        fused ? USBTelemetryFixed(velocity->paired_wheel_mps[1], 1000) : 0,
        fused ? USBTelemetryFixed(velocity->center_accel_n_mps2[0], 1000) : 0,
        fused ? USBTelemetryFixed(velocity->center_accel_n_mps2[1], 1000) : 0,
        imu_age, imu_sequence >> 16, imu_sequence & UINT16_MAX,
        imu_valid ? USBTelemetryFixed(imu_quaternion[0], 1000000) : 0,
        imu_valid ? USBTelemetryFixed(imu_quaternion[1], 1000000) : 0,
        imu_valid ? USBTelemetryFixed(imu_quaternion[2], 1000000) : 0,
        imu_valid ? USBTelemetryFixed(imu_quaternion[3], 1000000) : 0,
        imu_valid ? USBTelemetryFixed(imu_accel[0], 1000) : 0,
        imu_valid ? USBTelemetryFixed(imu_accel[1], 1000) : 0,
        imu_valid ? USBTelemetryFixed(imu_accel[2], 1000) : 0,
        imu_valid ? USBTelemetryFixed(imu_gyro[0], 1000) : 0,
        imu_valid ? USBTelemetryFixed(imu_gyro[1], 1000) : 0,
        imu_valid ? USBTelemetryFixed(imu_gyro[2], 1000) : 0,
        joints_valid ? d->joint_ecd[0] : 0, joints_valid ? d->joint_ecd[1] : 0,
        joints_valid ? d->joint_rpm[0] : 0, joints_valid ? d->joint_rpm[1] : 0,
        SampleAge(now_ms, d->joint_tick_ms[0], d->available && d->joints_valid),
        SampleAge(now_ms, d->joint_tick_ms[1], d->available && d->joints_valid),
        final_valid ? cycle->final_current[0] : 0, final_valid ? cycle->final_current[1] : 0,
        final_valid ? cycle->final_current[2] : 0, final_valid ? cycle->final_current[3] : 0,
        power_valid ? sample->p_unlimited_cw : 0, power_valid ? sample->p_limited_cw : 0,
        power_valid ? sample->effective_limit_cw : 0,
        meter_valid ? USBTelemetryFixed(cycle->measured_power_w, 100) : 0,
        SampleAge(now_ms, cycle ? cycle->tick_ms : 0, cycle && cycle->registered && cycle->final_valid),
        SampleAge(now_ms, sample->velocity_target_tick_ms, sample->velocity_target_valid),
        SampleAge(now_ms, cycle ? cycle->meter_tick_ms : 0, meter_known),
#if IMU_CAPTURE_ONLY
        1
#else
        0
#endif
#if CHASSIS_IMU_CAN_RX_ENABLE
        , 2, SampleAge(now_ms, remote.last_rx_tick_ms, remote.available),
        remote.available && remote.sample.source_age_ms != UINT16_MAX ? remote.sample.source_age_ms : -1,
        remote.available ? remote.sample.tick_ms >> 16 : 0U,
        remote.available ? remote.sample.tick_ms & UINT16_MAX : 0U,
        SampleAge(remote.last_rx_tick_ms, remote.first_rx_tick_ms, remote.available),
        remote.rejected_groups >> 16, remote.rejected_groups & UINT16_MAX
#endif
    };
    _Static_assert(sizeof fields / sizeof fields[0] == MEASUREMENT_CHANNELS,
                   "Measurement channel count must match the field layout");

#if VOFA_TELEMETRY_MODE == 1
    size_t used = 0;
    for (size_t i = 0; i < MEASUREMENT_CHANNELS; ++i) {
        const int written = snprintf(row + used, sizeof row - used, "%ld,", (long)fields[i]);
        if (written <= 0 || (size_t)written >= sizeof row - used) return;
        used += (size_t)written;
    }
    row[used - 1] = '\n';
    (void)HAL_UART_Transmit_IT(uart, (uint8_t *)row, (uint16_t)used);
#else
    for (size_t i = 0; i < MEASUREMENT_CHANNELS; ++i)
        measurement_frame.channels[i] = (float)fields[i];

    /* As in the 19-channel mode, avoid false delimiters spanning floats.
     * Consumers round integer-semantic metadata before decoding it. */
    uint8_t *bytes = (uint8_t *)&measurement_frame;
    for (size_t i = 0; i + sizeof(measurement_frame.tail) <= sizeof(measurement_frame.channels); ++i) {
        if (memcmp(bytes + i, measurement_frame.tail, sizeof(measurement_frame.tail)) == 0) {
            const size_t last = i + sizeof(measurement_frame.tail) - 1;
            bytes[last - last % sizeof(float)] ^= 1U;
        }
    }
    (void)HAL_UART_Transmit_IT(uart, bytes, sizeof(measurement_frame));
#endif
}
#endif
#endif
