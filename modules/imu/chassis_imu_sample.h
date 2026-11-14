#ifndef CHASSIS_IMU_SAMPLE_H
#define CHASSIS_IMU_SAMPLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHASSIS_IMU_CAN_ASSEMBLY_MAX_MS 10U

/* The dedicated A board supplies its own sample identity and source-local age.
 * Source and receiver clocks have independent epochs. */
typedef struct {
    float quaternion[4]; /* Unit [w,x,y,z], sensor to source-board navigation frame. */
    float accel_n_mps2[3], gyro_sensor_radps[3];
    uint32_t sequence, tick_ms; /* Source clock; never subtract from a receiver clock. */
    uint16_t source_age_ms; /* Source-local age when this transfer was prepared. */
    bool valid;
} ChassisImuCanSample_s;

typedef struct {
    ChassisImuCanSample_s sample;
    uint32_t first_rx_tick_ms, last_rx_tick_ms; /* Both use the receiver clock. */
    uint32_t rejected_groups;
    bool available; /* A complete packet exists, even if sample.valid is false. */
} ChassisImuCanSnapshot_s;

#endif
