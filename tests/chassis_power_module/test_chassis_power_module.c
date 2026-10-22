#include "chassis_power.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static unsigned test_stage=3, test_flags=2;

unsigned BSPLogWrite(const void *data,unsigned length) {(void)data;return length;}
unsigned BSPLogWriteString(const char *text) {return (unsigned)strlen(text);}

static ChassisPower_Init_Config_s Config(void)
{
    ChassisPower_Init_Config_s c = {0};
    for (unsigned w=0;w<4;++w) c.models[w]=(MotorPowerModelConfig_s){.current_conversion=1,.k4=.000001f};
    c.algorithm=(ChassisPowerAlgorithmConfig_s){.safety_factor=.95f,.max_current_command=15000,
        .small_error_threshold_rpm=500,.reserved_power_threshold_w=54,.per_motor_reserved_power_w=8};
    c.transient=ChassisPowerTransientDefaultConfig();
    c.budget=(ChassisPowerBudgetConfig_s){.empty_energy_ratio=.1f,.full_energy_ratio=.3f,
        .max_boost_w=100,.rise_rate_w_per_s=200,.demand_margin_w=5};
    c.budget.buffer=(ChassisBufferBudgetConfig_s){.max_energy_j=60,.recovery_enter_j=15,
        .recovery_exit_j=25,.hard_floor_j=5,.prediction_window_s=.1f,.guard_window_s=.1f,
        .settlement_delay_s=.05f,.energy_margin_j=2,.power_margin_w=2,.max_boost_w=100,
        .recovery_power_w=10,.recovery_target_time_s=2,.observer_max_step_s=.02f,
        .recovery_hold_s=.1f,.rise_rate_w_per_s=200,.safety_factor=.95f,.correction_rise_j_per_s=10};
    c.outer=(ChassisPowerOuterLoopConfig_s){.kp=1,.ki=1.5f,.measurement_lpf_tau_s=.03f,
        .activation_margin_w=5,.error_deadband_w=.3f,.max_positive_correction_w=8,
        .positive_slew_rate_w_per_s=60,.budget_drop_reset_w=1,.antiwindup_gain_per_s=10,
        .low_demand_delay_s=.5f,.low_demand_decay_w_per_s=1};
    c.stage=(uint8_t)test_stage;c.meter_enabled=(test_flags&2)!=0;
    c.use_supercap=(test_flags&1)!=0;c.buffer_enabled=(test_flags&4)!=0;
    return c;
}

