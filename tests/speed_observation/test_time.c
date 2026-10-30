#include "speed_observation_time.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    PPObservationClock c={0}; uint64_t rx=0;
    assert(!PPObservationReceiveTime(&c,0,0,20000,&rx));
    assert(PPObservationClockUpdate(&c,UINT32_MAX-999U,100,1000000));
    uint64_t start=c.now_us;
    assert(PPObservationClockUpdate(&c,1000,102,1000000));
    assert(c.now_us-start==2000);
    assert(PPObservationReceiveTime(&c,500,101,20000,&rx));
    assert(c.now_us-rx==500);
    assert(!PPObservationReceiveTime(&c,500,1,20000,&rx));
    assert(!PPObservationReceiveTime(&c,1001,102,20000,&rx));
    uint32_t epoch=c.source_epoch;
    assert(!PPObservationClockUpdate(&c,1000,100000,1000000));
    assert(c.source_epoch!=epoch);
    epoch=c.source_epoch;
    assert(!PPObservationClockUpdate(&c,1001,100000,168000000));
    assert(c.source_epoch!=epoch);
    assert(!PPObservationClockUpdate(&c,1002,100000,0));
    assert(!c.initialized);
    assert(PPObservationClockUpdate(&c,0,0,168000000));
    for(uint32_t n=1;n<=100000;++n)
        assert(PPObservationClockUpdate(&c,n*169U,(uint32_t)((uint64_t)n*169/168000),168000000));
    assert(c.now_us==(uint64_t)100000*169/168);
    assert(c.fraction==((uint64_t)100000*169*1000000)%168000000);
    c=(PPObservationClock){0};
    assert(PPObservationClockUpdate(&c,0,UINT32_MAX,1000000));
    start=c.now_us;
    assert(PPObservationClockUpdate(&c,2000,1,1000000));
    assert(c.now_us-start==2000);
    assert(PPObservationClockUpdate(&c,2000,1,1000000));
    assert(!PPObservationClockUpdate(&c,1999,0,1000000));
    puts("speed observation time: PASS"); return 0;
}
