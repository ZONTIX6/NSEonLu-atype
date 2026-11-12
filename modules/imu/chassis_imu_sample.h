#ifndef CHASSIS_IMU_SAMPLE_H
#define CHASSIS_IMU_SAMPLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHASSIS_IMU_CAN_ASSEMBLY_MAX_MS 10U

/* Names retained for the existing fusion and telemetry interfaces. The
 * dedicated IMU supplies no source timestamp: timing is estimated locally. */
typedef struct {
    float quaternion[4]; /* Unit [w,x,y,z], sensor to navigation frame. */
    float accel_n_mps2[3], gyro_sensor_radps[3];
    uint32_t sequence, tick_ms; /* Locally generated sequence / first receive tick. */
    uint16_t source_age_ms; /* DM: 0; sensor-internal latency is unknown. */
    bool valid;
} ChassisImuCanSample_s;

typedef struct {
    ChassisImuCanSample_s sample;
    uint32_t first_rx_tick_ms, last_rx_tick_ms; /* Both use the receiver clock. */
    uint32_t rejected_groups;
    bool available; /* A complete sample exists; freshness is checked by its consumer. */
} ChassisImuCanSnapshot_s;

#endif
