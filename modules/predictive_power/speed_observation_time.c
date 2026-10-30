#include "speed_observation_time.h"

static uint32_t next_epoch(uint32_t epoch) {++epoch;return epoch ? epoch : 1U;}
static void rebase(PPObservationClock *c,uint32_t cycles,uint32_t tick,uint32_t hz)
{
    c->now_us=(uint64_t)tick*1000U;c->cycles=cycles;c->tick_ms=tick;
    c->hz=hz;c->fraction=0;c->source_epoch=next_epoch(c->source_epoch);
    c->initialized=hz!=0;
}
bool PPObservationClockUpdate(PPObservationClock *c,uint32_t cycles,uint32_t tick,uint32_t hz)
{
    if(!c) return false;
    if(!hz) {rebase(c,cycles,tick,hz);return false;}
    if(!c->initialized) {rebase(c,cycles,tick,hz);return true;}
    const uint32_t coarse=tick-c->tick_ms,delta=cycles-c->cycles;
    const uint64_t coarse_us=(uint64_t)coarse*1000U;
    const uint64_t wrap_us=(UINT64_C(1)<<32)*1000000U/hz;
    const uint64_t numerator=(uint64_t)delta*1000000U+c->fraction;
    const uint64_t elapsed=numerator/hz;
    const uint64_t difference=elapsed>coarse_us ? elapsed-coarse_us : coarse_us-elapsed;
    if(c->hz!=hz || coarse>=UINT32_C(0x80000000) || coarse_us+2000>=wrap_us ||
       difference>2000 || c->now_us>UINT64_MAX-elapsed) {
        rebase(c,cycles,tick,hz);return false;
    }
    c->now_us+=elapsed;c->fraction=(uint32_t)(numerator%hz);
    c->cycles=cycles;c->tick_ms=tick;return true;
}
bool PPObservationReceiveTime(const PPObservationClock *c,uint32_t rx_cycles,
    uint32_t rx_tick,uint32_t max_age_us,uint64_t *rx_us)
{
    if(!rx_us) return false;
    *rx_us=0;
    if(!c || !c->initialized || !c->hz || !max_age_us) return false;
    const uint32_t coarse=c->tick_ms-rx_tick;
    const uint64_t coarse_us=(uint64_t)coarse*1000U;
    const uint64_t age=(uint64_t)(uint32_t)(c->cycles-rx_cycles)*1000000U/c->hz;
    const uint64_t wrap_us=(UINT64_C(1)<<32)*1000000U/c->hz;
    const uint64_t difference=age>coarse_us ? age-coarse_us : coarse_us-age;
    if(coarse>=UINT32_C(0x80000000) || coarse_us+2000>=wrap_us ||
       coarse_us>(uint64_t)max_age_us+1000 || age>max_age_us ||
       difference>2000 || age>c->now_us) return false;
    *rx_us=c->now_us-age;return true;
}
