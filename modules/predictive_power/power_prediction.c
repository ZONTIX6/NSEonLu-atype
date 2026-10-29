#include "power_prediction.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static uint32_t hash(uint32_t h,uint32_t v) { return (h^v)*UINT32_C(16777619); }
uint32_t PPModelChecksum(const PPModel *m)
{
    if(!m) return 0;
    uint32_t h=hash(hash(UINT32_C(2166136261),m->schema),m->version);
    h=hash(hash(h,m->command_delay_ms),m->braking_valid);
    const float values[]={m->command_scale,m->horizon_s,m->max_dt_s,m->max_speed,
        m->max_current,m->max_command,m->base_w,m->resistance,m->mechanical,
        m->speed_loss,m->speed_squared_loss,m->power_margin_w,m->speed_error,
        m->current_error,m->acceleration_bound};
    for(unsigned k=0;k<sizeof values/sizeof values[0];++k) {
        uint32_t bits;memcpy(&bits,&values[k],sizeof bits);h=hash(h,bits);
    }
    for(unsigned w=0;w<4;++w) {
        const float v[]={m->current_tau[w],m->current_gain[w],m->speed_gain[w],m->speed_drag[w]};
        for(unsigned k=0;k<4;++k) {uint32_t bits;memcpy(&bits,&v[k],sizeof bits);h=hash(h,bits);}
    }
    return h;
}
bool PPModelValid(const PPModel *m)
{
    if(!m || m->schema!=1 || m->version==0 || m->checksum!=PPModelChecksum(m) ||
        m->command_delay_ms>30) return false;
    const float positive[]={m->command_scale,m->horizon_s,m->max_dt_s,m->max_speed,m->max_current,m->max_command};
    for(unsigned k=0;k<sizeof positive/sizeof positive[0];++k)
        if(!isfinite(positive[k]) || positive[k]<=0) return false;
    if(m->max_command>32767 || m->max_dt_s>.1f || m->horizon_s>m->max_dt_s) return false;
    const float nonnegative[]={m->base_w,m->resistance,m->mechanical,m->speed_loss,
        m->speed_squared_loss,m->power_margin_w,m->speed_error,m->current_error,m->acceleration_bound};
    for(unsigned k=0;k<sizeof nonnegative/sizeof nonnegative[0];++k)
        if(!isfinite(nonnegative[k]) || nonnegative[k]<0) return false;
    for(unsigned w=0;w<4;++w)
        if(!isfinite(m->current_tau[w]) || m->current_tau[w]<=0 ||
           !isfinite(m->current_gain[w]) || m->current_gain[w]<=0 ||
           !isfinite(m->speed_gain[w]) || m->speed_gain[w]<=0 ||
           !isfinite(m->speed_drag[w]) || m->speed_drag[w]<0) return false;
    return true;
}
bool PPPredict(const PPModel *m,const PPState *s,const float u[4],float dt,PPPrediction *out)
{
    if(!out) return false;
    *out=(PPPrediction){0};
    if(!PPModelValid(m) || !s || !u || !s->valid || s->model_version!=m->version ||
        !isfinite(dt) || dt<=0 || dt>m->horizon_s) return false;
    PPPrediction p={.power_mean_w=m->base_w,.c=m->base_w+m->power_margin_w};
    for(unsigned w=0;w<4;++w) {
        if(!isfinite(u[w]) || !isfinite(s->omega[w]) || !isfinite(s->current[w]) ||
            fabsf(u[w])>m->max_command || fabsf(s->omega[w])>m->max_speed ||
            fabsf(s->current[w])>m->max_current ||
            (!m->braking_valid && (u[w]*s->omega[w]<0 || s->current[w]*s->omega[w]<0))) return false;
        const float delay=fminf(dt,m->command_delay_ms*.001f);
        float initial=s->current[w];
        if(delay>0) {
            if(!s->applied_valid || !isfinite(s->applied_command[w]) ||
                fabsf(s->applied_command[w])>m->max_command ||
                (!m->braking_valid && s->applied_command[w]*s->omega[w]<0)) return false;
            const float hold=expf(-delay/m->current_tau[w]);
            initial=hold*initial+(1-hold)*m->current_gain[w]*s->applied_command[w]/m->command_scale;
            if(fabsf(initial)>m->max_current || (!m->braking_valid && initial*s->omega[w]<0)) return false;
        }
        const float a=expf(-(dt-delay)/m->current_tau[w]),b=(1-a)*m->current_gain[w];
        const float x=u[w]/m->command_scale;
        p.current[w]=a*initial+b*x;
        p.omega[w]=s->omega[w]+dt*(m->speed_gain[w]*p.current[w]-m->speed_drag[w]*s->omega[w]);
        const float speed_bound=fabsf(s->omega[w])+m->speed_error+m->acceleration_bound*dt;
        if(fabsf(p.current[w])>m->max_current || fabsf(p.omega[w])>m->max_speed ||
            fabsf(p.omega[w])>speed_bound) return false;
        /* Triangle envelope; no candidate-dependent velocity enters this convex bound. */
        /* Covers the start, delay segment and endpoint, not only endpoint loss. */
        const float offset=fmaxf(fabsf(initial),fabsf(s->current[w]))+m->current_error;
        p.h[w]=m->resistance*b*b;
        p.g[w]=copysignf(1,u[w])*b*(2*m->resistance*offset+m->mechanical*speed_bound);
        p.c+=m->resistance*offset*offset+m->mechanical*offset*speed_bound+
            m->speed_loss*speed_bound+m->speed_squared_loss*speed_bound*speed_bound;
        const float omega=p.omega[w],current=p.current[w];
        p.power_mean_w+=m->resistance*current*current+m->mechanical*fmaxf(0,current*omega)+
            m->speed_loss*fabsf(omega)+m->speed_squared_loss*omega*omega;
    }
    p.power_upper_w=p.c;
    for(unsigned w=0;w<4;++w) {const float x=u[w]/m->command_scale;p.power_upper_w+=p.h[w]*x*x+p.g[w]*x;}
    if(!isfinite(p.power_upper_w) || !isfinite(p.power_mean_w) || p.power_upper_w<p.power_mean_w) return false;
    p.valid=true;*out=p;return true;
}
