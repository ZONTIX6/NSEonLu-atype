#ifndef CHASSIS_POWER_TRANSIENT_H
#define CHASSIS_POWER_TRANSIENT_H
#include "power_model.h"

typedef enum {
    CHASSIS_POWER_TRANSIENT_IDLE = 0,
    CHASSIS_POWER_TRANSIENT_STARTUP,
    CHASSIS_POWER_TRANSIENT_CLAMP,
    CHASSIS_POWER_TRANSIENT_RECOVERY,
    CHASSIS_POWER_TRANSIENT_RUN,
    CHASSIS_POWER_TRANSIENT_OFFLINE,
} ChassisPowerTransientMode_e;

typedef struct {
    float eta, fast_gain, release_w_per_s, stale_release_w_per_s;
    uint32_t meter_fresh_ms, feedback_fresh_ms, release_confirm_ms, stable_run_ms;
} ChassisPowerTransientConfig_s;
typedef struct {
    float budget_w, target_w, slow_w, meter_w;
    float raw_current[4], actual_rpm[4], target_rpm[4];
    uint32_t now_ms, meter_tick_ms, feedback_tick_ms[4], feedback_sequence[4];
    uint8_t online_mask, enabled_mask;
    bool meter_enabled, meter_online, control_enabled;
} ChassisPowerTransientInput_s;
typedef struct {
    float fast_w, sent_current[4];
    uint32_t meter_tick_ms, below_since_ms, run_since_ms;
    uint8_t fresh_samples, below_samples;
    bool meter_initialized, below_valid, run_valid;
    ChassisPowerTransientMode_e mode;
} ChassisPowerTransientState_s;
typedef struct {
    float fast_w, slow_applied_w, model_budget_w, effective_w;
    uint8_t drive_mask, brake_mask;
    bool learning_allowed, positive_allowed, meter_fresh, feedback_fresh, timing_valid, fault;
    ChassisPowerTransientMode_e mode;
} ChassisPowerTransientOutput_s;

ChassisPowerTransientConfig_s ChassisPowerTransientDefaultConfig(void);
void ChassisPowerTransientReset(ChassisPowerTransientState_s *state);
bool ChassisPowerTransientUpdate(const ChassisPowerTransientConfig_s *config,
    const ChassisPowerTransientInput_s *input, float dt_s,
    ChassisPowerTransientState_s *state, ChassisPowerTransientOutput_s *output);
void ChassisPowerTransientRecordSent(ChassisPowerTransientState_s *state, const float current[4]);
#endif
