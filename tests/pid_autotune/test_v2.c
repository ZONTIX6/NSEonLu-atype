#include "pid_autotune.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static PIDAutotuneFeedback f;
static bool frozen;
static PIDAutotuneTrial trial={.wheel=0,.rpm=500,.hold_ms=500,.current_limit=3000,.kp=.75f};
static void tick(PIDAutotune *a,uint32_t t,float rpm,float current,bool limited)
{
    f.now_ms=t;f.rpm[0]=rpm;
    for(unsigned w=0;w<4;++w) {f.rx_ms[w]=t/2*2;f.rx_sequence[w]=frozen?0:t/2;}
    PIDAutotuneKeepAlive(a,t);PIDAutotuneTick(a,&f);
    PIDAutotuneObserveV2(a,&f,rpm,current,current,limited);
}
static PIDAutotune begin(uint32_t start)
{
    PIDAutotune a={0};f=(PIDAutotuneFeedback){.now_ms=start,.permitted=true,.feedback_ok=true,.can_ok=true};
    assert(PIDAutotuneBeginV2(&a,1,&f));assert(PIDAutotuneStart(&a,&trial,&f));
    return a;
}
int main(void)
{
    PIDAutotune a=begin(0);
    for(unsigned t=0;t<200;++t) tick(&a,t,0,0,false);
    tick(&a,200,60,0,false);
    for(unsigned t=201;t<=500;++t) tick(&a,t,0,0,false);
    assert(PIDAutotuneTarget(&a,500)==0);
    tick(&a,501,0,0,false);assert(PIDAutotuneTarget(&a,501)==500);
    /* First 100 ms: error 100, next 400 ms: error 0. Time-weighted
       RMSE=sqrt(100^2*.1/.5); steady=0, saturation=.02/.5=4%.
       One output jump of 100 over .5s gives TV=200 units/s. */
    for(unsigned t=501;t<=1001;++t) {
        if(t==501 || t%3!=0) tick(&a,t,t<601?400:500,t<601?1000:1100,t<521);
    }
    /* A2 must wait for measured standstill after the step. */
    for(unsigned t=1002;t<=1402;++t) tick(&a,t,t<1102?60:0,0,false);
    assert(a.state==PID_AUTO_DONE && a.reason==PID_AUTO_NONE);
    assert(a.coverage_ms==500 && a.window_ms==500);
    assert(fabsf(a.rmse-sqrtf(2000))<.01f && a.steady==0);
    assert(a.limited_pm==40 && fabsf(a.jitter-200)<.01f);
    assert(a.feedback_count<a.samples && a.feedback_count>=200);
    assert(a.max_stat_ms==2 && a.max_feedback_ms==2);
    assert(a.settle_ms>=100 && a.settle_ms<=102);
    a=begin(0);
    for(unsigned t=0;t<=3000;++t) tick(&a,t,60,0,false);
    assert(!a.active && a.reason==PID_AUTO_STATIONARY);
    a=begin(0);
    for(unsigned t=0;t<=800;++t) {
        if(t>450 && t<481) continue;
        tick(&a,t,t>300?500:0,1000,false);
    }
    for(unsigned t=801;t<=1101;++t) tick(&a,t,0,0,false);
    assert(a.reason==PID_AUTO_QUALITY && a.coverage_ms<500 && a.max_stat_ms==31);
    a=begin(UINT32_MAX-150);
    for(unsigned t=0;t<=1101;++t) tick(&a,(UINT32_MAX-150)+t,t>300&&t<800?500:0,1000,false);
    assert(a.state==PID_AUTO_DONE && a.reason==PID_AUTO_NONE && a.coverage_ms==500);
    a=begin(0);frozen=true;
    for(unsigned t=0;t<=800;++t) {
        tick(&a,t,t>300?500:0,1000,false);
    }
    for(unsigned t=801;t<=1101;++t) tick(&a,t,0,0,false);
    assert(a.reason==PID_AUTO_QUALITY && a.feedback_count==1);frozen=false;
    a=begin(0);for(unsigned t=0;t<400;++t) tick(&a,t,t>300?500:0,1000,false);
    tick(&a,400,NAN,1000,false);assert(!a.active && a.reason==PID_AUTO_OVERSPEED);
    a=begin(0);
    for(unsigned n=0;n<50;++n){
        if(n)assert(PIDAutotuneStart(&a,&trial,&f));
        const uint32_t start=f.now_ms;
        for(unsigned t=0;t<=1101;++t)tick(&a,start+t,t>300&&t<800?500:0,1000,false);
        assert(a.state==PID_AUTO_DONE);
    }
    assert(a.trial==50 && !PIDAutotuneStart(&a,&trial,&f));
    puts("A2 standstill/time-weighted metrics/quality/wraparound tests passed");
}
