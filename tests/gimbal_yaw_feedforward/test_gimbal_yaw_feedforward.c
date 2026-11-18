#include "../../application/gimbal/gimbal_yaw_feedforward.h"
#include "../../config/03_gimbal/gimbal_config.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

static unsigned failures;
#define CHECK(c) do {if (!(c)) {printf("FAIL line %d: %s\n",__LINE__,#c);++failures;}} while(0)
#define NEAR(a,b) CHECK(fabsf((a)-(b))<0.01f)

/* Refresh wheel feedback while advancing the actual task clock. */
static float Advance(GimbalYawFeedforward_s *state, bool spin, float speed,
    uint32_t *now, unsigned duration_ms, unsigned step_ms)
{
    float out=state->output;
    for (unsigned elapsed=0;elapsed<duration_ms;elapsed+=step_ms) {
        *now+=step_ms;
        out=GimbalYawFeedforwardUpdate(state,true,spin,true,speed,*now,*now);
        CHECK(isfinite(out) && out>=0 && out<=YAW_SPIN_FEEDFORWARD_MAX_COMMAND);
    }
    return out;
}

static void CheckCleared(const GimbalYawFeedforward_s *state)
{
    CHECK(!state->active && state->output==0 && state->last_tick_ms==0);
}

int main(void)
{
    GimbalYawFeedforward_s state={0};
    if (!YAW_SPIN_FEEDFORWARD_ENABLE || YAW_SPIN_FEEDFORWARD_GAIN<=0) {
        state=(GimbalYawFeedforward_s){.active=true,.output=500,.last_tick_ms=100};
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,6.77f,101,101));
        CheckCleared(&state);
    } else {
        uint32_t now=100;
        /* Follow motion cannot arm FF; entry starts at zero without a step. */
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,3,now,now));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,6.77f,now,now));
        const float steady=6.77f*YAW_SPIN_FEEDFORWARD_GAIN;
        float out=0;
        for (unsigned i=0;i<80;++i) {
            float previous=out;
            out=Advance(&state,true,6.77f,&now,1,1);
            CHECK(out>previous && out<steady);
        }
        /* Independent continuous-time envelope: ~63% at tau, ~95% at 3*tau. */
        CHECK(out/steady>.62f && out/steady<.64f);
        out=Advance(&state,true,6.77f,&now,160,1);
        CHECK(out/steady>.94f && out/steady<.96f);
        out=Advance(&state,true,6.77f,&now,1000,5);
        CHECK(fabsf(out-steady)<.1f);
        NEAR(out,GimbalYawFeedforwardUpdate(&state,true,true,true,1,now,now));

        /* Normal exit still follows measured deceleration, smoothly. */
        float previous=out;
        out=Advance(&state,false,3,&now,5,5);
        CHECK(state.active && out<previous && out>2400);
        out=Advance(&state,false,3,&now,1200,5);
        NEAR(2400,out);
        previous=out;
        out=Advance(&state,false,.05f,&now,5,5);
        CHECK(!state.active && out>previous*.9f && out<previous);
        /* Follow motion during the stop tail must not re-arm compensation. */
        previous=out;
        out=Advance(&state,false,3,&now,5,5);
        CHECK(!state.active && out<previous);
        /* Re-entry during the tail preserves continuity instead of zeroing. */
        previous=out;
        out=Advance(&state,true,6.77f,&now,5,5);
        CHECK(state.active && out>previous && out<steady);
        out=Advance(&state,false,0,&now,1200,5);
        NEAR(0,out); CheckCleared(&state);
        NEAR(0,Advance(&state,false,3,&now,100,5));

        /* Variable task periods use elapsed time; repeating one tick adds none. */
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,6.77f,now,now));
        const unsigned steps[]={1,3,2,4};
        for (unsigned i=0;i<32;++i)
            out=Advance(&state,true,6.77f,&now,steps[i%4],steps[i%4]);
        CHECK(out/steady>.61f && out/steady<.64f); /* 80 ms total. */
        NEAR(out,GimbalYawFeedforwardUpdate(&state,true,true,true,6.77f,now,now));
        CHECK(fabsf(Advance(&state,true,FLT_MAX,&now,1600,5)-8000)<.1f);

        /* Every hard stop clears the stored filter, not only its returned value. */
        NEAR(0,GimbalYawFeedforwardUpdate(&state,false,true,true,1,now,now));
        CheckCleared(&state);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,now,now));
        CHECK(Advance(&state,true,1,&now,80,5)>400);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,false,1,now,now));
        CheckCleared(&state);
        const float bad[]={NAN,INFINITY,-INFINITY,-6.77f};
        for (unsigned i=0;i<4;++i) {
            GimbalYawFeedforwardUpdate(&state,true,true,true,1,now,now);
            CHECK(Advance(&state,true,1,&now,80,5)>400);
            NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,bad[i],now,now));
            CheckCleared(&state);
        }

        /* A repeated source timestamp never extends its 20 ms validity. */
        const uint32_t source=now;
        GimbalYawFeedforwardUpdate(&state,true,true,true,1,source,now);
        now+=20;
        CHECK(GimbalYawFeedforwardUpdate(&state,true,true,true,1,source,now)>0);
        ++now;
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,source,now));
        CheckCleared(&state);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,1,now,now));
        /* A task gap also restarts from zero even if the new wheels are fresh. */
        GimbalYawFeedforwardUpdate(&state,true,true,true,1,now,now);
        CHECK(Advance(&state,true,1,&now,80,5)>400);
        now+=21;
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,now,now));
        CHECK(state.active && state.output==0);

        /* Both source and filter clocks accept uint32 rollover. */
        state=(GimbalYawFeedforward_s){0}; now=UINT32_MAX-9;
        GimbalYawFeedforwardUpdate(&state,true,true,true,1,now,now);
        now=10;
        CHECK(GimbalYawFeedforwardUpdate(&state,true,true,true,1,UINT32_MAX-9,now)>0);
        now=11;
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,UINT32_MAX-9,now));
        CheckCleared(&state);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,201,200));
        CheckCleared(&state);
    }
    NEAR(0,GimbalYawFeedforwardUpdate(NULL,true,true,true,1,100,100));
    printf("yaw feedforward: %u failures\n",failures);
    return failures ? 1 : 0;
}
