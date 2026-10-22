#ifndef MPU6500Driver_H
#define MPU6500Driver_H
#include "mpu6500_reg.h"
#include "ist8310_reg.h"
#include "bsp_spi.h"
#include "imu_sample.h"
#include "bsp_dwt.h"
#define MPU_DELAY(x) DWT_Delay_ms(x)

typedef struct MPU6500Instance MPU6500Instance;
typedef struct { SPI_Init_Config_s spi_config; uint32_t timeout_us; } MPU6500_Init_Config_s;
MPU6500Instance *MPU6500Register(const MPU6500_Init_Config_s *config);
bool MPU6500Read(MPU6500Instance *instance, IMUSample_s *sample);

#define MPU6500_ACCEL_2G_SEN 0.00059814453125f
#define MPU6500_ACCEL_4G_SEN 0.0011962890625f
#define MPU6500_ACCEL_8G_SEN 0.002392578125f
#define MPU6500_ACCEL_16G_SEN 0.00478515625f

#define MPU6500_GYRO_2000_SEN 0.00106422513550135501355013550136
#define MPU6500_GYRO_1000_SEN 5.3211256775067750677506775067751e-4
#define MPU6500_GYRO_500_SEN 2.6646247667514843087362171331637e-4
#define MPU6500_GYRO_250_SEN 1.3323123833757421543681085665818e-4

#endif // !MPU6500Driver_H
