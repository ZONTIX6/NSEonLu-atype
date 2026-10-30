#ifndef DJI_TUNING_ADAPTER_H
#define DJI_TUNING_ADAPTER_H

#include "dji_motor.h"

/* One registered four-wheel group, matching the existing vehicle policy. */
const DJIMotorControlPolicy_s *DJITuningAdapterPolicy(void);

#endif
