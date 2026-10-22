#ifndef MPU6500_MATH_H
#define MPU6500_MATH_H
#include <stdbool.h>
#include <stdint.h>
float MPU6500Temperature(uint8_t high, uint8_t low);
bool MPU6500InitialQuaternion(const float acceleration[3], float quaternion[4]);
void MPU6500ExportSample(const float gyro_rad[3], const float ekf_ypr[3],
                         float gyro_deg[3], float vehicle_ypr[3]);
#endif
