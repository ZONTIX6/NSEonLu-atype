#include "chassis_force_config.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static uint32_t hash_word(uint32_t h,uint32_t v)
{
    for(unsigned k=0;k<4;++k) {h=(h^(v&255U))*UINT32_C(16777619);v>>=8;}
    return h;
}
static uint32_t hash_float(uint32_t h,float v)
{
    uint32_t bits;memcpy(&bits,&v,4);return hash_word(h,bits);
}
uint32_t ChassisForceConfigChecksum(const ChassisForceConfig_s *c)
{
    if(!c) return 0;
    uint32_t h=hash_word(hash_word(hash_word(UINT32_C(2166136261),c->backend),c->schema_version),c->calibration_valid);
    for(unsigned w=0;w<4;++w) {
        const ChassisForceWheel_s *p=&c->wheel[w];
        const float v[]={p->x_m,p->y_m,p->dx,p->dy,p->radius_m,p->reduction,p->k_out_nm_a,
            p->current_limit,p->force_limit_n,p->current_slew};
        for(unsigned k=0;k<sizeof v/sizeof v[0];++k) h=hash_float(h,v[k]);
    }
    const float v[]={c->force_max_n,c->spin_torque_nm,c->spin_start_torque_nm,c->spin_torque_max_nm,
        c->force_rise_n_s,c->force_fall_n_s,c->torque_rise_nm_s,c->torque_fall_nm_s,
        c->velocity_limit_mps,c->omega_limit_rad_s,c->wheel_limit_rpm,c->resume_ratio,
        c->input_deadzone,c->input_gamma,c->brake_torque_nm,c->brake_stop_rad_s,
        c->power_gain_nm_per_w_s,c->power_deadband_w,c->power_lpf_tau_s,c->torque_probe_step_nm,
        c->power_request_step_w,c->stall_current_command,c->stall_rpm,c->temp_derate_c,
        c->temp_stop_c,c->speed_prediction_rpm,c->startup_current_limit};
    for(unsigned k=0;k<sizeof v/sizeof v[0];++k) h=hash_float(h,v[k]);
    h=hash_word(h,c->brake_timeout_ms);h=hash_word(h,c->power_recovery_samples);
    h=hash_word(h,c->grant_timeout_ms);return hash_word(h,c->stall_timeout_ms);
}
ChassisForceConfig_s ChassisForceDefaultConfig(void)
{
    ChassisForceConfig_s c={.backend=1,.schema_version=1,
        .calibration_valid=CHASSIS_FORCE_CALIBRATION_VALID!=0,.resume_ratio=.9f,
        .input_deadzone=.05f,.input_gamma=1.5f,.power_gain_nm_per_w_s=.01f,
        .power_deadband_w=2,.power_lpf_tau_s=.02f,.power_recovery_samples=3,
        .torque_probe_step_nm=.05f,.power_request_step_w=5,.grant_timeout_ms=50};
    c.checksum=ChassisForceConfigChecksum(&c);return c;
}
bool ChassisForceConfigValid(const ChassisForceConfig_s *c)
{
    if(!c || c->backend!=1 || c->schema_version!=1 || !c->calibration_valid ||
        c->checksum!=ChassisForceConfigChecksum(c)) return false;
    const float positive[]={c->force_max_n,c->spin_torque_nm,c->spin_start_torque_nm,c->spin_torque_max_nm,
        c->force_rise_n_s,c->force_fall_n_s,c->torque_rise_nm_s,c->torque_fall_nm_s,
        c->velocity_limit_mps,c->omega_limit_rad_s,c->wheel_limit_rpm,c->input_gamma,
        c->power_gain_nm_per_w_s,c->power_deadband_w,c->power_lpf_tau_s,c->torque_probe_step_nm,
        c->power_request_step_w,c->stall_current_command,c->stall_rpm,c->speed_prediction_rpm,c->startup_current_limit};
    for(unsigned k=0;k<sizeof positive/sizeof positive[0];++k)
        if(!isfinite(positive[k]) || positive[k]<=0) return false;
    if(!isfinite(c->resume_ratio) || c->resume_ratio<=0 || c->resume_ratio>=1 ||
        !isfinite(c->input_deadzone) || c->input_deadzone<0 || c->input_deadzone>=1 ||
        !isfinite(c->temp_derate_c) || !isfinite(c->temp_stop_c) || c->temp_derate_c<=0 ||
        c->temp_stop_c<=c->temp_derate_c || c->temp_stop_c>100 ||
        c->spin_start_torque_nm>c->spin_torque_max_nm || c->spin_torque_nm>c->spin_torque_max_nm ||
        c->power_recovery_samples<3 || c->power_recovery_samples>20 ||
        c->grant_timeout_ms<20 || c->grant_timeout_ms>1000 ||
        c->stall_timeout_ms==0 || c->stall_timeout_ms>5000 || c->stall_current_command>16384 ||
        c->startup_current_limit>16384 || c->torque_probe_step_nm>c->spin_torque_max_nm ||
        c->power_request_step_w>20) return false;
#if CHASSIS_FORCE_EXIT_MODE == CHASSIS_FORCE_EXIT_BRAKE
    if(!isfinite(c->brake_torque_nm) || c->brake_torque_nm<=0 || c->brake_torque_nm>c->spin_torque_max_nm ||
        !isfinite(c->brake_stop_rad_s) || c->brake_stop_rad_s<=0 || c->brake_stop_rad_s>=c->omega_limit_rad_s ||
        c->brake_timeout_ms==0 || c->brake_timeout_ms>5000) return false;
#endif
    for(unsigned w=0;w<4;++w) {
        const ChassisForceWheel_s *p=&c->wheel[w];
        const float v[]={p->x_m,p->y_m,p->dx,p->dy,p->radius_m,p->reduction,p->k_out_nm_a,
            p->current_limit,p->force_limit_n,p->current_slew};
        for(unsigned k=0;k<sizeof v/sizeof v[0];++k) if(!isfinite(v[k])) return false;
        if(fabsf(p->dx*p->dx+p->dy*p->dy-1)>.001f || p->radius_m<=0 || p->reduction<=0 ||
            p->k_out_nm_a<=0 || p->current_limit<=0 || p->current_limit>16384 ||
            p->force_limit_n<=0 || p->current_slew<=0) return false;
    }
    return true;
}
