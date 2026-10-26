#ifndef FC_TEST_SUPPORT_H
#define FC_TEST_SUPPORT_H
#include "force_calibration.h"
#include "board_force_calibration.h"
#include <assert.h>
static FcInput permitted_input(void)
{
    return (FcInput){.now_us=1000,.usb_generation=9,.feedback_seq={1,1,1,1},
        .usb_open=true,.bench_permitted=true,.command_fresh=true,.aux_stopped=true,
        .meter_fresh=true,.budget_valid=true,.budget_w=50,.load_w=2};
}
static FcDrive advance_ms(FcEngine *e,FcInput *in,uint32_t session,unsigned ms)
{
    FcDrive d={0};
    for(unsigned k=0;k<ms;k++) {
        in->now_us+=1000;
        for(unsigned w=0;w<4;w++) ++in->feedback_seq[w];
        if(k%50==0) assert(FcHeartbeat(e,session,in->usb_generation,in->now_us));
        d=FcStep(e,in);FcRecordSubmission(e,in->now_us,d.current,true);
    }
    return d;
}
static void ready_engine(FcEngine *e,FcInput *in,uint32_t *session)
{
    const FcLimits limits=BoardForceCalibrationLimits();FcInit(e,&limits);
    *in=permitted_input();assert(FcArm(e,in,session));
    (void)FcStep(e,in);assert(advance_ms(e,in,*session,100).state==FC_READY);
}
#endif
