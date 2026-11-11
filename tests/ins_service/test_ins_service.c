#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CHASSIS_VELOCITY_FUSION_ENABLE 1
#include "../../modules/imu/ins_service.c"

TestEKF QEKF_INS;
static unsigned reads,fail_at,ekf_inits,ekf_updates,heater_calls;
static float timeline,heater;
static bool timeout_samples, stale_sample;
static bool moving_sample;
static uint32_t cycles_at_read;
static float ekf_delay;
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
    if (moving_sample) {
        sample->accel_m_s2[0]=-9.8f;
        sample->accel_m_s2[1]=1.0f;
        sample->accel_m_s2[2]=0.0f;
        sample->gyro_rad_s[0]=.025f;
    }
    cycles_at_read=(uint32_t)(timeline*168000000.0f);
    sample->rx_cycles=cycles_at_read;
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
{(void)gx;(void)gy;(void)gz;(void)ax;(void)ay;(void)az;(void)step;++ekf_updates;timeline+=ekf_delay;}

static void Reset(unsigned fail)
{
    INS=(INS_t){0};mpu_data=(mpu_data_t){0};INS_Init_flag=0;
    service_configured=false;ins_healthy=false;reads=ekf_inits=ekf_updates=heater_calls=0;
    timeline=heater=0;fail_at=fail;timeout_samples=stale_sample=false;
    moving_sample=false;ekf_delay=0.0f;
    published_motion=(INSMotionSnapshot_s){0};
    const INSServiceConfig_s config={.sample=Sample,.heat=Heat,.context=&reads};
    assert(!INSServiceConfigure(NULL));assert(INSServiceConfigure(&config));
    assert(!INSServiceConfigure(&config));
}

static void TestMotionSnapshot(void)
{
    Reset(0);
    INS_Init();
    assert(!INSGetMotionSnapshot(NULL));
    INSMotionSnapshot_s motion;
    assert(!INSGetMotionSnapshot(&motion) && !motion.valid);

    INS_Task();
    assert(INSGetMotionSnapshot(&motion));
    assert(fabsf(motion.accel_n_mps2[0])<.005f && fabsf(motion.accel_n_mps2[2])<.005f);
    const uint32_t first_sequence=motion.sequence;

    moving_sample=true;
    /* Sensor rotated +90 deg about y. Include small EKF normalization error. */
    QEKF_INS.q[0]=QEKF_INS.q[2]=.706f;
    timeline+=.001f;
    const uint32_t read_tick=HAL_GetTick();
    ekf_delay=.002f;
    INS_Task();
    assert(INSGetMotionSnapshot(&motion) && motion.valid);
    assert(motion.sequence==first_sequence+1 && motion.tick_ms==read_tick && motion.cycles==cycles_at_read);
    assert(fabsf(motion.quaternion[2]-.707106781f)<.00001f);
    assert(fabsf(motion.accel_n_mps2[0])<.005f && fabsf(motion.accel_n_mps2[1]-1.001f)<.005f &&
        fabsf(motion.accel_n_mps2[2])<.005f);
    assert(fabsf(motion.gyro_sensor_radps[0]-.02f)<.00001f);

    fail_at=reads+1;
    INS_Task();
    assert(!INSGetMotionSnapshot(&motion) && !motion.valid && motion.sequence==first_sequence+1);
    INS_Task();
    assert(INSGetMotionSnapshot(&motion) && motion.sequence==first_sequence+2);
    timeline+=.011f;
    assert(!INSGetMotionSnapshot(&motion) && !motion.valid);

    QEKF_INS.q[2]=NAN;
    INS_Task();
    assert(!INSGetMotionSnapshot(&motion) && !motion.valid);
    INSSnapshot_s legacy;
    assert(INSGetSnapshot(&legacy) && legacy.valid);
}

int main(void)
{
    INSSnapshot_s snapshot;
    assert(!INSGetSnapshot(NULL));
    Reset(3000);
    attitude_t *address=INS_Init();assert(address);
    fail_at=0;INS_Task();
    assert(ekf_inits==0 && ekf_updates==0 && !INSIsHealthy(HAL_GetTick()));
    assert(heater==0 && !INSGetSnapshot(&snapshot) && !snapshot.has_sample);
    assert(INS_Init()==address && reads==3000);
    Reset(0);address=INS_Init();assert(address);INS_Task();
    assert(ekf_inits==1 && ekf_updates==1 && INSIsHealthy(HAL_GetTick()));
    assert(INSGetSnapshot(&snapshot) && snapshot.valid && snapshot.has_sample);
    assert(fabsf(mpu_data.GyroOffset[2]-.015f)<.00001f);
    assert(fabsf(heater-.1f)<.000001f && INS_Init()==address);
    assert(address->Pitch==snapshot.attitude.Pitch && address->Accel[2]>9.7f);
    const uint32_t published=snapshot.tick_ms;
    fail_at=reads+1;INS_Task();
    assert(!INSGetSnapshot(&snapshot) && !snapshot.valid && snapshot.has_sample && snapshot.tick_ms==published && heater==0);
    fail_at=0;stale_sample=true;
    unsigned updates_before=ekf_updates;INS_Task();assert(ekf_updates==updates_before);
    Reset(0);timeout_samples=true;INS_Init();
    assert(ekf_inits==1 && fabsf(mpu_data.GyroOffset[0]-.037452f)<.00001f);
    TestMotionSnapshot();
    puts("PASS: INS callbacks, calibration, timestamped motion and snapshot validity");
}
