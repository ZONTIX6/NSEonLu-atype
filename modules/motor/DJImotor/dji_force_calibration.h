#ifndef DJI_FORCE_CALIBRATION_H
#define DJI_FORCE_CALIBRATION_H
#include "dji_motor.h"
#include "chassis_power.h"
#include "force_calibration_capture.h"
typedef struct {
    ChassisPowerControlSnapshot_s power;
    uint32_t command_received_ms,source_age_ms,yaw_age_ms,safety_epoch;
    bool bench_permitted,aux_stopped;
} FcPermit;
bool DJIForceCalibrationInit(DJIMotorInstance *const motors[4],const FcLimits *limits,
    void (*read_link)(bool *open,uint32_t *generation));
void DJIForceCalibrationPublish(const FcPermit *permit);
void DJIForceCalibrationReadControl(void);
void DJIForceCalibrationPrepare(const DJIMotorCycle_s *cycle);
bool DJIForceCalibrationValidate(const DJIMotorCycle_s *cycle,const int16_t current[DJI_MOTOR_CNT],
    const bool enabled[DJI_MOTOR_CNT],uint32_t now_ms);
void DJIForceCalibrationRecord(const DJIMotorCycle_s *cycle,const int16_t current[DJI_MOTOR_CNT],uint8_t attempted,uint8_t successful);
void DJIForceCalibrationFeedback(const DJIMotorInstance *motor,int16_t raw_rpm,int16_t raw_current,uint32_t rx_cycles);
void DJIForceCalibrationCancel(FcReason reason);
void DJIForceCalibrationGetStatus(FcStatus *status);
bool DJIForceCalibrationSubmit(const FcRequest *request);
bool DJIForceCalibrationReply(FcCommandResult *reply);
void DJIForceCalibrationReplyAccepted(uint32_t request_id);
bool DJIForceCalibrationRead(uint32_t capture_id,uint16_t first,uint8_t count,uint8_t *bytes,size_t capacity);
#endif
