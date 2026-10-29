#ifndef CHASSIS_POWER_SHADOW_H
#define CHASSIS_POWER_SHADOW_H
#include "chassis_power_types.h"
#include "../predictive_power/predictive_power_config.h"
#include "../predictive_power/power_observer.h"
#include "../predictive_power/power_supervisor.h"
#include "../predictive_power/energy_scheduler.h"
typedef struct {
    PPEnergyPlan plan;
    PPEnergyTrace trace;
    uint32_t config_version,elapsed_us;
    PPReason reason;
} ChassisPowerShadowEnergySnapshot;
typedef struct {
    uint32_t tick_ms,control_version,config_version,model_version,meter_sequence;
    uint32_t elapsed_us,max_elapsed_us,cycles,overruns;
    float dt_s,measured_w;
    PPReason reason;
    bool dynamic_model,baseline_envelope,eligible;
    PPPrediction prediction;
    PPAllocationResult allocation;
    PPFinalActuation final;
    uint32_t energy_created_ms;
    bool energy_valid;
} ChassisPowerShadowSnapshot;
/* Pending model is consumed atomically by the fast task. This configures shadow
 * analysis only; checksums verify integrity, not physical calibration. Feedback
 * is in the DJI raw equivalent units unless caller supplies a verified scale. */
bool ChassisPowerShadowSetModel(const PPModel *model);
bool ChassisPowerShadowGet(ChassisPowerShadowSnapshot *snapshot);
void ChassisPowerShadowCycle(const ChassisPowerCycleInput_s *,const ChassisPower_Init_Config_s *,
                            const ChassisPowerCycleOutput_s *legacy);
void ChassisPowerShadowRecord(const ChassisPowerFinalInput_s *);
/* Slow-task only, at most once per 10 ms. No write access to live policy. */
void ChassisPowerShadowEnergyUpdate(const ChassisPowerPolicyInput_s *,const ChassisPower_Init_Config_s *,
                                    const ChassisPowerPolicyOutput_s *);
bool ChassisPowerShadowEnergyGet(ChassisPowerShadowEnergySnapshot *);
#endif
