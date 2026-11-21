#ifndef DJI_TRACTION_H
#define DJI_TRACTION_H
#include "chassis_traction.h"

typedef struct {
    ChassisTractionOutput_s traction;
    float raw_current[4], target_rpm[4], feedback_current[4];
    int16_t final_current[4];
    ChassisTractionReference_s reference;
    uint32_t tick_ms, control_version;
    uint32_t cycles, intervention_cycles, submitted_intervention_cycles;
    uint32_t spin_cycles, lock_cycles;
    uint8_t enabled_mask;
    bool available, can_submitted;
} DJITractionSnapshot_s;
bool DJIPowerAdapterGetTractionSnapshot(DJITractionSnapshot_s *out);
bool DJIPowerAdapterTractionLimited(uint32_t now_ms);
#endif
