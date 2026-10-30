#include "chassis_speed_observation.h"
#include "speed_observation_config.h"
volatile ChassisSpeedObservationRequest chassis_speed_observation_request;
volatile ChassisSpeedObservationDiagnostics chassis_speed_observation_diagnostics;
#if CHASSIS_SPEED_CAPTURE_ENABLE || CHASSIS_SPEED_KF_SHADOW_ENABLE
#include "dji_power_adapter.h"
#include "chassis_power_shadow.h"
#include "bsp_dwt.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "crc16.h"
#include <string.h>
#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG
#include "chassis_speed_kf_experiment.h"
#endif
#if CHASSIS_SPEED_CAPTURE_ENABLE
#include "bsp_uart_log.h"
#include "super_cap.h"
static CSCCapture capture;
static CSCCycleRecord record;
static uint8_t frame[CSC_MAX_FRAME_BYTES];
static uint32_t capture_config,capture_epoch,capture_model,capture_kf;
#endif
static uint32_t begin_cycles,begin_tick,begin_hz,last_begin,last_begin_tick,last_begin_hz,period_us;
static bool started;
static uint32_t elapsed(uint32_t start_cycles,uint32_t start_tick,uint32_t start_hz,
                        uint32_t cycles,uint32_t tick,uint32_t hz)
{
    if(!hz || hz!=start_hz)return UINT32_MAX;
    const uint32_t coarse=tick-start_tick;
    const uint64_t coarse_us=(uint64_t)coarse*1000U;
    const uint64_t wrap_us=(UINT64_C(1)<<32)*1000000U/hz;
    const uint64_t us=(uint64_t)(uint32_t)(cycles-start_cycles)*1000000U/hz;
    const uint64_t difference=us>coarse_us ? us-coarse_us:coarse_us-us;
    if(coarse>=UINT32_C(0x80000000) || coarse_us+2000U>=wrap_us ||
       difference>2000U || us>=UINT32_MAX)return UINT32_MAX;
    return (uint32_t)us;
}
static unsigned histogram_bucket(uint32_t us)
{const uint32_t limits[7]={100,300,500,1000,2000,3000,5000};unsigned b=0;while(b<7&&us>limits[b])++b;return b;}
void ChassisSpeedObservationInit(void)
{
    memset((void *)&chassis_speed_observation_request,0,sizeof chassis_speed_observation_request);
    memset((void *)&chassis_speed_observation_diagnostics,0,sizeof chassis_speed_observation_diagnostics);
    ChassisSpeedObservationResources(UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX);
    started=false;
#if CHASSIS_SPEED_CAPTURE_ENABLE
    memset(&capture,0,sizeof capture);
    chassis_speed_observation_diagnostics.config_result=DJIPowerAdapterSetSpeedCapture(&capture) ? 1U:2U;
#endif
#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG
    const PPWheelKFConfig *c=&chassis_speed_experiment_kf_config;
    uint8_t b[40]={0};
    CSCWriteU32(b,c->version);CSCWriteU32(b+4,c->max_gap_us);CSCWriteU32(b+8,c->max_age_us);
    CSCWriteU32(b+12,c->max_horizon_us);CSCWriteF32(b+16,c->q);CSCWriteF32(b+20,c->r);
    CSCWriteF32(b+24,c->initial_p00);CSCWriteF32(b+28,c->initial_p11);CSCWriteF32(b+32,c->gate_squared);b[36]=c->max_rejections;
    chassis_speed_observation_diagnostics.config_result=
        crc_16(b,40)==chassis_speed_experiment_kf_checksum && ChassisPowerShadowSetWheelKFConfig(c) ? 1U:3U;
#endif
}
bool ChassisSpeedObservationArm(uint32_t id)
{
#if CHASSIS_SPEED_CAPTURE_ENABLE
    if(capture.state!=CSC_IDLE || !ChassisPowerShadowVersions(&capture_model,&capture_kf)) return false;
    /* IDLE storage cannot be drained or appended. Prepare in place so Arm's
     * publication lock does not copy the 1 KiB metadata block. */
    CSCMetadata *metadata=&capture.metadata;
    CSCMetadataInit(metadata,id,DWT_GetFrequencyHz());
    if(!DJIPowerAdapterDescribeSpeedCapture(metadata))return false;
    CSCWriteU16(metadata->bytes+552,crc_16(metadata->bytes+64,40));
#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG
    if(chassis_speed_experiment_kf_synthetic)
        CSCWriteU32(metadata->bytes+44,CSCReadU32(metadata->bytes+44)|32U);
#endif
    /* Metadata is prepared outside the publication lock. */
    taskENTER_CRITICAL();bool ok=CSCArm(&capture,id,metadata);taskEXIT_CRITICAL();
    if(ok){
        capture_epoch=CSCReadU32(metadata->bytes+32);capture_config=CSCReadU32(metadata->bytes+624);
        ChassisPowerShadowResume();
    }return ok;
#else
    (void)id;return false;
#endif
}
void ChassisSpeedObservationFreeze(void)
{
#if CHASSIS_SPEED_CAPTURE_ENABLE
    taskENTER_CRITICAL();CSCFreeze(&capture);taskEXIT_CRITICAL();
#endif
}
void ChassisSpeedObservationMotorBegin(uint32_t cycles,uint32_t tick_ms)
{
    begin_cycles=cycles;begin_tick=tick_ms;begin_hz=DWT_GetFrequencyHz();
    period_us=started ? elapsed(last_begin,last_begin_tick,last_begin_hz,cycles,tick_ms,begin_hz):UINT32_MAX;
    last_begin=cycles;last_begin_tick=tick_ms;last_begin_hz=begin_hz;started=true;
    chassis_speed_observation_diagnostics.period_us=period_us;
    if(period_us!=UINT32_MAX) {
        if(period_us>chassis_speed_observation_diagnostics.period_max_us)chassis_speed_observation_diagnostics.period_max_us=period_us;
        ++chassis_speed_observation_diagnostics.period_histogram[histogram_bucket(period_us)];
    }
    uint32_t seq=chassis_speed_observation_request.sequence;
    if(seq!=chassis_speed_observation_request.acknowledged_sequence) {
        uint32_t action=chassis_speed_observation_request.action,id=chassis_speed_observation_request.capture_id;
        if(seq==chassis_speed_observation_request.sequence) {
            bool ok=false;
            if(action==1)ok=ChassisSpeedObservationArm(id);
            if(action==2){ChassisSpeedObservationFreeze();ok=true;}
            chassis_speed_observation_request.result=ok ? 1U:2U;
            chassis_speed_observation_request.acknowledged_sequence=seq;
        }
    }
}
void ChassisSpeedObservationMotorEnd(uint32_t cycles,uint32_t tick_ms)
{
    uint32_t us=elapsed(begin_cycles,begin_tick,begin_hz,cycles,tick_ms,DWT_GetFrequencyHz());
    chassis_speed_observation_diagnostics.execution_us=us;
    if(us!=UINT32_MAX) {
        if(us>chassis_speed_observation_diagnostics.execution_max_us)chassis_speed_observation_diagnostics.execution_max_us=us;
        ++chassis_speed_observation_diagnostics.execution_histogram[histogram_bucket(us)];
        if(us>2000)++chassis_speed_observation_diagnostics.overruns;
    }
#if CHASSIS_SPEED_CAPTURE_ENABLE
    if(DJIPowerAdapterTakeSpeedCycle(&record)) {
        /* tick_ms belongs to the adapter observation clock, which may be later
         * than MotorBegin. Preserve its pairing with observation_cycles. */
        uint8_t *b=record.bytes;CSCWriteU32(b+4,begin_cycles);CSCWriteU32(b+8,cycles);
        CSCWriteU32(b+12,period_us);CSCWriteU32(b+16,us);
        uint32_t flags=CSCReadU32(b+64);
        if(period_us!=UINT32_MAX)flags|=1U;
#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG
        if(chassis_speed_experiment_kf_synthetic)flags|=16U;
#endif
        CSCWriteU32(b+64,flags);
        uint8_t *d=b+400;
        CSCWriteU32(d+68,chassis_speed_observation_diagnostics.execution_max_us);
        CSCWriteU32(d+76,chassis_speed_observation_diagnostics.motor_stack_words);
        CSCWriteU32(d+80,chassis_speed_observation_diagnostics.robot_stack_words);
        CSCWriteU32(d+84,chassis_speed_observation_diagnostics.observation_stack_words);
        CSCWriteU32(d+88,chassis_speed_observation_diagnostics.heap_free_bytes);
        CSCWriteU32(d+92,chassis_speed_observation_diagnostics.heap_min_bytes);
        uint32_t config=CSCReadU32(b+20),epoch=CSCReadU32(b+40);
        taskENTER_CRITICAL();
        if(config!=capture_config || epoch!=capture_epoch || CSCReadU32(b+28)!=capture_model ||
            CSCReadU32(b+36)!=capture_kf)CSCFreezeReason(&capture,CSC_FREEZE_VERSION);
        (void)CSCAppendCycleTimed(&capture,&record,begin_tick,tick_ms);taskEXIT_CRITICAL();
    }
#endif
}
void ChassisSpeedObservationResources(uint32_t motor,uint32_t robot,uint32_t observation,uint32_t heap,uint32_t minimum)
{
    chassis_speed_observation_diagnostics.motor_stack_words=motor;
    chassis_speed_observation_diagnostics.robot_stack_words=robot;
    chassis_speed_observation_diagnostics.observation_stack_words=observation;
    chassis_speed_observation_diagnostics.heap_free_bytes=heap;
    chassis_speed_observation_diagnostics.heap_min_bytes=minimum;
}
void ChassisSpeedObservationTask(void)
{
#if CHASSIS_SPEED_CAPTURE_ENABLE
    BSPUARTLogPoll();
    SuperCapSnapshot_s input;
    if(SuperCapGetSnapshot(HAL_GetTick(),&input))DJIPowerAdapterPublishInputPower(&input);
    if(!DJIPowerAdapterDriveStopped(HAL_GetTick()))return;
    taskENTER_CRITICAL();bool draining=CSCBeginDrain(&capture);taskEXIT_CRITICAL();
    if(!draining)return;
    uint16_t length;
    if(CSCPeekFrame(&capture,frame,sizeof frame,&length)) {
        /* Drive permission is published by tasks. Serialize its final check
         * with the nonblocking enqueue; IRQs stay enabled during this section. */
        vTaskSuspendAll();
        if(DJIPowerAdapterDriveStopped(HAL_GetTick())) {
            if(BSPUARTLogWrite(frame,length)==length)CSCFrameAccepted(&capture);
            else CSCTransportRetry(&capture);
        }
        (void)xTaskResumeAll();
    }
    taskENTER_CRITICAL();CSCDrainComplete(&capture,BSPUARTLogIsIdle());taskEXIT_CRITICAL();
#endif
}
#else
void ChassisSpeedObservationInit(void) {}
void ChassisSpeedObservationMotorBegin(uint32_t c,uint32_t t) {(void)c;(void)t;}
void ChassisSpeedObservationMotorEnd(uint32_t c,uint32_t t) {(void)c;(void)t;}
void ChassisSpeedObservationTask(void) {}
bool ChassisSpeedObservationArm(uint32_t id) {(void)id;return false;}
void ChassisSpeedObservationFreeze(void) {}
void ChassisSpeedObservationResources(uint32_t a,uint32_t b,uint32_t c,uint32_t d,uint32_t e)
{(void)a;(void)b;(void)c;(void)d;(void)e;}
#endif
