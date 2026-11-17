#ifndef GIMBAL_H
#define GIMBAL_H

#if CHASSIS_VELOCITY_FUSION_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include "../../modules/motor/DJImotor/dji_motor_diagnostics.h"

typedef struct {
    uint32_t tick_ms, cycles;
    uint16_t ecd;
    int16_t rpm;
    bool valid;
} GimbalEncoderSample_s;

/* Raw mechanical yaw and pitch, in that order. */
bool GimbalGetEncoderSnapshot(GimbalEncoderSample_s samples[2]);
bool GimbalGetYawControlSnapshot(DJIMotorControlSnapshot_s *out);
#endif

/**
 * @brief 初始化云台,会被RobotInit()调用
 * 
 */
void GimbalInit();

/**
 * @brief 云台任务
 * 
 */
void GimbalTask();

#endif // GIMBAL_H
