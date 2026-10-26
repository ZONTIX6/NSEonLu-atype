#include "test_support.h"
#include <math.h>
#include <stdio.h>
static const FcPulse pulse={.wheel=1,.current=300,.rise_ms=50,.hold_ms=100,.fall_ms=50};
static void waveform(void)
{
    FcEngine e;FcInput in;uint32_t session;ready_engine(&e,&in,&session);
    assert(FcStart(&e,session,&pulse,&in));uint32_t start=in.now_us;
    FcDrive d=advance_ms(&e,&in,session,75);
    assert(d.current[0]==0 && d.current[1]==150 && d.current[2]==0 && d.current[3]==0);
    d=advance_ms(&e,&in,session,175);
    assert(in.now_us-start==250000 && d.current[1]==0 && d.state==FC_TAIL);
    d=advance_ms(&e,&in,session,100);assert(d.state==FC_FROZEN);
    FcStatus s;FcGetStatus(&e,&s);assert(s.complete && s.zero_submitted);
    assert(!FcStart(&e,session,&pulse,&in));
    assert(FcArm(&e,&in,&session));assert(!FcStart(&e,session,&pulse,&in));
    advance_ms(&e,&in,session,101);FcPulse negative=pulse;negative.current=-300;
    assert(FcStart(&e,session,&negative,&in));assert(advance_ms(&e,&in,session,75).current[1]==-150);
}
static void guards(void)
{
    for(unsigned fault=0;fault<18;fault++) {
        FcEngine e;FcInput in;uint32_t session;ready_engine(&e,&in,&session);
        assert(FcStart(&e,session,&pulse,&in));advance_ms(&e,&in,session,110);
        in.now_us+=1000;
        switch(fault) {
        case 0:in.usb_open=false;break;case 1:++in.usb_generation;break;
        case 2:in.bench_permitted=false;break;case 3:in.command_fresh=false;break;
        case 4:in.aux_stopped=false;break;case 5:in.feedback_age_us[3]=5001;break;
        case 6:in.temperature[0]=50;break;case 7:in.raw_rpm[2]=-1000;break;
        case 8:in.budget_valid=false;break;case 9:in.budget_w=49;break;
        case 10:in.load_w=51;break;case 11:in.used_rpm[2]=NAN;break;
        case 12:in.load_w=INFINITY;break;case 13:in.meter_fresh=false;break;
        case 14:in.now_us+=5000;break;case 15:in.feedback_seq[1]=0;in.feedback_age_us[1]=5001;break;
        case 16:FcCancel(&e,FC_REASON_STOP,in.now_us);break;
        case 17:FcRecordSubmission(&e,in.now_us,e.status.final,false);break;
        }
        FcDrive d=FcStep(&e,&in);for(unsigned w=0;w<4;w++) assert(d.current[w]==0);
        assert(d.reason!=FC_REASON_NONE);FcReason reason=d.reason;
        FcCancel(&e,FC_REASON_PROTOCOL,in.now_us);assert(e.status.reason==reason);
    }
}
static void ranges_final_and_wrap(void)
{
    FcEngine e;FcInput in;uint32_t session;ready_engine(&e,&in,&session);
    for(unsigned k=0;k<7;k++) {
        FcPulse bad=pulse;
        switch(k) {case 0:bad.wheel=4;break;case 1:bad.current=0;break;
        case 2:bad.current=1501;break;case 3:bad.rise_ms=0;break;case 4:bad.hold_ms=0;break;
        case 5:bad.fall_ms=0;break;case 6:bad.hold_ms=500;break;}
        assert(!FcStart(&e,session,&bad,&in));
    }
    assert(!FcStart(&e,session-1,&pulse,&in));assert(FcStart(&e,session,&pulse,&in));
    advance_ms(&e,&in,session,110);int16_t current[4]={1,300,0,0};
    assert(!FcValidateFinal(&e,&in,current));
    ready_engine(&e,&in,&session);assert(FcStart(&e,session,&pulse,&in));
    advance_ms(&e,&in,session,110);current[0]=0;current[1]=1501;
    assert(!FcValidateFinal(&e,&in,current));
    FcLimits limits=BoardForceCalibrationLimits();FcInit(&e,&limits);in=permitted_input();
    in.now_us=UINT32_MAX-50000;e.next_session=UINT32_MAX;
    assert(FcArm(&e,&in,&session) && session!=0);FcStep(&e,&in);
    advance_ms(&e,&in,session,101);assert(FcStart(&e,session,&pulse,&in));
    in.feedback_seq[0]=UINT32_MAX;assert(advance_ms(&e,&in,session,75).current[1]==150);
    in.now_us+=150001;assert(FcStep(&e,&in).current[1]==0);
    limits.max_current=0;FcInit(&e,&limits);assert(!FcArm(&e,&in,&session));
}
int main(void) {waveform();guards();ranges_final_and_wrap();puts("calibration engine: OK");return 0;}
