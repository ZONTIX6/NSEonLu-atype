#include "chassis_speed_observation.h"
#include "dji_power_adapter.h"
#include "chassis_power_shadow.h"
#include "super_cap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static CSCCapture *bound;
static uint32_t tick,frequency=1000000;
static bool have_cycle,stopped=true,blocked=true,idle;
static uint32_t cycle_kf_version;
static unsigned writes,polls;
uint32_t DWT_GetFrequencyHz(void) {return frequency;}
uint32_t HAL_GetTick(void) {return tick;}
bool DJIPowerAdapterSetSpeedCapture(CSCCapture *c) {bound=c;return true;}
bool DJIPowerAdapterDescribeSpeedCapture(CSCMetadata *m) {(void)m;return true;}
bool DJIPowerAdapterTakeSpeedCycle(CSCCycleRecord *c) {if(!have_cycle)return false;memset(c,0,sizeof *c);CSCWriteU32(c->bytes+36,cycle_kf_version);have_cycle=false;return true;}
bool DJIPowerAdapterDriveStopped(uint32_t now) {(void)now;return stopped;}
void DJIPowerAdapterPublishInputPower(const SuperCapSnapshot_s *c) {(void)c;}
bool SuperCapGetSnapshot(uint32_t now,SuperCapSnapshot_s *c) {(void)now;memset(c,0,sizeof *c);return false;}
void ChassisPowerShadowResume(void) {}
bool ChassisPowerShadowVersions(uint32_t *m,uint32_t *k) {*m=0;*k=0;return true;}
void BSPUARTLogPoll(void) {++polls;if(!blocked)idle=true;}
unsigned BSPUARTLogWrite(const void *p,unsigned n) {(void)p;++writes;return blocked ? 0:n;}
bool BSPUARTLogIsIdle(void) {return idle;}
int main(void)
{
    ChassisSpeedObservationInit();assert(bound);
    assert(ChassisSpeedObservationArm(1));
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
    puts("PASS: real runtime measures period/execution independently and drains after congestion");
}
