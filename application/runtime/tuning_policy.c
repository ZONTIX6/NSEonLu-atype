#include "tuning_policy.h"
#include "tuning_flash.h"
#include "runtime_tuning.h"
#include "vehicle_safety.h"
#include "chassis.h"
#include "dji_motor.h"
#include <stddef.h>

static bool TuningPolicyCanSave(void *context,uint32_t now_ms)
{
    (void)context;
    ChassisTelemetrySnapshot_s chassis={0};
    bool safe=VehicleSafetyIsStopped(now_ms) && !RuntimeTuningTrialActive() &&
        ChassisGetTelemetrySnapshot(&chassis) && chassis.mode==0 &&
        (uint32_t)(now_ms-chassis.tick_ms)<=5 && DJIMotorCanSaveParameters(now_ms);
#if defined(CHASSIS_BOARD)
    safe &= VehicleSafetyAuxStopped(now_ms);
#endif
    return safe;
}
void TuningPolicyInit(void) {(void)TuningFlashRegisterSaveGuard(TuningPolicyCanSave,NULL);}
