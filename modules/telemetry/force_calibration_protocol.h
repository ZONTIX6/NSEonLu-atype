#ifndef FORCE_CALIBRATION_PROTOCOL_H
#define FORCE_CALIBRATION_PROTOCOL_H
#include "../chassis_force_calibration/force_calibration.h"
#include <stddef.h>
void ForceCalibrationProtocolInit(void);
void ForceCalibrationProtocolSubmit(const char *line,uint32_t generation);
size_t ForceCalibrationProtocolReply(char *buffer,size_t capacity);
void ForceCalibrationProtocolReplySent(void);
void ForceCalibrationProtocolCancel(uint32_t generation,FcReason reason);
#endif
