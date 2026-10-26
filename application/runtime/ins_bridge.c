#include "ins_bridge.h"
#include "ins_task.h"
#include "robot_def.h"
#include "master_process.h"

void INSBridgeTask(void)
{
    INS_Task();
#if defined(ONE_BOARD) || defined(GIMBAL_BOARD)
    INSSnapshot_s snapshot;
    if(INSGetSnapshot(&snapshot)) VisionSetAltitude(snapshot.attitude.Yaw,snapshot.attitude.Pitch,snapshot.attitude.Roll);
    VisionSend();
#endif
}
