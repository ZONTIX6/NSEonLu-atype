#ifndef IMU_SAMPLE_H
#define IMU_SAMPLE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    float accel_m_s2[3], gyro_rad_s[3], temperature_c;
    uint32_t tick_ms;
    bool valid;
} IMUSample_s;
#endif
