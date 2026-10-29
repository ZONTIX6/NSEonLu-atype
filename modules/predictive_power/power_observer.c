#include "power_observer.h"
#include <math.h>
#include <string.h>
static bool forward(uint32_t a,uint32_t b) {return (uint32_t)(a-b)<UINT32_C(0x80000000);}
bool PPObserverRecord(PPObserver *o,const PPFinalActuation *f)
{
    if(!o || !f || (o->recorded && !forward(f->tick_ms,o->last_record_ms))) return false;
    o->history[o->head]=*f;o->head=(uint8_t)((o->head+1)%PP_HISTORY_LENGTH);
    if(o->count<PP_HISTORY_LENGTH) ++o->count;
    o->last_record_ms=f->tick_ms;o->recorded=true;return true;
}
bool PPObserverCommandAt(const PPObserver *o,uint32_t tick,uint32_t age,PPFinalActuation *f)
{
    if(!f) return false;
    *f=(PPFinalActuation){0};
    if(!o || age>=UINT32_C(0x80000000)) return false;
    for(unsigned k=0;k<o->count;++k) {
        const PPFinalActuation *p=&o->history[(o->head+PP_HISTORY_LENGTH-1-k)%PP_HISTORY_LENGTH];
        if(p->submitted && (uint32_t)(tick-p->tick_ms)<=age) {*f=*p;return true;}
    }
    return false;
}
bool PPObserverStableCommand(const PPObserver *o,uint32_t now,uint32_t delay,
                             uint32_t model_version,uint32_t config_version,PPFinalActuation *out)
{
    PPFinalActuation first,last;
    if(!out) return false;
    *out=(PPFinalActuation){0};
    if(delay>30 || !PPObserverCommandAt(o,now-delay,5,&first) ||
        !PPObserverCommandAt(o,now,5,&last) || first.model_version!=model_version ||
        first.config_version!=config_version) return false;
    for(unsigned k=0;k<o->count;++k) {
        const PPFinalActuation *f=&o->history[(o->head+PP_HISTORY_LENGTH-1-k)%PP_HISTORY_LENGTH];
        if((uint32_t)(now-f->tick_ms)>(uint32_t)(now-first.tick_ms)) continue;
        if(!f->submitted || f->model_version!=model_version || f->config_version!=config_version ||
            memcmp(f->command,first.command,sizeof first.command)!=0) return false;
    }
    *out=last;return true;
}
bool PPObserverUpdate(PPObserver *o,const PPMeasurement *m)
{
    if(!o || !m) return false;
    o->state.valid=false;
    if(m->model_version==0 || m->online_mask!=15) return false;
    if(o->state.model_version!=m->model_version) {
        memset(o->sampled,0,sizeof o->sampled);o->state.model_version=m->model_version;
    }
    /* Validate all four before accepting any component. */
    for(unsigned w=0;w<4;++w) {
        if(!isfinite(m->omega[w]) || !isfinite(m->current[w]) ||
            (uint32_t)(m->now_ms-m->rx_ms[w])>5 ||
            (o->sampled[w] && (!forward(m->sequence[w],o->sequence[w]) ||
                             !forward(m->rx_ms[w],o->rx_ms[w])))) return false;
        if(o->sampled[w] && m->sequence[w]==o->sequence[w] &&
            ((uint32_t)(m->now_ms-o->rx_ms[w])>5 || m->rx_ms[w]!=o->rx_ms[w])) return false;
    }
    for(unsigned w=0;w<4;++w) {
        if(!o->sampled[w] || m->sequence[w]!=o->sequence[w]) {
            o->state.current[w]=m->current[w];o->state.omega[w]=m->omega[w];
            o->rx_ms[w]=m->rx_ms[w];o->sequence[w]=m->sequence[w];o->sampled[w]=true;
        }
    }
    o->state.tick_ms=m->now_ms;o->state.valid=true;return true;
}
