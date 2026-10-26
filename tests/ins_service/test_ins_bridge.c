#include "ins_task.h"
#include "runtime/ins_bridge.h"
#include <assert.h>
#include <stdio.h>
static unsigned ticks,updates,sends;
static bool valid;
void INS_Task(void) {++ticks;}
bool INSGetSnapshot(INSSnapshot_s *output)
{*output=(INSSnapshot_s){.attitude={.Yaw=1,.Pitch=2,.Roll=3},.valid=valid};return valid;}
void VisionSetAltitude(float yaw,float pitch,float roll) {assert(yaw==1 && pitch==2 && roll==3);++updates;}
void VisionSend(void) {++sends;}
int main(void)
{
    INSBridgeTask();valid=true;INSBridgeTask();
    assert(ticks==2);
#ifdef CHASSIS_BOARD
    assert(updates==0 && sends==0);
#else
    assert(updates==1 && sends==2);
#endif
    puts("PASS: role-specific INS/vision cadence and invalid sample exclusion");
}
