#include "energy_scheduler.h"
#include <math.h>
#include <stddef.h>
static const float steps[PP_ENERGY_NODES]={.01f,.01f,.02f,.02f,.04f,.04f,.08f,.08f,.1f,.1f};
static bool valid(const PPEnergyConfig *c,const PPEnergyInput *i)
{
    if(!c || !i || !i->permitted || !i->buffer_valid || !i->strategy_version ||
        !i->model_version || !i->capability_version) return false;
    const float nonnegative[]={c->buffer_max_j,c->buffer_floor_j,c->buffer_terminal_j,
        c->input_max_w,c->load_max_w,c->input_slew_w_s,c->path_loss_w,
        i->buffer_low_j,i->buffer_high_j,i->referee_w,i->demand_w,i->current_request_w,i->load_floor_w};
    for(unsigned k=0;k<sizeof nonnegative/sizeof nonnegative[0];++k)
        if(!isfinite(nonnegative[k]) || nonnegative[k]<0) return false;
    if(c->buffer_floor_j>c->buffer_terminal_j || c->buffer_terminal_j>c->buffer_max_j ||
        i->buffer_low_j>i->buffer_high_j || i->buffer_low_j<c->buffer_floor_j ||
        i->buffer_high_j>c->buffer_max_j || i->load_floor_w>i->demand_w) return false;
    if(i->capacitor) {
        const float v[]={c->cap_max_j,c->cap_floor_j,c->cap_terminal_j,c->eta_charge,c->eta_discharge,
            c->charge_max_w,c->discharge_max_w,c->leak_w,i->cap_low_j,i->cap_high_j,i->input_low_w,i->input_high_w};
        for(unsigned k=0;k<sizeof v/sizeof v[0];++k) if(!isfinite(v[k]) || v[k]<0) return false;
        if(!i->cap_valid || c->eta_charge<=0 || c->eta_charge>1 || c->eta_discharge<=0 || c->eta_discharge>1 ||
            c->cap_floor_j>c->cap_terminal_j || c->cap_terminal_j>c->cap_max_j ||
            i->cap_low_j>i->cap_high_j || i->cap_low_j<c->cap_floor_j || i->cap_high_j>c->cap_max_j ||
            i->input_low_w>i->input_high_w || i->input_high_w>c->input_max_w) return false;
    }
    return true;
}
static float cap_rate(const PPEnergyConfig *c,float q)
{
    return (q>=0 ? -q/c->eta_discharge : -c->eta_charge*q)-c->leak_w;
}
bool PPEnergyCheck(const PPEnergyConfig *c,const PPEnergyInput *i,const PPEnergyPlan *p,PPEnergyTrace *out)
{
    if(!out) return false;
    *out=(PPEnergyTrace){0};if(!p || !valid(c,i)) return false;
    if(p->strategy_version!=i->strategy_version || p->model_version!=i->model_version ||
        p->capability_version!=i->capability_version) return false;
    PPEnergyTrace t={.buffer_low_j=i->buffer_low_j,.buffer_high_j=i->buffer_high_j,
        .cap_low_j=i->cap_low_j,.cap_high_j=i->cap_high_j,
        .minimum_buffer_j=i->buffer_low_j,.minimum_cap_j=i->cap_low_j};
    float previous=i->current_request_w;
    for(unsigned k=0;k<PP_ENERGY_NODES;++k) {
        const unsigned b=k/2;const float dt=steps[k],load=p->load_w[b],request=p->request_w[b];
        if(!isfinite(load) || load<i->load_floor_w || load>c->load_max_w || load>i->demand_w ||
            !isfinite(request) || request<0 || request>c->input_max_w) return false;
        if(i->capacitor && (fabsf(request-previous)>c->input_slew_w_s*dt+.0001f ||
            (!i->strong_confirmation && load+c->path_loss_w>i->input_low_w))) return false;
        previous=request;
        const float high_in=i->capacitor ? i->input_high_w : load+c->path_loss_w;
        const float low_in=i->capacitor ? i->input_low_w : i->load_floor_w+c->path_loss_w;
        if(high_in>c->input_max_w) return false;
        t.buffer_low_j=fminf(c->buffer_max_j,t.buffer_low_j+(i->referee_w-high_in)*dt);
        t.buffer_high_j=fminf(c->buffer_max_j,t.buffer_high_j+(i->referee_w-low_in)*dt);
        t.minimum_buffer_j=fminf(t.minimum_buffer_j,t.buffer_low_j);
        if(t.buffer_low_j<c->buffer_floor_j) return false;
        if(i->capacitor) {
            const float high_q=load+c->path_loss_w-low_in;
            const float low_q=i->load_floor_w+c->path_loss_w-high_in;
            if(high_q>c->discharge_max_w || low_q< -c->charge_max_w) return false;
            t.cap_low_j+=cap_rate(c,high_q)*dt;t.cap_high_j+=cap_rate(c,low_q)*dt;
            t.minimum_cap_j=fminf(t.minimum_cap_j,t.cap_low_j);
            if(t.cap_low_j<c->cap_floor_j || t.cap_high_j>c->cap_max_j) return false;
        }
    }
    if(!isfinite(t.buffer_low_j) || !isfinite(t.cap_low_j) || !isfinite(t.cap_high_j) ||
        t.buffer_low_j<c->buffer_terminal_j || (i->capacitor && t.cap_low_j<c->cap_terminal_j)) return false;
    t.feasible=true;*out=t;return true;
}
bool PPEnergySchedule(const PPEnergyConfig *c,const PPEnergyInput *i,PPEnergyPlan *out)
{
    if(!out) return false;
    *out=(PPEnergyPlan){0};if(!valid(c,i)) return false;
    PPEnergyPlan p={.created_ms=i->now_ms,.strategy_version=i->strategy_version,
        .model_version=i->model_version,.capability_version=i->capability_version};
    for(unsigned b=0;b<PP_ENERGY_BLOCKS;++b) {
        /* Keep the already accepted request. Unidentified input dynamics cannot
         * justify planned charging credit, even for later prediction nodes. */
        p.request_w[b]=i->capacitor ? i->current_request_w : fminf(i->referee_w,c->input_max_w);
        p.load_w[b]=i->load_floor_w;
    }
    PPEnergyTrace t;if(!PPEnergyCheck(c,i,&p,&t)) return false;
    /* Constant held demand is bounded for the entire 0.5 s horizon. */
    float lo=i->load_floor_w,hi=fminf(c->load_max_w,i->demand_w);
    for(unsigned n=0;n<24;++n) {
        const float mid=(lo+hi)*.5f;
        for(unsigned b=0;b<PP_ENERGY_BLOCKS;++b) p.load_w[b]=mid;
        if(PPEnergyCheck(c,i,&p,&t)) lo=mid;else hi=mid;
    }
    for(unsigned b=0;b<PP_ENERGY_BLOCKS;++b) p.load_w[b]=lo;
    p.valid=PPEnergyCheck(c,i,&p,&t);p.recovering=lo<i->referee_w;
    *out=p;return p.valid;
}
