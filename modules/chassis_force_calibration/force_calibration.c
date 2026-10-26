#include "force_calibration.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
static bool active(const FcEngine *e) {return e->status.state>=FC_PREPARING && e->status.state<=FC_TAIL;}
static FcReason guard(const FcEngine *e,const FcInput *i)
{
    const FcLimits *l=&e->status.limits;
    if(!i->usb_open || i->usb_generation!=e->status.generation) return FC_REASON_USB;
    if(!i->bench_permitted || !i->command_fresh || !i->aux_stopped) return FC_REASON_PERMISSION;
    if((uint32_t)(i->now_us-e->heartbeat_us)>(uint32_t)l->heartbeat_ms*1000) return FC_REASON_HEARTBEAT;
    for(unsigned w=0;w<4;w++) {
        /* A wrapped feedback sequence of zero is valid; age proves freshness. */
        if(i->feedback_age_us[w]>FC_MAX_FEEDBACK_US) return FC_REASON_FEEDBACK;
        if(!isfinite(i->used_rpm[w])) return FC_REASON_FEEDBACK;
        if(abs(i->raw_rpm[w])>=l->max_rpm || fabsf(i->used_rpm[w])>=l->max_rpm) return FC_REASON_OVERSPEED;
        if(i->temperature[w]>=l->stop_temperature_c) return FC_REASON_TEMPERATURE;
    }
    if(!i->meter_fresh || !i->budget_valid || !isfinite(i->load_w) || !isfinite(i->budget_w) ||
       i->budget_w<=0 || i->load_w<0 || i->load_w>i->budget_w ||
       (e->started && i->budget_w+.001f<e->start_budget_w)) return FC_REASON_POWER;
    if(e->clock_started && (uint32_t)(i->now_us-e->last_us)>FC_MAX_CYCLE_US) return FC_REASON_TIMING;
    return FC_REASON_NONE;
}
void FcInit(FcEngine *e,const FcLimits *l)
{
    memset(e,0,sizeof *e);
    if(!l) return;
    e->status.limits=*l;
    e->valid_limits=l->max_current>0 && l->max_current<=1500 && l->max_on_ms>0 && l->max_on_ms<=500 &&
        l->max_rpm>l->stationary_rpm && l->max_rpm<=1000 && l->stationary_ms>0 &&
        l->heartbeat_ms>0 && l->heartbeat_ms<=150 && l->pre_ms>0 && l->pre_ms<=50 &&
        l->tail_ms>=100 && l->tail_ms<=100 && l->stop_temperature_c>0 && l->stop_temperature_c<=50;
}
bool FcArm(FcEngine *e,const FcInput *i,uint32_t *session)
{
    if(!e || !i || !session || !e->valid_limits || active(e)) return false;
    /* Test guards without renewing an old clock or inheriting an old generation. */
    FcEngine trial=*e;trial.status.generation=i->usb_generation;trial.heartbeat_us=i->now_us;
    trial.clock_started=false;trial.started=false;
    if(guard(&trial,i)!=FC_REASON_NONE) return false;
    do {++e->next_session;} while(!e->next_session);
    e->status.session=e->next_session;e->status.generation=i->usb_generation;
    e->status.state=FC_PREPARING;e->status.reason=FC_REASON_NONE;
    e->heartbeat_us=i->now_us;e->last_us=i->now_us;e->clock_started=false;
    e->stationary=false;e->started=false;*session=e->status.session;
    return true;
}
bool FcStart(FcEngine *e,uint32_t session,const FcPulse *p,const FcInput *i)
{
    if(!e || !i || !p || session!=e->status.session || e->status.state!=FC_READY || guard(e,i)!=FC_REASON_NONE ||
       p->wheel>=4 || !p->current || abs(p->current)>e->status.limits.max_current ||
       !p->rise_ms || !p->hold_ms || !p->fall_ms ||
       (uint32_t)p->rise_ms+p->hold_ms+p->fall_ms>e->status.limits.max_on_ms) return false;
    do {++e->next_capture;} while(!e->next_capture);
    e->status.capture_id=e->next_capture;e->status.capture_session=session;
    e->status.capture_generation=i->usb_generation;e->status.pulse=*p;
    e->status.state=FC_PRE;e->status.complete=false;e->status.zero_submitted=false;
    memset(e->status.final,0,sizeof e->status.final);
    e->start_us=i->now_us;e->start_budget_w=i->budget_w;e->started=true;
    return true;
}
bool FcHeartbeat(FcEngine *e,uint32_t session,uint32_t generation,uint32_t now)
{
    if(!e || !session || session!=e->status.session || generation!=e->status.generation) return false;
    e->heartbeat_us=now;return true;
}
void FcCancel(FcEngine *e,FcReason reason,uint32_t now)
{
    if(!e || !active(e)) return;
    if(e->status.reason==FC_REASON_NONE) e->status.reason=reason;
    if(e->status.state!=FC_TAIL) {e->tail_us=now;e->status.zero_submitted=false;}
    e->status.state=e->started ? FC_TAIL : FC_ABORTED;
    e->status.requested=0;memset(e->status.final,0,sizeof e->status.final);
}
FcDrive FcStep(FcEngine *e,const FcInput *i)
{
    FcDrive d={0};
    if(!e || !i) return d;
    memcpy(e->status.raw_rpm,i->raw_rpm,sizeof i->raw_rpm);
    memcpy(e->status.used_rpm,i->used_rpm,sizeof i->used_rpm);
    memcpy(e->status.temperature,i->temperature,sizeof i->temperature);
    memcpy(e->status.feedback_age_us,i->feedback_age_us,sizeof i->feedback_age_us);
    e->status.load_w=i->load_w;e->status.budget_w=i->budget_w;
    if(active(e) && e->status.state!=FC_TAIL) {
        FcReason reason=guard(e,i);if(reason) FcCancel(e,reason,i->now_us);
    }
    const bool baseline=!e->clock_started;
    e->last_us=i->now_us;e->clock_started=true;
    if(e->status.state==FC_PREPARING || e->status.state==FC_READY) {
        bool stopped=true;
        for(unsigned w=0;w<4;w++) stopped &= abs(i->raw_rpm[w])<=e->status.limits.stationary_rpm &&
            fabsf(i->used_rpm[w])<=e->status.limits.stationary_rpm;
        if(!stopped) {e->stationary=false;e->status.state=FC_PREPARING;}
        else if(!e->stationary) {e->stationary=true;e->stationary_us=i->now_us;}
        else if((uint32_t)(i->now_us-e->stationary_us)>=(uint32_t)e->status.limits.stationary_ms*1000)
            e->status.state=FC_READY;
    }
    if(e->status.state>=FC_PRE && e->status.state<=FC_FALL) {
        const FcPulse *p=&e->status.pulse;
        const uint32_t elapsed=i->now_us-e->start_us,pre=(uint32_t)e->status.limits.pre_ms*1000;
        int32_t value=0;
        if(elapsed<pre) e->status.state=FC_PRE;
        else {
            const uint32_t t=elapsed-pre,rise=(uint32_t)p->rise_ms*1000,hold=(uint32_t)p->hold_ms*1000,
                fall=(uint32_t)p->fall_ms*1000;
            if(t<rise) {e->status.state=FC_RISE;value=(int32_t)p->current*(int32_t)t/(int32_t)rise;}
            else if(t<rise+hold) {e->status.state=FC_HOLD;value=p->current;}
            else if(t<rise+hold+fall) {e->status.state=FC_FALL;value=(int32_t)p->current*(int32_t)(rise+hold+fall-t)/(int32_t)fall;}
            else {e->status.state=FC_TAIL;e->tail_us=i->now_us;e->status.zero_submitted=false;}
        }
        if(!baseline) d.current[p->wheel]=(int16_t)value;
    }
    if(e->status.state==FC_TAIL && e->status.zero_submitted &&
       (uint32_t)(i->now_us-e->tail_us)>=(uint32_t)e->status.limits.tail_ms*1000) {
        e->status.state=e->status.reason ? FC_ABORTED : FC_FROZEN;
        e->status.complete=e->status.reason==FC_REASON_NONE;
    }
    e->status.requested=d.current[e->status.pulse.wheel];
    d.state=e->status.state;d.reason=e->status.reason;
    d.recording=e->started && e->status.state>=FC_PRE && e->status.state<=FC_TAIL;
    return d;
}
bool FcValidateFinal(FcEngine *e,const FcInput *i,const int16_t current[4])
{
    bool zero=true;for(unsigned w=0;w<4;w++) zero &= current[w]==0;
    if(zero) return true;
    FcReason reason=guard(e,i);
    const FcPulse *p=&e->status.pulse;
    if(reason) {FcCancel(e,reason,i->now_us);return false;}
    if(e->status.state<FC_RISE || e->status.state>FC_FALL ||
       (uint32_t)(i->now_us-e->start_us)>=(uint32_t)(e->status.limits.pre_ms+p->rise_ms+p->hold_ms+p->fall_ms)*1000) {
        FcCancel(e,FC_REASON_TIMING,i->now_us);return false;
    }
    for(unsigned w=0;w<4;w++) if((w!=p->wheel && current[w]) || abs(current[w])>abs(p->current) ||
       abs(current[w])>e->status.limits.max_current || (int32_t)current[w]*p->current<0) {
        FcCancel(e,FC_REASON_PROTOCOL,i->now_us);return false;
    }
    return true;
}
void FcRecordSubmission(FcEngine *e,uint32_t now,const int16_t current[4],bool submitted)
{
    memcpy(e->status.final,current,sizeof e->status.final);
    bool zero=true;for(unsigned w=0;w<4;w++) zero &= current[w]==0;
    if(!submitted) {FcCancel(e,FC_REASON_CAN,now);return;}
    if(zero) {e->status.zero_submitted=true;e->status.zero_submitted_us=now;}
    else e->status.zero_submitted=false;
}
void FcGetStatus(const FcEngine *e,FcStatus *s) {*s=e->status;}
