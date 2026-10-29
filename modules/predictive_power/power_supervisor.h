#ifndef POWER_SUPERVISOR_H
#define POWER_SUPERVISOR_H
#include "power_allocator.h"
typedef enum {PP_DISABLED,PP_BASE_ONLY,PP_ASSIST_READY,PP_ASSIST_ACTIVE,PP_RECOVERING,PP_DEGRADED,PP_FAULT} PPMode;
typedef enum {PP_OK,PP_NO_PERMISSION,PP_NO_MODEL,PP_STALE_ALLOWANCE,PP_VERSION_MISMATCH,
    PP_TIMEOUT,PP_INFEASIBLE,PP_INVALID_INPUT} PPReason;
typedef struct {
    uint32_t now_ms,allowance_ms,model_version,allowance_model_version;
    uint32_t config_version,allowance_config_version,capability_version,allowance_capability_version,elapsed_us;
    uint32_t strategy_version,allowance_strategy_version;
    bool permitted,hard_fault,model_valid,baseline_valid;
    int16_t baseline[4];
} PPSupervisorInput;
typedef struct {
    int16_t command[4];
    PPMode mode;PPReason reason;
    float power_w;
    bool feasible,fallback;
} PPSupervisorOutput;
bool PPSupervise(const PPSupervisorInput *,const PPAllocationProblem *,const PPAllocationResult *,PPSupervisorOutput *);
#endif
