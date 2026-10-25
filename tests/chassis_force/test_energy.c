#include "chassis_power_force.h"
#include "chassis_power_internal.h"
#include "chassis_force_telemetry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
void RTTTelemetryWriteChassisPowerRow(const ChassisPowerOutput_s *p,const ChassisPowerTelemetry_s *t,
    const ChassisPowerControlTelemetry_s *c,uint32_t n,float w,bool o,uint32_t m)
{(void)p;(void)t;(void)c;(void)n;(void)w;(void)o;(void)m;}
int main(void)
{
    ChassisPower_Init_Config_s c={.force_control=true,.stage=3,.meter_enabled=true,.buffer_enabled=true,.use_supercap=true,
        .algorithm={.safety_factor=.98f,.max_current_command=15000,.small_error_threshold_rpm=500,
            .reserved_power_threshold_w=54,.per_motor_reserved_power_w=8},
        .transient=ChassisPowerTransientDefaultConfig(),
        .outer={.kp=.5f,.ki=1,.measurement_lpf_tau_s=.02f,.activation_margin_w=5,.error_deadband_w=2,
            .max_positive_correction_w=8,.positive_slew_rate_w_per_s=10,.budget_drop_reset_w=10,
            .antiwindup_gain_per_s=1,.low_demand_delay_s=.1f,.low_demand_decay_w_per_s=10},
        .budget={.empty_energy_ratio=.1f,.full_energy_ratio=.3f,.max_boost_w=100,.rise_rate_w_per_s=200,
            .buffer={.max_energy_j=60,.recovery_enter_j=20,.recovery_exit_j=30,.hard_floor_j=10,
                .prediction_window_s=.1f,.guard_window_s=.1f,.settlement_delay_s=.1f,.energy_margin_j=1,
                .power_margin_w=1,.max_boost_w=20,.recovery_power_w=10,.recovery_target_time_s=2,
                .observer_max_step_s=.1f,.recovery_hold_s=.1f,.rise_rate_w_per_s=100,
                .safety_factor=.98f,.correction_rise_j_per_s=10}}};
    for(unsigned w=0;w<4;++w) c.models[w]=(MotorPowerModelConfig_s){.k4=1,.current_conversion=1000};
    assert(ChassisPowerInit(&c));
    ChassisPowerPolicyInput_s i={.force_backend=true,.dt_s=.005f,.now_ms=100,
        .referee={.valid=true,.chassis_output_enabled=true,.chassis_power_limit_w=50,.buffer_energy_j=50,.buffer_tick_ms=100},
        .cap={.status={.online=true,.output_enabled=true,.energy_ratio=.5f,.available_power_limit_w=150,.last_rx_tick_ms=100},
            .input_power={.online=true,.input_control_active=true,.input_power_target_w=50,.input_power_w=40,.last_rx_tick_ms=100},
            .input_received_frame_count=1},
        .meter={.online=true,.power_w=40,.last_rx_tick_ms=100}};
    ChassisPowerPolicyOutput_s o;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(!o.input_confirmed && !o.force_growth_allowed);
    assert(o.cap_command.referee_power_limit_w==50);
    ChassisPowerCapCommandResult(&o.cap_command,true,100,1,&o);
    i.cap.input_received_frame_count=2; /* A pre-send sample in the same tick is not an ACK. */
    assert(ChassisPowerUpdatePolicy(&i,&o));assert(!o.input_confirmed);
    i.now_ms=105;i.cap.status.last_rx_tick_ms=105;i.cap.input_power.last_rx_tick_ms=105;
    i.cap.input_received_frame_count=2;i.cap.input_power.input_power_target_w=o.cap_command.input_power_target_w;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(o.input_confirmed);
    assert(fabsf(o.physical_load_target_w-o.control.nominal_budget_w*.98f)<.001f);
    i.previous_valid=true;i.force_operating_valid=true;i.force_operating_demand_w=5;
    i.previous=(ChassisPowerSnapshot_s){.registered=true,.cycle_valid=true,.final_valid=true,.can_submitted=true,
        .tick_ms=105,.final_tick_ms=105,.online_mask=15,.telemetry.motor_enabled_mask=15};
    for(unsigned w=0;w<4;++w) i.previous.feedback_tick_ms[w]=105;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(o.budget.applied_budget_w<=50); /* Idle model losses never request stored energy. */
    i.previous.transient.drive_mask=15;i.force_operating_demand_w=52;
    for(unsigned k=0;k<20;++k) assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(o.physical_load_target_w<=52+c.budget.demand_margin_w+.0001f);
    i.now_ms=126;i.meter.last_rx_tick_ms=126;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(!o.input_confirmed && !o.force_growth_allowed); /* 21 ms is stale. */
    ChassisPowerCapCommandResult(&o.cap_command,false,126,2,&o);
    assert(!o.force_growth_allowed && !o.input_confirmed);
    char header[3072],row[3072];ChassisForceCycleOutput_s f={0};ChassisPowerSnapshot_s power={.policy=o};
    assert(ChassisForceTelemetryHeader(header,sizeof header)>0);
    assert(ChassisForceTelemetryRow(&f,&power,&i.cap,&i.referee,row,sizeof row)>0);
    unsigned header_commas=0,row_commas=0;
    for(const char *p=header;*p;++p) header_commas+=*p==',';
    for(const char *p=row;*p;++p) row_commas+=*p==',';
    assert(header_commas==row_commas && strncmp(row,"force-v1,",9)==0);
    assert(ChassisForceTelemetryHeader(header,10)==0);
    assert(ChassisForceTelemetryRow(&f,&power,&i.cap,&i.referee,row,10)==0);
    ChassisForceCycleInput_s cycle={0};
    ChassisPowerControlSnapshot_s control={.nominal_budget_w=40/.98f,.target_power_w=40,
        .power_ceiling_w=40,.tick_ms=126,.control_enabled=true};
    PowerMeterStatus_s meter={.online=true,.power_w=35,.last_rx_tick_ms=126};
    assert(ChassisPowerForceBegin(&control,&meter,126,.002f,&cycle));
    assert(fabsf(cycle.model_budget_w-40)<.0001f);
    control.slow_correction_w=3;
    assert(ChassisPowerForceBegin(&control,&meter,126,.002f,&cycle));
    assert(cycle.model_budget_w>40 && cycle.physical_target_w==40);
    assert(ChassisPowerRequestConfig(&c,1));
    assert(ChassisPowerUpdatePolicy(&i,&o));
    control.config_version=o.control.config_version;
    assert(control.config_version==1 && ChassisPowerForceBegin(&control,&meter,126,.002f,&cycle));
    cycle=(ChassisForceCycleInput_s){.now_ms=200,.online_mask=15,.enabled_mask=15,
        .meter_valid=true,.load_power_w=35,.physical_target_w=40,.model_budget_w=40};
    f=(ChassisForceCycleOutput_s){.valid=true,.can_submitted=true,.drive_mask=15,.limits=CHASSIS_FORCE_LIMIT_ALLOCATION};
    const uint32_t sequence[4]={1,1,1,1};
    assert(ChassisPowerForceRecord(&cycle,&f,sequence));
    cycle.now_ms=301;assert(ChassisPowerForceRecord(&cycle,&f,sequence));
    assert(chassis_power->state.transient.positive_allowed && chassis_power->state.transient.learning_allowed);
    cycle.load_power_w=45;cycle.now_ms=302;assert(ChassisPowerForceRecord(&cycle,&f,sequence));
    cycle.now_ms=403;assert(ChassisPowerForceRecord(&cycle,&f,sequence));
    assert(chassis_power->state.transient.negative_learning_allowed);
    puts("force energy confirmation: OK");return 0;
}
