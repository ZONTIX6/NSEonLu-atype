#include "chassis_force_telemetry.h"
#include "chassis_energy_config.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <string.h>
static const char columns[]="force-v1,tick,command_seq,cycle_seq,config_version,safety_epoch,policy_version,backend,exit,spin,source,calibrated,calibration_checksum,state,faults,limits,can_submitted,dt_us,"
    "fg_x_mN,fg_y_mN,fx_request_mN,fy_request_mN,mz_request_mNm,fx_sent_model_mN,fy_sent_model_mN,mz_sent_model_mNm,"
    "vx_mm_s,vy_mm_s,omega_mrad_s,residual_mm_s,alpha_pm,beta_pm,predicted_cW,track_cW,load_cW,torque_mNm,growth_allowed,offer_mNm,offer_cW,operating_cW,"
    "braking,brake_timed_out,zero_over_budget,command_age_ms,source_age_ms,yaw_age_ms,"
    "raw_lf,raw_rf,raw_lb,raw_rb,allocated_lf,allocated_rf,allocated_lb,allocated_rb,sent_lf,sent_rf,sent_lb,sent_rb,"
    "rpm_lf,rpm_rf,rpm_lb,rpm_rb,feedback_current_lf,feedback_current_rf,feedback_current_lb,feedback_current_rb,"
    "temperature_lf,temperature_rf,temperature_lb,temperature_rb,feedback_age_lf,feedback_age_rf,feedback_age_lb,feedback_age_rb,"
    "nominal_cW,model_allow_cW,source_ceiling_cW,referee_W,buffer_lower_cJ,buffer_recovering,cap_energy_pm,cap_available_W,"
    "input_actual_cW,input_request_W,input_confirmed_W,input_confirmed,input_confirmed_age_ms,cap051_age_ms,cap052_age_ms,cap061_failures,confirmation_timeouts,follow_translation_only,feedback_valid,input_us,estimate_us,command_us,allocation_us,guard_us,can_us\r\n";
typedef struct {char *buffer;size_t used,capacity;bool valid;} Writer;
static void append(Writer *w,const char *format,...)
{
    if(!w->valid || w->used>=w->capacity) {w->valid=false;return;}
    va_list args;va_start(args,format);
    const int n=vsnprintf(w->buffer+w->used,w->capacity-w->used,format,args);va_end(args);
    if(n<0 || (size_t)n>=w->capacity-w->used) {w->valid=false;return;}
    w->used+=(size_t)n;
}
static long fixed(float value,float scale)
{
    const float x=value*scale;
    if(!isfinite(x)) return 0;
    if(x>2147483520.0f) return INT32_MAX;
    if(x<-2147483520.0f) return INT32_MIN;
    return (long)lroundf(x);
}
size_t ChassisForceTelemetryHeader(char *b,size_t cap)
{
    if(!b || cap<sizeof columns) return 0;
    memcpy(b,columns,sizeof columns);return sizeof columns-1;
}
size_t ChassisForceTelemetryRow(const ChassisForceCycleOutput_s *f,const ChassisPowerSnapshot_s *p,
    const SuperCapSnapshot_s *c,const RefereePowerSnapshot_s *r,char *b,size_t cap)
{
    if(!f || !p || !c || !r || !b || !cap) return 0;
    Writer w={.buffer=b,.capacity=cap,.valid=true};
    append(&w,"force-v1,%lu,%lu,%lu,%lu,%lu,%lu,1,%u,%u,%u,%u,%lu,%u,%lu,%lu,%u,%ld",
        (unsigned long)f->tick_ms,(unsigned long)f->command_seq,(unsigned long)f->cycle_seq,
        (unsigned long)f->config_version,(unsigned long)f->safety_epoch,(unsigned long)f->policy_version,
        CHASSIS_FORCE_EXIT_MODE,CHASSIS_FORCE_SPIN_MODE,CHASSIS_POWER_SOURCE_MODE,f->calibrated,
        (unsigned long)f->calibration_checksum,(unsigned)f->state,(unsigned long)f->faults,(unsigned long)f->limits,
        f->can_submitted,fixed(f->dt_s,1000000));
    for(unsigned k=0;k<2;++k) append(&w,",%ld",fixed(f->force_g[k],1000));
    for(unsigned k=0;k<3;++k) append(&w,",%ld",fixed(f->wrench_request[k],1000));
    for(unsigned k=0;k<3;++k) append(&w,",%ld",fixed(f->wrench_sent_est[k],1000));
    for(unsigned k=0;k<3;++k) append(&w,",%ld",fixed(f->velocity[k],1000));
    append(&w,",%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u,%ld,%ld,%ld,%u,%u,%u,%lu,%lu,%lu",
        fixed(f->residual,1000),fixed(f->alpha,1000),fixed(f->beta,1000),fixed(f->predicted_w,100),
        fixed(f->physical_target_w,100),fixed(f->load_power_w,100),fixed(f->torque_nm,1000),f->growth_permitted,
        fixed(f->offered_torque_nm,1000),fixed(f->offered_power_w,100),fixed(f->operating_power_w,100),f->braking,
        f->brake_timed_out,f->zero_over_budget,(unsigned long)f->command_age_ms,(unsigned long)f->source_age_ms,(unsigned long)f->yaw_age_ms);
    for(unsigned k=0;k<4;++k) append(&w,",%ld",fixed(f->raw_current[k],1));
    for(unsigned k=0;k<4;++k) append(&w,",%ld",fixed(f->allocated_current[k],1));
    for(unsigned k=0;k<4;++k) append(&w,",%d",f->current[k]);
    for(unsigned k=0;k<4;++k) append(&w,",%ld",fixed(f->actual_rpm[k],1));
    for(unsigned k=0;k<4;++k) append(&w,",%ld",fixed(f->feedback_current[k],1));
    for(unsigned k=0;k<4;++k) append(&w,",%u",f->temperature[k]);
    for(unsigned k=0;k<4;++k) append(&w,",%lu",(unsigned long)f->feedback_age_ms[k]);
    append(&w,",%ld,%ld,%ld,%u,%ld,%u,%ld,%u,%ld,%u,%ld,%u,%lu,%lu,%lu,%lu,%lu,1",
        fixed(p->policy.control.nominal_budget_w,100),fixed(p->power.effective_limit_w,100),
        fixed(p->policy.control.power_ceiling_w,100),r->chassis_power_limit_w,
        fixed(p->policy.budget.buffer_lower_j,100),p->policy.budget.recovering,fixed(c->status.energy_ratio,1000),
        c->status.available_power_limit_w,fixed(c->input_power.input_power_w,100),p->policy.cap_command.input_power_target_w,
        fixed(p->policy.confirmed_input_target_w,1),p->policy.input_confirmed,(unsigned long)p->policy.confirmed_age_ms,
        (unsigned long)(f->tick_ms-c->status.last_rx_tick_ms),(unsigned long)(f->tick_ms-c->input_power.last_rx_tick_ms),
        (unsigned long)p->policy.cap_send_failures,(unsigned long)p->policy.cap_confirmation_timeouts);
    append(&w,",%u,%lu,%lu,%lu,%lu,%lu,%lu\r\n",f->feedback_valid,(unsigned long)f->input_us,
        (unsigned long)f->estimate_us,(unsigned long)f->command_us,(unsigned long)f->allocation_us,
        (unsigned long)f->guard_us,(unsigned long)f->can_us);
    if(!w.valid) {b[0]=0;return 0;}return w.used;
}
