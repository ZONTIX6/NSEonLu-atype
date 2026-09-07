#ifndef CHASSIS_POWER_FEEDBACK_H
#define CHASSIS_POWER_FEEDBACK_H

#include "power_model.h"
#include "robot_def.h"

#include <stdbool.h>

void ChassisPowerFeedbackUpdate(Chassis_Power_Control_Data_s *feedback,
                                bool registered,
                                const ChassisPowerOutput_s *power);

#endif
