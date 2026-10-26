#ifndef FORCE_CALIBRATION_APP_H
#define FORCE_CALIBRATION_APP_H
#include "dji_force_calibration.h"
#include "chassis_force_intent.h"
bool ForceCalibrationAppInit(DJIMotorInstance *const motors[4]);
bool ForceCalibrationAppPermit(const ChassisForceIntentReceiver_s *command,uint32_t now_ms);
void ForceCalibrationAppPublish(const ChassisForceIntentReceiver_s *command,
    const ChassisPowerControlSnapshot_s *power,uint32_t now_ms);
#endif
