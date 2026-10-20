#include "pid_autotune.h"
#include <math.h>

static bool Stationary(const PIDAutotuneFeedback *f)
{
    for (unsigned i=0;i<4;++i) if (!isfinite(f->rpm[i]) || fabsf(f->rpm[i])>50) return false;
    return true;
}
bool PIDAutotuneBegin(PIDAutotune *a,uint32_t session,const PIDAutotuneFeedback *f)
{
    if (a->active || !session || !f->permitted || !f->feedback_ok || !f->can_ok || !Stationary(f)) return false;
    *a=(PIDAutotune){.active=true,.session=session,.state=PID_AUTO_READY,
        .lease_ms=f->now_ms,.begin_ms=f->now_ms};
    return true;
}
bool PIDAutotuneBeginV2(PIDAutotune *a,uint32_t session,const PIDAutotuneFeedback *f)
{
    if (!PIDAutotuneBegin(a,session,f)) return false;
    a->v2=true;return true;
}
bool PIDAutotuneStart(PIDAutotune *a,const PIDAutotuneTrial *t,const PIDAutotuneFeedback *f)
{
    if (!a->active || a->state==PID_AUTO_RUNNING || (a->v2 && a->trial>=50) || (!a->v2 && !Stationary(f)) ||
        t->wheel>=4 || !isfinite(t->rpm) || t->rpm<100 || t->rpm>1500 ||
        t->hold_ms<500 || t->hold_ms>2000 || !isfinite(t->current_limit) ||
        t->current_limit<500 || t->current_limit>5000 ||
        !isfinite(t->kp) || t->kp<0 || t->kp>1000 ||
        !isfinite(t->ki) || t->ki<0 || t->ki>1000 ||
        !isfinite(t->kd) || t->kd<0 || t->kd>1000) return false;
    uint32_t session=a->session,trial=a->trial+1,begin=a->begin_ms;bool v2=a->v2;
    *a=(PIDAutotune){.active=true,.session=session,.trial=trial,.begin_ms=begin,
        .lease_ms=f->now_ms,.start_ms=f->now_ms,.test=*t,.state=PID_AUTO_RUNNING,
        .settle_ms=UINT32_MAX,.v2=v2,.last_tick=f->now_ms,.min_stat_ms=UINT32_MAX,
        .stat_limit_ms=20};
    return true;
}
void PIDAutotuneKeepAlive(PIDAutotune *a,uint32_t now) {a->lease_ms=now;}
void PIDAutotuneStop(PIDAutotune *a,PIDAutotuneReason reason)
{
    a->active=false; a->state=PID_AUTO_ABORTED;a->reason=reason;
}
float PIDAutotuneTarget(const PIDAutotune *a,uint32_t now)
{
    if(a->v2) return a->active && a->state==PID_AUTO_RUNNING && a->phase==1 &&
        (uint32_t)(now-a->step_ms)<a->test.hold_ms ? a->test.rpm : 0;
    uint32_t elapsed=now-a->start_ms;
    return a->active && a->state==PID_AUTO_RUNNING && elapsed>=300 &&
        elapsed<300+a->test.hold_ms ? a->test.rpm : 0;
}
static void FinishV2(PIDAutotune *a)
{
    a->state=PID_AUTO_DONE;
    a->stat_limit_ms=a->min_stat_ms==UINT32_MAX ? 20 :
        (uint32_t)fminf(20,fmaxf(2,a->min_stat_ms*4));
    if(a->coverage_ms) {
        a->rmse=(float)sqrt(a->error2/a->coverage_ms);
        a->jitter=(float)(a->current_variation*1000/a->coverage_ms);
        a->limited_pm=(uint32_t)(1000ULL*a->limited_ms/a->coverage_ms);
    }
    a->overshoot=fmaxf(0,a->peak-a->test.rpm);
    if(a->steady_ms) a->steady=(float)(a->steady_sum/a->steady_ms);
    a->settle_ms=a->last_outside+100<=a->test.hold_ms ? a->last_outside : UINT32_MAX;
    if(a->samples<20 || a->feedback_count<20 || !a->steady_ms ||
        a->coverage_ms*100ULL<a->test.hold_ms*95ULL || a->max_feedback_ms>20 ||
        a->max_stat_ms>a->stat_limit_ms || !isfinite(a->jitter) || a->jitter>4294967.0f)
        a->reason=PID_AUTO_QUALITY;
    else if(a->peak<a->test.rpm*.1f) a->reason=PID_AUTO_NO_RESPONSE;
    else if(a->limited_ms*100ULL>a->coverage_ms*5ULL) a->reason=PID_AUTO_LIMITED;
}
static void TickV2(PIDAutotune *a,const PIDAutotuneFeedback *f)
{
    if(a->state!=PID_AUTO_RUNNING) return;
    uint32_t now=f->now_ms;
    if(a->phase==1 && (uint32_t)(now-a->step_ms)>=a->test.hold_ms) {
        a->phase=2;a->stationary_seen=false;a->window_ms=a->test.hold_ms;
    }
    if(a->phase!=1) {
        if(!Stationary(f) || (uint32_t)(now-a->last_tick)>20) a->stationary_seen=false;
        else if(!a->stationary_seen) {a->stationary_seen=true;a->stationary_since=now;}
        if(a->stationary_seen && (uint32_t)(now-a->stationary_since)>=300) {
            if(a->phase==0) {a->phase=1;a->step_ms=now;a->stationary_seen=false;}
            else if((uint32_t)(now-a->step_ms)>=a->test.hold_ms+300) FinishV2(a);
        } else if((uint32_t)(now-(a->phase==0 ? a->start_ms : a->step_ms+a->test.hold_ms))>=3000)
            PIDAutotuneStop(a,PID_AUTO_STATIONARY);
    }
    a->last_tick=now;
}
void PIDAutotuneTick(PIDAutotune *a,const PIDAutotuneFeedback *f)
{
    if (!a->active) return;
    if ((uint32_t)(f->now_ms-a->lease_ms)>1000 || (uint32_t)(f->now_ms-a->begin_ms)>180000)
        {PIDAutotuneStop(a,PID_AUTO_TIMEOUT);return;}
    if (!f->permitted) {PIDAutotuneStop(a,PID_AUTO_GUARD);return;}
    if (!f->feedback_ok) {PIDAutotuneStop(a,PID_AUTO_FEEDBACK);return;}
    if (!f->can_ok) {PIDAutotuneStop(a,PID_AUTO_CAN);return;}
    for (unsigned i=0;i<4;++i) {
        float maximum=a->state==PID_AUTO_RUNNING && i==a->test.wheel ? fmaxf(a->test.rpm*1.75f,a->test.rpm+300) : 100;
        if (!isfinite(f->rpm[i]) || fabsf(f->rpm[i])>maximum)
            {PIDAutotuneStop(a,PID_AUTO_OVERSPEED);return;}
    }
    if(a->v2) {TickV2(a,f);return;}
    if (a->state!=PID_AUTO_RUNNING || (uint32_t)(f->now_ms-a->start_ms)<a->test.hold_ms+600) return;
    a->state=PID_AUTO_DONE;
    if (a->samples<20 || !a->steady_samples) {a->reason=PID_AUTO_NO_RESPONSE;return;}
    a->rmse=(float)sqrt(a->error2/a->samples);
    a->overshoot=fmaxf(0,a->peak-a->test.rpm);
    a->steady=(float)(a->steady_sum/a->steady_samples);
    a->jitter=(float)(a->current_variation/a->samples);
    a->limited_pm=(uint32_t)(1000ULL*a->limited_samples/a->samples);
    a->settle_ms=a->last_outside+100<a->test.hold_ms ? a->last_outside : UINT32_MAX;
    if (a->peak<a->test.rpm*.1f) a->reason=PID_AUTO_NO_RESPONSE;
    else if (a->limited_pm>50) a->reason=PID_AUTO_LIMITED;
}
void PIDAutotuneObserveV2(PIDAutotune *a,const PIDAutotuneFeedback *f,float rpm,float raw,float final,bool limited)
{
    if(!a->v2) {PIDAutotuneObserve(a,f->now_ms,rpm,raw,final,limited);return;}
    if(!a->active || a->state!=PID_AUTO_RUNNING || !a->phase || a->metrics_closed) return;
    if(!isfinite(rpm) || !isfinite(raw) || !isfinite(final)) {PIDAutotuneStop(a,PID_AUTO_FEEDBACK);return;}
    uint32_t elapsed=f->now_ms-a->step_ms,end=a->test.hold_ms;
    uint32_t offset=elapsed<end ? elapsed : end;
    uint32_t rx=f->rx_ms[a->test.wheel],sequence=f->rx_sequence[a->test.wheel];
    uint32_t age=f->now_ms-rx;
    if(age>a->max_feedback_ms) a->max_feedback_ms=age;
    if(a->observed) {
        uint32_t delta=f->now_ms-a->previous_ms;
        if(delta>a->max_stat_ms) a->max_stat_ms=delta;
        if(delta && delta<a->min_stat_ms) a->min_stat_ms=delta;
        uint32_t previous=a->previous_ms-a->step_ms;
        uint32_t dt=offset>previous ? offset-previous : 0;
        if(a->previous_error>fmaxf(a->test.rpm*.05f,10)) a->last_outside=offset;
        if(sequence!=a->previous_sequence) {
            uint32_t gap=rx-a->previous_rx_ms;
            if(gap>a->max_feedback_ms) a->max_feedback_ms=gap;
            ++a->feedback_count;a->previous_rx_ms=rx;
        }
        /* Never turn a missing interval into successful time coverage. */
        if(delta<=20 && age<=20) {
            a->coverage_ms+=dt;
            a->error2+=(double)a->previous_error*a->previous_error*dt;
            if(a->previous_limited) a->limited_ms+=dt;
            uint32_t steady_start=end*4/5,from=previous>steady_start ? previous : steady_start;
            if(offset>from) {a->steady_sum+=(double)a->previous_error*(offset-from);a->steady_ms+=offset-from;}
            if(delta && elapsed<end) a->current_variation+=fabsf(final-a->previous_current);
        }
    } else {a->feedback_count=1;a->previous_rx_ms=rx;}
    if(elapsed>=end) {a->metrics_closed=true;return;}
    ++a->samples;
    a->observed=true;a->previous_ms=f->now_ms;a->previous_sequence=sequence;
    a->previous_error=fabsf(a->test.rpm-rpm);a->previous_current=final;
    a->previous_limited=limited || fabsf(raw-final)>fmaxf(5,fabsf(raw)*.05f);
    a->peak=fmaxf(a->peak,rpm);
    if(a->previous_error>fmaxf(a->test.rpm*.05f,10)) a->last_outside=elapsed+1;
}
void PIDAutotuneObserve(PIDAutotune *a,uint32_t now,float rpm,float raw,float final,bool limited)
{
    uint32_t elapsed=now-a->start_ms;
    if (!a->active || a->state!=PID_AUTO_RUNNING || elapsed<300 || elapsed>=300+a->test.hold_ms) return;
    if (!isfinite(rpm) || !isfinite(raw) || !isfinite(final)) {PIDAutotuneStop(a,PID_AUTO_FEEDBACK);return;}
    float error=fabsf(a->test.rpm-rpm);
    a->error2+=(double)error*error;
    if (a->samples) a->current_variation+=fabsf(final-a->previous_current);
    a->previous_current=final;++a->samples;
    a->limited_samples+=limited || fabsf(raw-final)>fmaxf(5,fabsf(raw)*.05f);
    a->peak=fmaxf(a->peak,rpm);
    if (error>a->test.rpm*.05f) a->last_outside=elapsed-300+1;
    if (elapsed>=300+a->test.hold_ms*4/5) {a->steady_sum+=error;++a->steady_samples;}
}
