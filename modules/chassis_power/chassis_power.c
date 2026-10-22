#include "chassis_power_internal.h"

static ChassisPowerInstance_s storage;
static ChassisPowerInstance_s *power_instance;
ChassisPowerInstance_s *ChassisPowerPrivate(void) { return power_instance; }

bool ChassisPowerConfigValid(const ChassisPower_Init_Config_s *c)
{
    if (c == NULL || c->stage > 3 || !ChassisPowerAlgorithmConfigIsValid(&c->algorithm) ||
        !ChassisPowerBudgetConfigIsValid(&c->budget) ||
        (c->buffer_enabled && !ChassisBufferBudgetConfigIsValid(&c->budget.buffer)) ||
        !ChassisPowerOuterLoopConfigIsValid(&c->outer) || c->algorithm.max_current_command>32767 ||
        !isfinite(c->outer.antiwindup_gain_per_s) || c->outer.antiwindup_gain_per_s<=0 ||
        !isfinite(c->outer.low_demand_delay_s) || c->outer.low_demand_delay_s<0 ||
        !isfinite(c->outer.low_demand_decay_w_per_s) || c->outer.low_demand_decay_w_per_s<=0) return false;
    for (unsigned w=0;w<4;++w) if (!PowerModelConfigIsValid(&c->models[w])) return false;
    ChassisPowerTransientState_s state={0};
    ChassisPowerTransientOutput_s output;
    const ChassisPowerTransientInput_s input={0};
    ChassisPowerTransientConfig_s transient=c->transient;
    transient.eta=c->algorithm.safety_factor;
    transient.max_current_command=c->algorithm.max_current_command;
    return ChassisPowerTransientUpdate(&transient,&input,.002f,&state,&output);
}

bool ChassisPowerInit(const ChassisPower_Init_Config_s *config)
{
    if (chassis_power != NULL || !ChassisPowerConfigValid(config)) return false;
    memset(&storage,0,sizeof storage);
    storage.config=storage.slow_config=*config;
    storage.input.attenuation=1;
    ChassisPowerBudgetReset(&storage.budget_state,0);
    storage.state.registered=true;
    power_instance=&storage;
    return true;
}

bool ChassisPowerRequestConfig(const ChassisPower_Init_Config_s *config,uint32_t version)
{
    if (chassis_power == NULL || !ChassisPowerConfigValid(config)) return false;
    POWER_LOCK();
    const uint32_t previous=chassis_power->config_pending ? chassis_power->pending_version : chassis_power->slow_version;
    const uint32_t advance=version-previous;
    const bool valid=advance!=0 && advance<UINT32_C(0x80000000);
    if (valid) {
        chassis_power->pending_config=*config;
        chassis_power->pending_version=version;
        chassis_power->config_pending=true;
    }
    POWER_UNLOCK();
    return valid;
}

bool ChassisPowerGetSnapshot(ChassisPowerSnapshot_s *output)
{
    if (output == NULL) return false;
    memset(output,0,sizeof *output);
    if (chassis_power == NULL) return false;
    POWER_LOCK();
    *output=chassis_power->state;
    output->policy=chassis_power->policy;
    output->policy_tick_ms=chassis_power->policy_tick_ms;
    POWER_UNLOCK();
    return true;
}
