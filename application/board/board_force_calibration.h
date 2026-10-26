#ifndef BOARD_FORCE_CALIBRATION_H
#define BOARD_FORCE_CALIBRATION_H
#include "force_calibration.h"
#if CHASSIS_FORCE_CALIBRATION_ENABLE
#include "robot_def.h"
#include "chassis_control_config.h"
#if !defined(ONE_BOARD)
#error Force calibration requires ONE_BOARD
#endif
#if CHASSIS_CONTROL_MODE != 1
#error Force calibration requires force backend
#endif
#if CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_OMNI_X
#error Force calibration requires OMNI_X
#endif
#if CHASSIS_POWER_CONTROL_STAGE != 3
#error Force calibration requires power stage 3
#endif
#if !ATYPE_BENCH_MODE
#error Force calibration requires bench firmware
#endif
#if !USB_TELEMETRY_ENABLE
#error Force calibration requires USB telemetry
#endif
#if !POWER_METER_ENABLE
#error Force calibration requires load power meter
#endif
#if CHASSIS_FORCE_SPIN_MODE != 0
#error Force calibration requires SPIN=0
#endif
#endif
FcLimits BoardForceCalibrationLimits(void);
#endif
