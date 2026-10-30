#include "force_calibration_app.h"
#include "board/board_force_calibration.h"
#include "vehicle_safety.h"
#include "bsp_usb.h"
#include "force_calibration_protocol.h"
#include <string.h>
static DJIMotorInstance *wheels[4];
static bool aux_confirming,aux_ready;
static uint32_t aux_since;
static void link_snapshot(bool *open,uint32_t *generation)
{*open=CDC_IsPortOpen_FS()!=0;*generation=CDC_GetSession_FS();}
bool ForceCalibrationAppInit(DJIMotorInstance *const motors[4])
{
    memcpy(wheels,motors,sizeof wheels);const FcLimits limits=BoardForceCalibrationLimits();
    ForceCalibrationProtocolConfigure(&limits);
    return DJIForceCalibrationInit(motors,&limits,link_snapshot);
}
bool ForceCalibrationAppPermit(const ChassisForceIntentReceiver_s *command,uint32_t now)
{
    bool aux=DJIMotorAuxiliaryStopped(wheels,now);
    if(!aux) {aux_confirming=false;aux_ready=false;}
    else if(!aux_confirming) {aux_confirming=true;aux_since=now;}
    else aux_ready=(uint32_t)(now-aux_since)>=100;
    return command && command->valid && ChassisForceIntentFresh(command,now,0) &&
        command->intent.input_permitted && command->intent.bench_permitted &&
        command->intent.input_x==0 && command->intent.input_y==0 && aux_ready &&
        VehicleSafetyAllowsAutotune(now) && VehicleSafetyAllowsDrive(now);
}
void ForceCalibrationAppPublish(const ChassisForceIntentReceiver_s *command,const ChassisPowerControlSnapshot_s *power,uint32_t now)
{
    if(!command || !power) {DJIForceCalibrationCancel(FC_REASON_PERMISSION);return;}
    FcPermit permit={.power=*power,.command_received_ms=command->received_ms,
        .source_age_ms=command->intent.source_age_ms,.yaw_age_ms=command->intent.yaw_age_ms,
        .safety_epoch=VehicleSafetyEpoch(),.bench_permitted=ForceCalibrationAppPermit(command,now),.aux_stopped=aux_ready};
    DJIForceCalibrationPublish(&permit);
}
