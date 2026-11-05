#include "shadow_jscope.h"
volatile ShadowJScopeDiagnostics shadow_jscope_diagnostics;
#if CHASSIS_SHADOW_JSCOPE_ENABLE
#include "chassis_power_shadow.h"
#include "chassis_control_config.h"
#include "board_config.h"
#include "SEGGER_RTT.h"
#include "main.h"
#include <math.h>
#include <stddef.h>
#ifdef STM32F427xx
#include "bsp_dwt.h"
#endif
#if CHASSIS_PREDICTIVE_POWER_MODE != CHASSIS_PREDICTIVE_SHADOW
#error Shadow J-Scope requires CHASSIS_PREDICTIVE_POWER_MODE=1
#endif
#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED || !BOARD_LOG_RTT_ENABLED
#error Shadow J-Scope requires speed control and RTT logging (LOG_OUTPUT=1 or 3)
#endif

/* RTT-Normal permits twelve variables including the microsecond timestamp. */
typedef struct {
    uint32_t time_us;
    float values[9];
    uint32_t flags, dropped;
} ShadowPacket;
_Static_assert(sizeof(float)==4 && sizeof(ShadowPacket)==48 &&
               offsetof(ShadowPacket,flags)==40,"J-Scope wire layout");
static unsigned char up_buffer[2048];
static ChassisPowerShadowSnapshot snapshot;
static bool ready, polled, seen;
static uint32_t last_poll_ms, last_cycle;

void ShadowJScopeInit(void)
{
    if(ready) return;
    shadow_jscope_diagnostics.init_result=SEGGER_RTT_ConfigUpBuffer(2,
        "JScope_t4f4f4f4f4f4f4f4f4f4u4u4",up_buffer,sizeof up_buffer,
        SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    ready=shadow_jscope_diagnostics.init_result>=0;
}

void ShadowJScopePoll(uint32_t now_ms)
{
    /* Ceil to the 5 ms observer cadence: requested rate is an upper bound. */
    const uint32_t interval_ms=((1000U+5U*CHASSIS_SHADOW_JSCOPE_HZ-1U)/
                                (5U*CHASSIS_SHADOW_JSCOPE_HZ))*5U;
    if(!ready || (polled && (uint32_t)(now_ms-last_poll_ms)<interval_ms)) return;
    last_poll_ms=now_ms;polled=true;
    if(!ChassisPowerShadowGet(&snapshot)) return;
    /* The motor task may publish after the caller sampled now_ms. */
    now_ms=HAL_GetTick();
    if((uint32_t)(now_ms-snapshot.tick_ms)>20U) {++shadow_jscope_diagnostics.stale;return;}
    if(seen && snapshot.cycles==last_cycle) return;
    seen=true;last_cycle=snapshot.cycles;
#ifdef STM32F427xx
    const uint32_t start=DWT_GetCycleCount();
#endif
    const unsigned w=CHASSIS_SHADOW_JSCOPE_WHEEL;
    const ChassisWheelKFDiagnostics *kf=&snapshot.wheel_kf;
    const bool kf_ready=kf->configured && !kf->paused;
    ShadowPacket packet={.time_us=snapshot.tick_ms*1000U,
        .flags=((uint32_t)snapshot.reason<<16),.dropped=shadow_jscope_diagnostics.dropped};
    for(unsigned n=0;n<9;++n) packet.values[n]=NAN;
    if(kf_ready && kf->current[w].valid && isfinite(kf->current[w].omega) && isfinite(kf->current[w].alpha)) {
        packet.values[0]=kf->current[w].omega*9.549296586f;
        packet.values[4]=kf->current[w].alpha;
        packet.flags|=1U;
    }
    for(unsigned h=0;h<3;++h) {
        if(kf_ready && (kf->future_valid_mask[h]&(1U<<w)) && isfinite(kf->future_omega[h][w])) {
            packet.values[1+h]=kf->future_omega[h][w]*9.549296586f;
            packet.flags|=1U<<(h+1);
        }
    }
    if(snapshot.eligible && snapshot.allocation.feasible && isfinite(snapshot.allocation.power_w)) {
        packet.values[5]=snapshot.allocation.command[w];
        packet.values[7]=snapshot.allocation.power_w;
        packet.flags|=1U<<4;
    }
    packet.values[6]=snapshot.final.command[w];
    if(snapshot.measured_valid && isfinite(snapshot.measured_w)) {
        packet.values[8]=snapshot.measured_w;packet.flags|=1U<<5;
    }
    if(snapshot.final.submitted) packet.flags|=1U<<6;
    if(snapshot.dynamic_model) packet.flags|=1U<<7;
    if(snapshot.baseline_envelope) packet.flags|=1U<<8;
    if(kf->paused) packet.flags|=1U<<9;
    if(SEGGER_RTT_Write(2,&packet,sizeof packet)==sizeof packet) ++shadow_jscope_diagnostics.sent;
    else ++shadow_jscope_diagnostics.dropped;
#ifdef STM32F427xx
    const uint32_t hz=DWT_GetFrequencyHz();
    const uint32_t us=hz ? (uint32_t)((uint64_t)(uint32_t)(DWT_GetCycleCount()-start)*1000000U/hz):UINT32_MAX;
    if(us>shadow_jscope_diagnostics.elapsed_max_us) shadow_jscope_diagnostics.elapsed_max_us=us;
#endif
}
#else
void ShadowJScopeInit(void) {}
void ShadowJScopePoll(uint32_t now_ms) {(void)now_ms;}
#endif
