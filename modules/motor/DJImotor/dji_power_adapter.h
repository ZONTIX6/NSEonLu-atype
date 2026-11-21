#ifndef DJI_POWER_ADAPTER_H
#define DJI_POWER_ADAPTER_H

#include "dji_motor.h"
#include "chassis_power.h"
#include "../../telemetry/chassis_speed_capture.h"

typedef struct
{
    DJIMotorInstance *motors[4]; /* LF, RF, LB, RB; already registered M3508s. */
    uint8_t count;
    float max_current_command;
} DJIPowerAdapterConfig_s;

bool DJIPowerAdapterInit(const DJIPowerAdapterConfig_s *config);
bool DJIPowerAdapterPublish(const ChassisPowerControlSnapshot_s *control);
bool DJIPowerAdapterGetVelocityFeedbackEvidence(DJIVelocityFeedbackEvidence_s *out);

bool DJIPowerAdapterSetSpeedCapture(CSCCapture *);
bool DJIPowerAdapterTakeSpeedCycle(CSCCycleRecord *);
bool DJIPowerAdapterDescribeSpeedCapture(CSCMetadata *);
bool DJIPowerAdapterDriveStopped(uint32_t now_ms);
void DJIPowerAdapterPublishInputPower(const SuperCapSnapshot_s *);
#endif
