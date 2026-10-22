#ifndef _MPU6500_H_
#define _MPU6500_H_
#include "bsp_dwt.h"
#include "ins_task.h"
#define MPU_DELAY(x) DWT_Delay_ms(x)

#define GxOFFSET 0.037452f
#define GyOFFSET 0.003360f
#define GzOFFSET 0.008532f
#define gNORM 9.605101f
typedef struct
{
	float Accel[3];

    float Gyro[3];

    float TempWhenCali;
    float Temperature;

    float AccelScale;
    float GyroOffset[3];

    float gNorm;
} mpu_data_t;

attitude_t *INS_Init(void);

void INS_Task(void);

#endif
