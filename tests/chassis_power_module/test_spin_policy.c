#include "chassis_power.h"
#include "chassis_spin.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
unsigned BSPLogWrite(const void *data,unsigned length) {(void)data;return length;}
unsigned BSPLogWriteString(const char *text) {return (unsigned)strlen(text);}
static void Near(float a,float b) {assert(isfinite(b) && fabsf(a-b)<.02f);}

/* Exercise idle -> real motor demand -> base confirmation -> buffer assist through
 * all production controllers. The independent motor has finite inertia and drag. */
static void ClosedLoop(const ChassisPower_Init_Config_s *config)
{
    const ChassisSpinConfig_s spin={.fixed_wz=6000,.min_wz=1000,.max_wz=8000,
        .gain_wz_per_w_s=80,.rise_wz_per_s=1200,.fall_wz_per_s=2400,.power_lpf_tau_s=.05f,
        .power_deadband_w=1,.settle_time_s=.05f,.budget_drop_reset_w=1};
    ChassisSpinState_s state={0};
    float rpm=0,power=0,energy=60,target=0;
    bool saw_base=false,saw_assist=false,saw_demand=false;
    for(unsigned n=0;n<2000;++n) {
        const uint32_t tick=2000+n*5;
        ChassisPowerPolicyInput_s i={.now_ms=tick,.dt_s=.005f,.adaptive_spin=true,
            .meter={.online=true,.power_w=power,.last_rx_tick_ms=tick},
            .referee={.valid=true,.chassis_output_valid=true,.chassis_output_enabled=true,
                .chassis_power_limit_w=80,.buffer_energy_j=(uint16_t)energy,.buffer_tick_ms=tick}};
        i.previous_valid=ChassisPowerGetSnapshot(&i.previous);
        ChassisPowerPolicyOutput_s p;
        assert(ChassisPowerUpdatePolicy(&i,&p));
        if(p.budget.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK && power>75 && power<=80) saw_base=true;
        if(p.budget.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST) {assert(saw_base);saw_assist=true;}
        ChassisSpinInput_s si={.spin_enabled=true,.control_enabled=p.control.control_enabled,
            .feedback_valid=i.previous_valid && i.previous.online_mask==15,
            .increase_allowed=ChassisSpinTransientAllowsIncrease(&i.previous.transient) &&
                p.budget.spin_phase!=CHASSIS_SPIN_POWER_DEGRADED,
            .target_power_w=p.control.target_power_w,.measured_power_w=power,
            .power_limited=i.previous.base_limited,.energy_recovering=p.budget.recovering,
            .evaluate_tracking=true,.model_limited=i.previous.base_limited,
            .guard_limited=!ChassisSpinGuardAllowsIncrease(&i.previous.transient),
            .braking=i.previous.transient.brake_mask!=0,.now_ms=tick};
        for(unsigned w=0;w<4;++w) {
            si.actual_rpm[w]=i.previous.telemetry.actual_speed_rpm[w];
            si.target_rpm[w]=i.previous.telemetry.target_speed_rpm[w];
            si.feedback_tick_ms[w]=i.previous.feedback_tick_ms[w];
            si.feedback_sequence[w]=i.previous.feedback_sequence[w];
        }
        const float before=target;
        assert(ChassisSpinUpdate(&spin,&si,.005f,&state,&target));
        assert(fabsf(target-before)<=12.01f);
        ChassisPowerCycleInput_s fast={.control=p.control,.now_ms=tick,.dt_s=.005f,
            .online_mask=15,.enabled_mask=15,.meter=i.meter};
        for(unsigned w=0;w<4;++w) {
            fast.actual_rpm[w]=rpm;fast.target_rpm[w]=target*.945f;
            fast.raw_current[w]=fminf(15000,fmaxf(-15000,(fast.target_rpm[w]-rpm)*20));
            fast.feedback_tick_ms[w]=tick;fast.feedback_sequence[w]=n+1;
        }
        ChassisPowerCycleOutput_s out;
        assert(ChassisPowerLimit(&fast,&out));
        ChassisPowerFinalInput_s final={.control_version=p.control.version,.now_ms=tick,
            .enabled_mask=15,.can_submitted=true};
        power=0;
        for(unsigned w=0;w<4;++w) {
            final.current_command[w]=(int16_t)out.current[w];
            power+=PowerModelPredict(&config->models[w],final.current_command[w],rpm);
        }
        assert(ChassisPowerRecordFinal(&final));
        assert(power<=p.control.power_ceiling_w+.02f);
        saw_demand |= out.transient.drive_mask!=0;
        rpm+=(final.current_command[0]*.5f-rpm)*.005f/.1f;
        energy=fminf(60,fmaxf(0,energy+(80-power)*.005f));
    }
    assert(saw_demand && saw_base && saw_assist);
}
int main(void)
{
    ChassisPower_Init_Config_s c={.stage=3,.meter_enabled=true,.buffer_enabled=true};
    for(unsigned w=0;w<4;++w) c.models[w]=(MotorPowerModelConfig_s){.current_conversion=1,.k4=.000001f};
    c.algorithm=(ChassisPowerAlgorithmConfig_s){.safety_factor=.98f,.max_current_command=15000,
        .small_error_threshold_rpm=500,.reserved_power_threshold_w=54,.per_motor_reserved_power_w=8};
    c.transient=ChassisPowerTransientDefaultConfig();
    c.budget=(ChassisPowerBudgetConfig_s){.empty_energy_ratio=.1f,.full_energy_ratio=.3f,
        .max_boost_w=100,.rise_rate_w_per_s=200,.demand_margin_w=5,
        .buffer={.max_energy_j=60,.recovery_enter_j=10,.recovery_exit_j=20,.hard_floor_j=5,
            .prediction_window_s=.25f,.guard_window_s=.3f,.settlement_delay_s=.1f,
            .energy_margin_j=3,.power_margin_w=5,.max_boost_w=60,.recovery_power_w=20,
            .recovery_target_time_s=1,.observer_max_step_s=.02f,.recovery_hold_s=.1f,
            .rise_rate_w_per_s=1200,.safety_factor=.98f,.correction_rise_j_per_s=20}};
    c.outer=(ChassisPowerOuterLoopConfig_s){.kp=1,.ki=1.5f,.measurement_lpf_tau_s=.03f,
        .activation_margin_w=5,.error_deadband_w=.3f,.max_positive_correction_w=8,
        .positive_slew_rate_w_per_s=60,.budget_drop_reset_w=1,.antiwindup_gain_per_s=10,
        .low_demand_delay_s=.5f,.low_demand_decay_w_per_s=1};
    assert(ChassisPowerInit(&c));
    ChassisPowerPolicyInput_s i={.now_ms=100,.dt_s=.005f,.adaptive_spin=true,.previous_valid=true,
        .referee={.valid=true,.chassis_output_enabled=true,.chassis_output_valid=true,.chassis_power_limit_w=80,
            .buffer_energy_j=60},.meter={.online=true,.power_w=20},
        .previous={.registered=true,.online_mask=15,.meter_online=true,
            .power={.status=POWER_MODEL_STATUS_OK},.telemetry={.motor_enabled_mask=15},
            .transient={.guard_scale=1,.mode=CHASSIS_POWER_TRANSIENT_RUN,.positive_allowed=true,
                .drive_mask=15,.feedback_fresh=true,.timing_valid=true,.meter_fresh=true}}};
    ChassisPowerPolicyOutput_s o={0};
    for(unsigned n=0;n<200;++n) {
        i.now_ms+=5;i.referee.buffer_tick_ms=i.now_ms;i.meter.last_rx_tick_ms=i.now_ms;
        i.previous.tick_ms=i.previous.meter_tick_ms=i.now_ms;
        i.previous.measured_power_w=i.meter.power_w;
        for(unsigned w=0;w<4;++w) {
            i.previous.feedback_tick_ms[w]=i.now_ms;
            i.previous.telemetry.actual_speed_rpm[w]=1000;
            i.previous.telemetry.target_speed_rpm[w]=1000;
            i.previous.power.predicted_unlimited_power_w[w]=40;
        }
        assert(ChassisPowerUpdatePolicy(&i,&o));
        Near(80,o.control.nominal_budget_w);Near(78.4f,o.control.target_power_w);
    }
    i.meter.power_w=78.4f;
    for(unsigned n=0;n<200;++n) {
        i.now_ms+=5;i.referee.buffer_tick_ms=i.now_ms;i.meter.last_rx_tick_ms=i.now_ms;
        i.previous.tick_ms=i.previous.meter_tick_ms=i.now_ms;i.previous.measured_power_w=78.4f;
        for(unsigned w=0;w<4;++w) i.previous.feedback_tick_ms[w]=i.now_ms;
        assert(ChassisPowerUpdatePolicy(&i,&o));
    }
    assert(o.budget.spin_phase==CHASSIS_SPIN_POWER_BUFFER_ASSIST);
    assert(o.control.target_power_w>80);
    Near(o.control.target_power_w,78.4f+o.budget.spin_buffer_grant_w);
    Near(o.control.power_ceiling_w,80+o.budget.spin_buffer_grant_w);
    ChassisPowerCycleInput_s fast={.control=o.control,.now_ms=i.now_ms,.dt_s=.002f,
        .online_mask=15,.enabled_mask=15,.meter=i.meter};
    fast.control.slow_correction_w=8; /* Model correction cannot create physical extra allowance. */
    for(unsigned w=0;w<4;++w) {
        fast.raw_current[w]=10000;fast.target_rpm[w]=fast.actual_rpm[w]=1000;
        fast.feedback_tick_ms[w]=i.now_ms;fast.feedback_sequence[w]=1;
    }
    ChassisPowerCycleOutput_s current;
    assert(ChassisPowerLimit(&fast,&current));
    float sum=0;
    for(unsigned w=0;w<4;++w) sum+=PowerModelPredict(&c.models[w],current.current[w],1000);
    assert(sum<=o.control.power_ceiling_w+.02f);
    i.dt_s=0;
    assert(!ChassisPowerUpdatePolicy(&i,&o));
    assert(!o.control.control_enabled);
    i.dt_s=.005f;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(o.budget.spin_phase==CHASSIS_SPIN_POWER_BASE_TRACK);
    Near(0,o.budget.spin_buffer_grant_w);
    i.previous.feedback_tick_ms[2]-=30;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(o.budget.spin_phase==CHASSIS_SPIN_POWER_DEGRADED);
    Near(0,o.budget.spin_buffer_grant_w);Near(80,o.control.power_ceiling_w);
    i.zero_force=true;
    assert(ChassisPowerUpdatePolicy(&i,&o));
    assert(!o.control.control_enabled && o.budget.spin_phase==CHASSIS_SPIN_POWER_OFF);
    ClosedLoop(&c);
    puts("PASS: production spin policy, budget conversion, current guard and stale feedback");
    return 0;
}
