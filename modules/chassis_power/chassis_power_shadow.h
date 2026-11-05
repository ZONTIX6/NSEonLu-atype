#ifndef CHASSIS_POWER_SHADOW_H
#define CHASSIS_POWER_SHADOW_H
#include "chassis_power_types.h"
#include "../predictive_power/predictive_power_config.h"
#include "../predictive_power/power_observer.h"
#include "../predictive_power/power_supervisor.h"
#include "../predictive_power/energy_scheduler.h"
typedef struct {
    PPWheelEstimate corrected[4], current[4];
    float future_omega[3][4], future_p00[3][4];
    uint32_t accepted_age_us[4], corrected_count[4], skipped_events[4];
    uint32_t kf_elapsed_us, consecutive_overruns;
    uint8_t future_valid_mask[3];
    bool configured, paused;
} ChassisWheelKFDiagnostics;

typedef struct {
    PPEnergyPlan plan;
    PPEnergyTrace trace;
    uint32_t config_version,elapsed_us;
    PPReason reason;
} ChassisPowerShadowEnergySnapshot;
typedef struct {
    uint32_t tick_ms,control_version,config_version,model_version,meter_sequence,model_checksum;
    uint32_t elapsed_us,max_elapsed_us,cycles,overruns;
    float dt_s,measured_w;
    PPReason reason;
    bool dynamic_model,baseline_envelope,eligible,measured_valid;
    PPPrediction prediction;
    PPAllocationResult allocation;
    PPFinalActuation final;
    uint32_t energy_created_ms;
    bool energy_valid;
    ChassisWheelKFDiagnostics wheel_kf;
} ChassisPowerShadowSnapshot;
/* Pending model is consumed atomically by the fast task. This configures shadow
 * analysis only; checksums verify integrity, not physical calibration. Feedback
 * is in the DJI raw equivalent units unless caller supplies a verified scale. */
bool ChassisPowerShadowSetWheelKFConfig(const PPWheelKFConfig *);
/* Motor-task only: checkpoint precedes this task's next correction. */
bool ChassisPowerShadowGetWheelKFCheckpoint(PPWheelKF out[4], PPWheelKFConfig *config);
void ChassisPowerShadowRecordElapsed(uint32_t elapsed_us);
/* Motor-task only, reject an Arm while a publication awaits the cycle boundary. */
bool ChassisPowerShadowVersions(uint32_t *model_version,uint32_t *kf_version);
void ChassisPowerShadowResume(void);
void ChassisPowerShadowEnergyResume(void);
bool ChassisPowerShadowSetModel(const PPModel *model);
bool ChassisPowerShadowGet(ChassisPowerShadowSnapshot *snapshot);
void ChassisPowerShadowCycle(const ChassisPowerCycleInput_s *,const ChassisPower_Init_Config_s *,
                            const ChassisPowerCycleOutput_s *legacy);
void ChassisPowerShadowRecord(const ChassisPowerFinalInput_s *);
/* Slow-task only, at most once per 10 ms. No write access to live policy. */
void ChassisPowerShadowEnergyUpdate(const ChassisPowerPolicyInput_s *,const ChassisPower_Init_Config_s *,
                                    const ChassisPowerPolicyOutput_s *);
bool ChassisPowerShadowEnergyGet(ChassisPowerShadowEnergySnapshot *);
void ChassisPowerShadowEnergyTiming(uint32_t *elapsed_us,uint32_t *strategy_version);
#endif
