#include "chassis_power_internal.h"

static ChassisPowerInstance_s storage;
static ChassisPowerInstance_s *power_instance;
ChassisPowerInstance_s *ChassisPowerPrivate(void) { return power_instance; }

static uint32_t HashU32(uint32_t hash, uint32_t value)
{
    for (unsigned byte=0;byte<4;++byte) {
        hash=(hash^(value&255U))*UINT32_C(16777619);
        value>>=8;
    }
    return hash;
}

static uint32_t HashFloat(uint32_t hash, float value)
{
    uint32_t bits;
    _Static_assert(sizeof(float)==sizeof(uint32_t), "Configuration fingerprint needs float32");
    memcpy(&bits,&value,sizeof bits);
    return HashU32(hash,bits);
}

uint32_t ChassisPowerConfigFingerprint(const ChassisPower_Init_Config_s *c)
{
    if(!c) return 0;
    uint32_t hash=UINT32_C(2166136261);
    /* Canonical format 1: declaration order within each block, four models in
     * LF/RF/LB/RB order, float32 bits or integer/bool widened to u32 LE.
     * tuning_source is separate evidence, not part of the numerical identity. */
    hash=HashU32(hash,1);
#define VOFA_POWER_FLOAT(member) hash=HashFloat(hash,c->member);
#define VOFA_POWER_U32(member) hash=HashU32(hash,(uint32_t)c->member);
#include "chassis_power_config_fields.def"
#undef VOFA_POWER_FLOAT
#undef VOFA_POWER_U32
    return hash;
}

static ChassisPowerConfigObservation_s ObserveConfig(const ChassisPower_Init_Config_s *config)
{
    return (ChassisPowerConfigObservation_s){.fingerprint=ChassisPowerConfigFingerprint(config),
        .tuning_source=config->tuning_source,.available=true,.outer=config->outer};
}

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
    storage.config_observation=storage.slow_observation=ObserveConfig(config);
    storage.input.attenuation=1;
    ChassisPowerBudgetReset(&storage.budget_state,0);
    storage.state.registered=true;
    power_instance=&storage;
    return true;
}

bool ChassisPowerRequestConfig(const ChassisPower_Init_Config_s *config,uint32_t version)
{
    if (chassis_power == NULL || !ChassisPowerConfigValid(config)) return false;
    const ChassisPowerConfigObservation_s observation=ObserveConfig(config);
    POWER_LOCK();
    const uint32_t previous=chassis_power->config_pending ? chassis_power->pending_version : chassis_power->slow_version;
    const uint32_t advance=version-previous;
    const bool valid=advance!=0 && advance<UINT32_C(0x80000000);
    if (valid) {
        chassis_power->pending_config=*config;
        chassis_power->pending_observation=observation;
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
