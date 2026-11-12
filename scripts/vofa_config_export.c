#include "chassis_power_internal.h"
#include "../config/20_chassis_power/chassis_power_defaults.h"
#include "../config/31_telemetry/tuning_overrides.h"
#include <stdio.h>
#include <stdlib.h>

static bool Word(const char *text,uint32_t *value)
{
    if(strlen(text)!=8) return false;
    char *end;
    const unsigned long parsed=strtoul(text,&end,16);
    if(*end) return false;
    *value=(uint32_t)parsed;
    return true;
}

int main(int argc,char **argv)
{
    ChassisPower_Init_Config_s c=ChassisPowerSpeedDefaultConfig();
    float *tuning[]={&c.outer.kp,&c.outer.ki,&c.outer.measurement_lpf_tau_s,
        &c.outer.activation_margin_w,&c.outer.error_deadband_w,&c.outer.max_positive_correction_w,
        &c.outer.positive_slew_rate_w_per_s,&c.outer.budget_drop_reset_w,&c.outer.antiwindup_gain_per_s,
        &c.outer.low_demand_delay_s,&c.outer.low_demand_decay_w_per_s,
        &c.budget.max_boost_w,&c.budget.rise_rate_w_per_s,&c.budget.demand_margin_w,
        &c.budget.empty_energy_ratio,&c.budget.full_energy_ratio,&c.budget.buffer.max_boost_w,
        &c.budget.buffer.recovery_power_w,&c.budget.buffer.recovery_target_time_s};
    if(argc==2 && !strcmp(argv[1],"--file")) {
        const float values[24]=TUNING_POWER_VALUES;
        if(!TUNING_POWER_OVERRIDE || TUNING_POWER_MODE!=c.stage) return 2;
        for(unsigned n=0;n<19;++n) *tuning[n]=values[n];
    } else if(argc==22 && !strcmp(argv[1],"--tuning")) {
        if(strtoul(argv[2],NULL,10)!=c.stage) return 2;
        for(unsigned n=0;n<19;++n) {
            uint32_t bits;
            if(!Word(argv[n+3],&bits)) return 2;
            memcpy(tuning[n],&bits,sizeof bits);
        }
    } else if(argc>1 && !strcmp(argv[1],"--words")) {
        const unsigned count=0
#define VOFA_POWER_FLOAT(member) +1
#define VOFA_POWER_U32(member) +1
#include "../modules/chassis_power/chassis_power_config_fields.def"
#undef VOFA_POWER_FLOAT
#undef VOFA_POWER_U32
        ;
        if((unsigned)argc!=count+2) return 2;
        unsigned index=2;
        uint32_t bits;
#define VOFA_POWER_FLOAT(member) if(!Word(argv[index++],&bits)) return 2; memcpy(&c.member,&bits,4);
#define VOFA_POWER_U32(member) if(!Word(argv[index++],&bits)) return 2; c.member=bits; if((uint32_t)c.member!=bits) return 2;
#include "../modules/chassis_power/chassis_power_config_fields.def"
#undef VOFA_POWER_FLOAT
#undef VOFA_POWER_U32
    } else if(argc!=1) return 2;
    if(!ChassisPowerConfigValid(&c)) return 3;
    printf("{\"fingerprint\":%lu,\"parameters\":{",(unsigned long)ChassisPowerConfigFingerprint(&c));
    const char *separator="";
#define VOFA_POWER_FLOAT(member) printf("%s\"%s\":%.9g",separator,#member,(double)c.member); separator=",";
#define VOFA_POWER_U32(member) printf("%s\"%s\":%lu",separator,#member,(unsigned long)c.member); separator=",";
#include "../modules/chassis_power/chassis_power_config_fields.def"
#undef VOFA_POWER_FLOAT
#undef VOFA_POWER_U32
    printf("},\"power_values\":[");
    for(unsigned n=0;n<19;++n) printf("%s%.9g",n ? ",":"",(double)*tuning[n]);
    puts("]}");
    return 0;
}
