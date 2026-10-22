#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../modules/imu/ins_service.c"

TestEKF QEKF_INS;
static unsigned reads,fail_at,ekf_inits,ekf_updates,heater_calls;
static float timeline,heater;
static bool timeout_samples, stale_sample;
float DWT_GetDeltaT(uint32_t *count) {(void)count;return .001f;}
float DWT_GetTimeline_s(void) {return timeline;}
void DWT_Delay(float seconds) {timeline+=seconds;}
uint32_t HAL_GetTick(void) {return (uint32_t)(timeline*1000);}
static bool Sample(void *context,IMUSample_s *sample)
{
    assert(context==&reads);
    ++reads;
    if(reads==fail_at) return false;
    *sample=(IMUSample_s){.accel_m_s2={0,0,9.8f},.gyro_rad_s={.005f,.01f,.015f},
        .temperature_c=21,.tick_ms=HAL_GetTick(),.valid=true};
    if(timeout_samples) {sample->accel_m_s2[2]=5;timeline+=1;sample->tick_ms=HAL_GetTick();}
    if(stale_sample) sample->tick_ms-=11;
    return true;
}
static void Heat(void *context,float duty) {assert(context==&reads);heater=duty;++heater_calls;}
void PIDInit(PIDInstance *pid,PID_Init_Config_s *config) {(void)config;pid->Output=10;}
float PIDCalculate(PIDInstance *pid,float measured,float target)
{(void)measured;(void)target;return pid->Output;}
int SEGGER_RTT_printf(unsigned channel,const char *format,...) {(void)channel;(void)format;return 0;}
void IMU_QuaternionEKF_Init(float *q,float a,float b,float c,float d,float e)
{(void)a;(void)b;(void)c;(void)d;(void)e;++ekf_inits;memcpy(QEKF_INS.q,q,sizeof QEKF_INS.q);}
void IMU_QuaternionEKF_Update(float gx,float gy,float gz,float ax,float ay,float az,float step)
{(void)gx;(void)gy;(void)gz;(void)ax;(void)ay;(void)az;(void)step;++ekf_updates;}

static void Reset(unsigned fail)
{
    INS=(INS_t){0};mpu_data=(mpu_data_t){0};INS_Init_flag=0;
    service_configured=false;ins_healthy=false;reads=ekf_inits=ekf_updates=heater_calls=0;
    timeline=heater=0;fail_at=fail;timeout_samples=stale_sample=false;
    const INSServiceConfig_s config={.sample=Sample,.heat=Heat,.context=&reads};
    assert(!INSServiceConfigure(NULL));assert(INSServiceConfigure(&config));
    assert(!INSServiceConfigure(&config));
}
int main(void)
{
    INSSnapshot_s snapshot;
    assert(!INSGetSnapshot(NULL));
    Reset(3000);
    attitude_t *address=INS_Init();assert(address);
    fail_at=0;INS_Task();
    assert(ekf_inits==0 && ekf_updates==0 && !INSIsHealthy(HAL_GetTick()));
    assert(heater==0 && !INSGetSnapshot(&snapshot));
    assert(INS_Init()==address && reads==3000);
    Reset(0);address=INS_Init();assert(address);INS_Task();
    assert(ekf_inits==1 && ekf_updates==1 && INSIsHealthy(HAL_GetTick()));
    assert(INSGetSnapshot(&snapshot) && snapshot.valid);
    assert(fabsf(mpu_data.GyroOffset[2]-.015f)<.00001f);
    assert(fabsf(heater-.1f)<.000001f && INS_Init()==address);
    assert(address->Pitch==snapshot.attitude.Pitch && address->Accel[2]>9.7f);
    const uint32_t published=snapshot.tick_ms;
    fail_at=reads+1;INS_Task();
    assert(!INSGetSnapshot(&snapshot) && !snapshot.valid && snapshot.tick_ms==published && heater==0);
    fail_at=0;stale_sample=true;
    unsigned updates_before=ekf_updates;INS_Task();assert(ekf_updates==updates_before);
    Reset(0);timeout_samples=true;INS_Init();
    assert(ekf_inits==1 && fabsf(mpu_data.GyroOffset[0]-.037452f)<.00001f);
    puts("PASS: INS callbacks, calibration failure latch, timeout fallback and stable snapshots");
}
