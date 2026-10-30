#include "chassis_speed_observation.h"
#include "dji_power_adapter.h"
#include "chassis_power_shadow.h"
#include "super_cap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static CSCCapture *bound;
static uint32_t tick,frequency=1000000;
static bool have_cycle,stopped=true,blocked=true,idle,late_raw,resume_after_check;
static uint32_t cycle_kf_version;
static unsigned writes,polls;
static bool scheduler_suspended;
void vTaskSuspendAll(void) {assert(!scheduler_suspended);scheduler_suspended=true;}
int xTaskResumeAll(void) {assert(scheduler_suspended);scheduler_suspended=false;return 0;}
uint32_t DWT_GetFrequencyHz(void) {return frequency;}
uint32_t HAL_GetTick(void) {return tick;}
bool DJIPowerAdapterSetSpeedCapture(CSCCapture *c) {bound=c;return true;}
bool DJIPowerAdapterDescribeSpeedCapture(CSCMetadata *m) {(void)m;return true;}
bool DJIPowerAdapterTakeSpeedCycle(CSCCycleRecord *c) {
    if(!have_cycle)return false;
    if(late_raw) {
        CSCRawRecord r={{0}};CSCWriteU32(r.bytes+8,10);CSCWriteU32(r.bytes+12,10100);
        assert(CSCAppendRaw(bound,&r));late_raw=false;
    }
    memset(c,0,sizeof *c);CSCWriteU32(c->bytes,tick);CSCWriteU32(c->bytes+36,cycle_kf_version);have_cycle=false;return true;
}
bool DJIPowerAdapterDriveStopped(uint32_t now) {
    (void)now;bool result=stopped;
    if(resume_after_check){stopped=false;resume_after_check=false;}
    return result;
}
void DJIPowerAdapterPublishInputPower(const SuperCapSnapshot_s *c) {(void)c;}
bool SuperCapGetSnapshot(uint32_t now,SuperCapSnapshot_s *c) {(void)now;memset(c,0,sizeof *c);return false;}
void ChassisPowerShadowResume(void) {}
bool ChassisPowerShadowVersions(uint32_t *m,uint32_t *k) {*m=0;*k=0;return true;}
void BSPUARTLogPoll(void) {++polls;if(!blocked)idle=true;}
unsigned BSPUARTLogWrite(const void *p,unsigned n) {(void)p;assert(scheduler_suspended);++writes;return blocked ? 0:n;}
bool BSPUARTLogIsIdle(void) {return idle;}
static void assert_no_execution_sample(void)
{
    assert(chassis_speed_observation_diagnostics.execution_us==UINT32_MAX);
    assert(chassis_speed_observation_diagnostics.execution_max_us==0);
    assert(chassis_speed_observation_diagnostics.overruns==0);
    for(unsigned i=0;i<8;++i)assert(chassis_speed_observation_diagnostics.execution_histogram[i]==0);
}
static void test_invalid_execution_time_is_not_a_duration(void)
{
    ChassisSpeedObservationInit();frequency=0;
    ChassisSpeedObservationMotorBegin(0,0);ChassisSpeedObservationMotorEnd(400,0);
    assert_no_execution_sample();
    ChassisSpeedObservationInit();frequency=1000000;
    ChassisSpeedObservationMotorBegin(0,0);frequency=2000000;
    ChassisSpeedObservationMotorEnd(400,0);assert_no_execution_sample();
    ChassisSpeedObservationInit();frequency=1000000;
    ChassisSpeedObservationMotorBegin(0,0);
    /* Same low DWT word after a full hardware wrap must not become 100 us. */
    ChassisSpeedObservationMotorEnd(100,4294968);assert_no_execution_sample();
    ChassisSpeedObservationMotorBegin(100,4294968);
    assert(chassis_speed_observation_diagnostics.period_us==UINT32_MAX);
    ChassisSpeedObservationMotorEnd(500,4294968);
    assert(chassis_speed_observation_diagnostics.execution_us==400);
}
int main(void)
{
    ChassisSpeedObservationInit();assert(bound);
    assert(ChassisSpeedObservationArm(1));
    ChassisSpeedObservationMotorBegin(900,0);tick=1;have_cycle=true;ChassisSpeedObservationMotorEnd(1300,1);
    /* The adapter's observation timestamp can be later than MotorBegin. */
    assert(CSCReadU32(bound->cycle[0].bytes)==1);
    assert(CSCReadU32(bound->cycle[0].bytes+4)==900);
    assert(CSCReadU32(bound->cycle[0].bytes+16)==400);
    assert(bound->start_tick_ms==0 && bound->end_tick_ms==1);
    ChassisSpeedObservationInit();assert(ChassisSpeedObservationArm(1));tick=9;
    ChassisSpeedObservationMotorBegin(9000,9);have_cycle=true;late_raw=true;
    ChassisSpeedObservationMotorEnd(10000,10);
    assert(bound->start_tick_ms==9 && bound->start_cycles==9000);
    assert(bound->end_tick_ms==10 && bound->end_cycles==10100);
    ChassisSpeedObservationInit();assert(ChassisSpeedObservationArm(1));
    ChassisSpeedObservationMotorBegin(0,0);have_cycle=true;ChassisSpeedObservationMotorEnd(400,0);
    ChassisSpeedObservationMotorBegin(2500,2);have_cycle=true;ChassisSpeedObservationMotorEnd(3000,3);
    ChassisSpeedObservationMotorBegin(5200,5);have_cycle=true;ChassisSpeedObservationMotorEnd(5800,5);
    assert(chassis_speed_observation_diagnostics.period_us==2700);
    assert(chassis_speed_observation_diagnostics.execution_us==600);
    assert(CSCReadU32(bound->cycle[1].bytes+12)==2500);
    assert(CSCReadU32(bound->cycle[2].bytes+16)==600);
    ChassisSpeedObservationFreeze();
    for(unsigned i=0;i<10;++i)ChassisSpeedObservationTask();
    assert(bound->state==CSC_DRAINING && bound->drain_kind==CSC_KIND_METADATA);
    assert(bound->session_transport_retries==10);
    assert(polls==10);
    unsigned before_resume=writes;resume_after_check=true;
    ChassisSpeedObservationTask();
    assert(writes==before_resume); /* Drive resumed while preparing the frame. */
    stopped=false;blocked=false;unsigned before=writes;
    ChassisSpeedObservationTask();assert(writes==before);
    stopped=true;
    for(unsigned i=0;i<20;++i)ChassisSpeedObservationTask();
    assert(bound->state==CSC_IDLE);
    assert(ChassisSpeedObservationArm(2));
    cycle_kf_version=1;
    ChassisSpeedObservationMotorBegin(8000,8);have_cycle=true;ChassisSpeedObservationMotorEnd(8500,8);
    assert(bound->state==CSC_FROZEN && bound->freeze_reason==CSC_FREEZE_VERSION);
    assert(bound->cycle_count==0);
    test_invalid_execution_time_is_not_a_duration();
    puts("PASS: real runtime measures period/execution independently and drains after congestion");
}
