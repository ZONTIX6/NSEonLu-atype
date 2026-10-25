#include "chassis_force_spin_power.h"
#include <math.h>
#include <stddef.h>
bool ChassisForceSpinPowerUpdate(const ChassisForceConfig_s *c,const ChassisForceSpinPowerInput_s *i,
    ChassisForceSpinPowerState_s *s,float *torque)
{
    if(!c || !i || !s || !torque) return false;
    s->growth_permitted=false;
    const float dt=isfinite(i->dt_s) && i->dt_s>0 && i->dt_s<=.005f ? i->dt_s : .002f;
    if(!i->enabled || !i->last_submission_ok || !isfinite(i->target_w) || i->target_w<=0) {
        *s=(ChassisForceSpinPowerState_s){0};*torque=0;return true;
    }
    const bool fresh=i->meter_valid && (uint32_t)(i->now_ms-i->meter_tick_ms)<=20 &&
        isfinite(i->load_w) && i->load_w>=-c->power_deadband_w;
    if(!fresh) {
        s->fresh_samples=0;s->initialized=false;
        s->torque_nm=fmaxf(0,s->torque_nm-c->torque_fall_nm_s*dt);
        *torque=s->torque_nm;return true;
    }
    if(s->initialized && i->meter_tick_ms==s->meter_tick_ms) {
        s->growth_permitted=i->growth_allowed && s->fresh_samples>=c->power_recovery_samples;
        *torque=s->torque_nm;return true;
    }
    const uint32_t elapsed=i->meter_tick_ms-s->meter_tick_ms;
    const bool continuous=s->initialized && elapsed>0 && elapsed<=20;
    const float sample_dt=continuous ? elapsed*.001f : 0;
    if(!continuous) {
        s->filtered_w=fmaxf(0,i->load_w);s->fresh_samples=1;
    } else {
        s->filtered_w+=sample_dt/(c->power_lpf_tau_s+sample_dt)*(fmaxf(0,i->load_w)-s->filtered_w);
        if(s->fresh_samples<c->power_recovery_samples) ++s->fresh_samples;
    }
    s->initialized=true;s->meter_tick_ms=i->meter_tick_ms;
    s->growth_permitted=i->growth_allowed && s->fresh_samples>=c->power_recovery_samples;
    /* Downward protection observes unfiltered power and budget drops immediately. */
    const bool drop=i->target_w<s->target_w;
    float error=i->target_w-s->filtered_w;
    if(drop || i->load_w>i->target_w+c->power_deadband_w) error=fminf(error,i->target_w-i->load_w);
    s->target_w=i->target_w;
    if(sample_dt>0) {
        const float e=copysignf(fmaxf(0,fabsf(error)-c->power_deadband_w),error);
        float rate=fmaxf(-c->torque_fall_nm_s,fminf(c->torque_rise_nm_s,c->power_gain_nm_per_w_s*e));
        if(rate>0 && !s->growth_permitted) rate=0;
        s->torque_nm=fmaxf(0,fminf(c->spin_torque_max_nm,s->torque_nm+rate*sample_dt));
    }
    /* Conservative start has no minimum-output guarantee and still passes allocation. */
    if(s->torque_nm<c->spin_start_torque_nm && i->growth_allowed && !drop &&
        i->load_w<=i->target_w+c->power_deadband_w && sample_dt>0)
        s->torque_nm=fminf(c->spin_start_torque_nm,s->torque_nm+c->torque_rise_nm_s*sample_dt);
    *torque=s->torque_nm;return isfinite(*torque);
}
void ChassisForceSpinPowerProject(ChassisForceSpinPowerState_s *s,float sent,bool limited)
{
    if(!s) return;
    if(!isfinite(sent) || sent<0) {s->torque_nm=0;s->growth_permitted=false;return;}
    if(limited) s->torque_nm=fminf(s->torque_nm,sent);
}
