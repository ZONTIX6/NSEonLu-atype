#include "power_supervisor.h"
#include <stddef.h>
bool PPSupervise(const PPSupervisorInput *i,const PPAllocationProblem *p,const PPAllocationResult *r,PPSupervisorOutput *o)
{
    if(!o) return false;
    *o=(PPSupervisorOutput){.mode=PP_DISABLED,.reason=PP_INVALID_INPUT};if(!i || !p) return false;
    if(!i->permitted || i->hard_fault) {o->mode=i->hard_fault ? PP_FAULT : PP_DISABLED;o->reason=PP_NO_PERMISSION;return false;}
    o->mode=PP_DEGRADED;
    if(!i->model_valid) {o->reason=PP_NO_MODEL;return false;}
    if((uint32_t)(i->now_ms-i->allowance_ms)>=20) {o->reason=PP_STALE_ALLOWANCE;return false;}
    if(i->strategy_version!=i->allowance_strategy_version ||
        i->model_version!=i->allowance_model_version || i->config_version!=i->allowance_config_version ||
        i->capability_version!=i->allowance_capability_version) {o->reason=PP_VERSION_MISMATCH;return false;}
    o->reason=i->elapsed_us>300 ? PP_TIMEOUT : PP_INFEASIBLE;
    if(i->elapsed_us<=300 && r && r->feasible && PPAllocationCheck(p,r->command,&o->power_w)) {
        for(unsigned w=0;w<4;++w) o->command[w]=r->command[w];
        o->mode=PP_BASE_ONLY;o->reason=PP_OK;o->feasible=true;return true;
    }
    o->fallback=true;
    if(i->baseline_valid && PPAllocationCheck(p,i->baseline,&o->power_w)) {
        for(unsigned w=0;w<4;++w) o->command[w]=i->baseline[w];
        o->feasible=true;return true;
    }
    /* Zero drive is an action, never an assertion of zero physical consumption. */
    o->feasible=PPAllocationCheck(p,o->command,&o->power_w);
    return o->feasible;
}
