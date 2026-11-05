#include "shadow_jscope.h"
#include "chassis_power_shadow.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static ChassisPowerShadowSnapshot sample;
static bool available, fail_write;
static int config_result=-1;
static unsigned writes, configurations;
static unsigned char packet[48];
static uint32_t clock_ms, preemption_ms;
uint32_t HAL_GetTick(void) {return clock_ms;}
static void poll(uint32_t now) {clock_ms=now;ShadowJScopePoll(now);}
bool ChassisPowerShadowGet(ChassisPowerShadowSnapshot *s)
{clock_ms+=preemption_ms;*s=sample;return available;}
int SEGGER_RTT_ConfigUpBuffer(unsigned channel,const char *name,void *buffer,unsigned size,unsigned mode)
{
    assert(channel==2 && buffer && size>=2*sizeof packet && mode==0);
    assert(strcmp(name,"JScope_t4f4f4f4f4f4f4f4f4f4u4u4")==0);
    ++configurations;return config_result;
}
unsigned SEGGER_RTT_Write(unsigned channel,const void *data,unsigned size)
{
    assert(channel==2 && size==sizeof packet);
    memcpy(packet,data,size);++writes;return fail_write ? 0 : size;
}
static float f(unsigned index) {float v;memcpy(&v,packet+4+index*4,4);return v;}
static uint32_t u(unsigned offset) {uint32_t v;memcpy(&v,packet+offset,4);return v;}
static void advance(uint32_t tick) {sample.tick_ms=tick;++sample.cycles;poll(tick);}
int main(void)
{
    const unsigned w=CHASSIS_SHADOW_JSCOPE_WHEEL;
    ShadowJScopeInit();assert(configurations==1);
    poll(0);assert(writes==0);
    config_result=0;ShadowJScopeInit();assert(configurations==2);
    ShadowJScopeInit();assert(configurations==2);
    available=true;sample.cycles=1;sample.tick_ms=100;
    sample.eligible=true;sample.allocation.feasible=true;sample.allocation.command[w]=-123;
    sample.allocation.power_w=42;sample.final.command[w]=-100;sample.final.submitted=true;
    sample.measured_w=40;sample.measured_valid=true;sample.baseline_envelope=true;
    sample.wheel_kf.configured=true;sample.wheel_kf.current[w].valid=true;
    sample.wheel_kf.current[w].omega=6.283185307f;sample.wheel_kf.current[w].alpha=2;
    for(unsigned h=0;h<3;++h) {sample.wheel_kf.future_valid_mask[h]=(uint8_t)(1U<<w);sample.wheel_kf.future_omega[h][w]=12.56637061f;}
    poll(100);assert(writes==1 && u(0)==100000);
    assert(fabsf(f(0)-60)<.001f && fabsf(f(2)-120)<.001f);
    assert(f(4)==2 && f(5)==-123 && f(6)==-100 && f(7)==42 && f(8)==40);
    assert((u(40)&0x17f)==0x17f && u(44)==0);
    advance(110);assert(writes==1);advance(120);assert(writes==2);
    poll(140);assert(writes==2); /* Same publication is not a sample. */
    poll(200);assert(writes==2); /* Stale publication is not replayed. */
    sample.wheel_kf.paused=true;sample.eligible=false;sample.measured_valid=false;
    advance(220);assert(writes==3);
    assert(isnan(f(0)) && isnan(f(1)) && isnan(f(5)) && isnan(f(7)) && isnan(f(8)));
    assert(f(6)==-100 && (u(40)&0x23f)==0x200);
    fail_write=true;advance(240);assert(writes==4);
    fail_write=false;advance(260);assert(writes==5 && u(44)==1);
    advance(UINT32_MAX-9);assert(writes==6);
    advance(10);assert(writes==7 && u(0)==10000);
    available=false;advance(30);assert(writes==7);
    /* A motor interrupt publishes a newer sample while the getter runs. */
    available=true;sample.tick_ms=52;++sample.cycles;preemption_ms=2;
    const uint32_t stale_before=shadow_jscope_diagnostics.stale;
    poll(50);assert(writes==8 && shadow_jscope_diagnostics.stale==stale_before);
    puts("PASS: shadow J-Scope packet, validity, timing, wrap and nonblocking loss");
    return 0;
}