int main(int argc,char **argv)
{
    if(argc==1) {
        char executable[128];
        snprintf(executable,sizeof executable,"%s",argv[0]);
#ifdef _WIN32
        for(char *p=executable;*p;++p) if(*p=='/') *p='\\';
#endif
        for(unsigned stage=0;stage<4;++stage) for(unsigned flags=0;flags<8;++flags) {
            char command[256];
            snprintf(command,sizeof command,"\"%s\" %u %u",executable,stage,flags);
            if(system(command)!=0) return 1;
        }
        puts("PASS: 32 source/meter/buffer/stage combinations");return 0;
    }
    assert(argc==3);
    test_stage=(unsigned)strtoul(argv[1],NULL,10);test_flags=(unsigned)strtoul(argv[2],NULL,10);
    ChassisPowerCycleOutput_s output;
    memset(&output,0xFF,sizeof output);
    assert(!ChassisPowerLimit(NULL,&output));
    for(unsigned w=0;w<4;++w) assert(output.current[w]==0);
    ChassisPower_Init_Config_s config=Config();
    assert(!ChassisPowerInit(NULL));
    config.models[0].current_conversion=0;
    assert(!ChassisPowerInit(&config));
    config=Config();assert(ChassisPowerInit(&config));
    assert(!ChassisPowerInit(&config));
    ChassisPowerPolicyInput_s policy_input={.now_ms=100,.dt_s=.005f,
        .bench_enabled=true,.bench_limit_w=20};
    ChassisPowerPolicyOutput_s policy_output;
    assert(ChassisPowerUpdatePolicy(&policy_input,&policy_output));
    assert(policy_output.valid && policy_output.control.control_enabled);
    assert(policy_output.budget.applied_budget_w==20);
    ChassisPowerCycleInput_s input={.now_ms=100,.dt_s=.002f,.online_mask=15,.enabled_mask=15,
        .control={.version=42,.tick_ms=100,.control_enabled=true,.nominal_budget_w=20,
            .target_power_w=19,.legacy_budget_w=20},
        .meter={.online=true,.last_rx_tick_ms=100,.power_w=19}};
    for(unsigned w=0;w<4;++w) {
        input.raw_current[w]=5000;input.actual_rpm[w]=input.target_rpm[w]=1000;
        input.feedback_tick_ms[w]=100;input.feedback_sequence[w]=1;
    }
    assert(ChassisPowerLimit(&input,&output));
    assert(output.valid && output.control_version==42);
    for(unsigned w=0;w<4;++w) assert(isfinite(output.current[w]) && output.current[w]>=0 && output.current[w]<5000);
    /* Safety after limiting must replace all final diagnostics with actual zeros. */
    ChassisPowerFinalInput_s final={.now_ms=100,.control_version=42,.can_submitted=true};
    assert(ChassisPowerRecordFinal(&final));
    ChassisPowerSnapshot_s snapshot;
    assert(ChassisPowerGetSnapshot(&snapshot));
    assert(snapshot.final_valid && snapshot.can_submitted);
    for(unsigned w=0;w<4;++w) assert(snapshot.final_current[w]==0 && snapshot.telemetry.current_command[w]==0);
    final.control_version=41;assert(!ChassisPowerRecordFinal(&final));
    /* The slow publication switches complete configuration versions at a boundary. */
    ChassisPower_Init_Config_s next=config;
    next.algorithm.max_current_command=NAN;
    assert(!ChassisPowerRequestConfig(&next,7));
    next=config;next.algorithm.max_current_command=40000;
    assert(!ChassisPowerRequestConfig(&next,7));
    next=config;next.outer.antiwindup_gain_per_s=NAN;
    assert(!ChassisPowerRequestConfig(&next,7));
    next=config;next.outer.low_demand_decay_w_per_s=0;
    assert(!ChassisPowerRequestConfig(&next,7));
    next=config;next.algorithm.max_current_command=1000;
    assert(ChassisPowerRequestConfig(&next,7));
    assert(!ChassisPowerRequestConfig(&next,7));
    assert(!ChassisPowerRequestConfig(&next,6));
    assert(ChassisPowerLimit(&input,&output)); /* Slow boundary has not consumed version 7. */
    assert(ChassisPowerGetSnapshot(&snapshot));
    assert(snapshot.final_valid && snapshot.control_version==42); /* Last complete cycle stays visible. */
    policy_input.previous_valid=ChassisPowerGetSnapshot(&policy_input.previous);
    policy_input.now_ms=101;
    assert(ChassisPowerUpdatePolicy(&policy_input,&policy_output));
    assert(policy_output.control.config_version==7);
    assert(ChassisPowerGetSnapshot(&snapshot));
    assert(snapshot.control_version==42 && snapshot.policy.control.config_version==7);
    assert(ChassisPowerLimit(&input,&output)); /* An in-flight version 0 still uses old config. */
    input.control.config_version=7;
    assert(ChassisPowerLimit(&input,&output));
    for(unsigned w=0;w<4;++w) assert(output.current[w]<=1000);
    input.control.config_version=0;
    assert(!ChassisPowerLimit(&input,&output)); /* Do not silently use new config for old input. */
    input.control.config_version=7;
    input.raw_current[1]=NAN;assert(!ChassisPowerLimit(&input,&output));
    for(unsigned w=0;w<4;++w) assert(output.current[w]==0);
    input.raw_current[1]=5000;input.dt_s=0;assert(!ChassisPowerLimit(&input,&output));
    input.dt_s=.002f;input.now_ms=111;assert(!ChassisPowerLimit(&input,&output));
    assert(!ChassisPowerGetSnapshot(NULL));
    puts("PASS: standalone chassis power module tests");
    return 0;
}
