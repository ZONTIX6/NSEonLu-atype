#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Compile the actual initialization and task, with only hardware/EKF boundaries stubbed. */
#include "../../modules/mpu6500/mpu6500.c"

TestEKF QEKF_INS;
static unsigned reads, fail_at, ekf_inits, ekf_updates;
static float timeline, heater;
float DWT_GetDeltaT(uint32_t *count) {(void)count;return .001f;}
float DWT_GetTimeline_s(void) {return timeline;}
void DWT_Delay(float seconds) {timeline+=seconds;}
uint32_t HAL_GetTick(void) {return (uint32_t)(timeline*1000);}
void mpu6500_device_init(void) {}
uint8_t mpu_read_byte(uint8_t reg) {(void)reg;return 0x10;}
uint8_t mpu_read_bytes(uint8_t reg,uint8_t *out,uint8_t len)
{
    (void)reg;
    if(++reads==fail_at) return 1;
    assert(len==14);memset(out,0,len);
    out[4]=0x10; /* 4096 * 8g sensitivity = 9.8 m/s^2 */
    out[9]=10;out[11]=20;out[13]=30;
    return 0;
}
void IMU_Temperature_Init(void) {}
void IMU_Temperature_Update(float duty) {heater=duty;}
void PIDInit(PIDInstance *pid,PID_Init_Config_s *config) {(void)config;pid->Output=10;}
float PIDCalculate(PIDInstance *pid,float measured,float target)
{(void)measured;(void)target;return pid->Output;}
int SEGGER_RTT_printf(unsigned channel,const char *format,...) {(void)channel;(void)format;return 0;}
void VisionSetAltitude(float yaw,float pitch,float roll) {(void)yaw;(void)pitch;(void)roll;}
void IMU_QuaternionEKF_Init(float *q,float a,float b,float c,float d,float e)
{(void)a;(void)b;(void)c;(void)d;(void)e;++ekf_inits;memcpy(QEKF_INS.q,q,sizeof QEKF_INS.q);}
void IMU_QuaternionEKF_Update(float gx,float gy,float gz,float ax,float ay,float az,float step)
{(void)gx;(void)gy;(void)gz;(void)ax;(void)ay;(void)az;(void)step;++ekf_updates;}

static void reset_driver(unsigned fail)
{
    INS=(INS_t){0};mpu_data=(mpu_data_t){0};INS_Init_flag=0;
    ins_healthy=false;reads=0;ekf_inits=ekf_updates=0;timeline=heater=0;fail_at=fail;
}
int main(void)
{
    reset_driver(3000); /* Fails after a nonzero, partial gyro sum. */
    assert(INS_Init()!=NULL);
    fail_at=0; /* The next SPI operation would succeed. */
    INS_Task();
    assert(ekf_inits==0 && ekf_updates==0 && !INSIsHealthy(HAL_GetTick()));
    assert(heater==0);
    reset_driver(0);
    assert(INS_Init()!=NULL);
    INS_Task();
    assert(ekf_inits==1 && ekf_updates==1 && INSIsHealthy(HAL_GetTick()));
    assert(fabsf(mpu_data.GyroOffset[2]-30*MPU6500_GYRO_SEN)<.00001f);
    puts("PASS: partial calibration SPI failure keeps INS and vehicle permission offline");
}
