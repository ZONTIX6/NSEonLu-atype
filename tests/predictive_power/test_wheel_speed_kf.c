#include "wheel_speed_kf.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static PPWheelKFConfig config(void)
{
    return (PPWheelKFConfig){.version=1,.max_gap_us=20000,.max_age_us=20000,
        .max_horizon_us=20000,.q=1,.r=.25f,.initial_p00=1,.initial_p11=100,
        .gate_squared=25,.max_rejections=3};
}
static PPWheelSample sample(uint32_t seq,uint64_t us,float omega)
{
    return (PPWheelSample){.rx_time_us=us,.sequence=seq,.source_epoch=1,
        .omega_rad_s=omega,.online=true};
}
static void psd(const PPWheelEstimate *e)
{
    assert(isfinite(e->omega) && isfinite(e->alpha));
    assert(isfinite(e->p00) && isfinite(e->p01) && isfinite(e->p11));
    assert(e->p00>=0 && e->p11>=0);
    assert((double)e->p00*e->p11-(double)e->p01*e->p01>=-1e-6);
}
static void initial_and_prediction(void)
{
    PPWheelKFConfig c=config(); PPWheelKF k={0};
    PPWheelSample s=sample(1,100000,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    PPWheelKF before=k; PPWheelEstimate e;
    assert(PPWheelKFPredict(&k,&c,100000,8000,&e));
    assert(fabsf(e.omega-10)<1e-6f && e.p00>k.corrected.p00);
    assert(memcmp(&before,&k,sizeof k)==0);
    assert(e.output_time_us==108000 && e.corrected_time_us==100000);
    assert(PPWheelKFPredict(&k,&c,101000,20000,&e));
    assert(e.output_time_us==121000);
    assert(!PPWheelKFPredict(&k,&c,121000,0,&e) && !e.valid);
    assert(e.reason==PP_KF_STALE);
    assert(!PPWheelKFPredict(&k,&c,99999,0,&e));
    assert(!PPWheelKFPredict(&k,&c,100000,20001,&e));
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE);
    assert(k.corrected.corrected_time_us==100000);
}
static void exact_update(void)
{
    PPWheelKFConfig c=config(); c.q=0; c.r=1; c.initial_p11=1;
    c.max_gap_us=2000000; c.max_age_us=2000000;
    PPWheelKF k={0}; PPWheelSample s=sample(1,1000000,0);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    s=sample(2,2000000,3);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_OK);
    /* Hand-derived: prior P=[2,1;1,1], S=3, K=[2/3,1/3]. */
    assert(fabsf(k.corrected.omega-2)<1e-6f);
    assert(fabsf(k.corrected.alpha-1)<1e-6f);
    assert(fabsf(k.corrected.p00-2.0f/3)<1e-6f);
    assert(fabsf(k.corrected.p01-1.0f/3)<1e-6f);
    assert(fabsf(k.corrected.p11-2.0f/3)<1e-6f);
    psd(&k.corrected);
}
static void watermarks_and_wrap(void)
{
    PPWheelKFConfig c=config(); PPWheelKF k={0};
    PPWheelSample s=sample(UINT32_MAX,100000,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    s=sample(0,100100,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_OK);
    s=sample(1,100200,10); /* Same millisecond, different real receive event. */
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_OK);
    s.omega_rad_s=NAN;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE);
    s=sample(UINT32_MAX,100300,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_TIME);
    assert(k.seen_sequence==1 && k.corrected.sequence==1);
    s=sample(2,100200,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_TIME);
    assert(k.corrected.sequence==1 && k.corrected.corrected_time_us==100200);
    assert(k.seen_sequence==2 && k.seen_time_us==100200);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE);
    s=sample(3,100100,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_TIME);
    assert(k.seen_time_us==100200);
    s=sample(4,100400,10);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_OK);
}
static void rejection_and_recovery(void)
{
    PPWheelKFConfig c=config(); PPWheelKF k={0};
    PPWheelSample s=sample(1,100000,0);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    for(unsigned n=2;n<=4;++n) {
        s=sample(n,100000+n*1000,1000);
        assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INNOVATION);
        unsigned rejected=k.rejected_consecutive;
        assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE);
        assert(k.rejected_consecutive==rejected);
        assert(k.corrected.corrected_time_us==100000);
    }
    assert(!k.corrected.valid);
    s=sample(5,105000,1000);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    assert(k.corrected.omega==1000 && k.corrected.alpha==0);
    s=sample(6,106000,NAN);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_VALUE);
    assert(k.corrected.corrected_time_us==105000);
    s.omega_rad_s=INFINITY;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE);
    s=sample(7,107000,1000); s.online=false;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_STALE && !k.corrected.valid);
    s.online=true;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE);
    s=sample(8,108000,999);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    s=sample(9,200000,500);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    c.version=2;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_DUPLICATE && !k.corrected.valid);
    s=sample(10,201000,100);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    s=sample(1,1,-20); s.source_epoch=2;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    assert(k.corrected.omega==-20 && k.corrected.source_epoch==2);
    s.source_epoch=1;
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_TIME);
}
static void invalid_numeric(void)
{
    PPWheelKFConfig c=config(); assert(PPWheelKFConfigValid(&c));
    PPWheelKFConfig bad=c;
    bad.r=0; assert(!PPWheelKFConfigValid(&bad));
    bad=c; bad.q=NAN; assert(!PPWheelKFConfigValid(&bad));
    bad=c; bad.gate_squared=INFINITY; assert(!PPWheelKFConfigValid(&bad));
    bad=c; bad.max_rejections=0; assert(!PPWheelKFConfigValid(&bad));
    bad=c; bad.version=0; assert(!PPWheelKFConfigValid(&bad));
    PPWheelKF k={0}; PPWheelSample s=sample(1,100000,1);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    k.corrected.p01=1000;
    PPWheelEstimate e;
    assert(!PPWheelKFPredict(&k,&c,100001,0,&e) && e.reason==PP_KF_NUMERIC);
    s=sample(2,100100,1);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_NUMERIC && !k.corrected.valid);
    s=sample(3,100200,1);
    assert(PPWheelKFCorrect(&k,&c,&s)==PP_KF_INITIALIZED);
    k.corrected.corrected_time_us=UINT64_MAX-5;
    assert(!PPWheelKFPredict(&k,&c,UINT64_MAX-1,10,&e));
    PPWheelKFReset(&k); assert(!k.initialized && !k.seen);
}
static void constant_acceleration_and_quantization(void)
{
    PPWheelKFConfig c=config(); c.q=10; c.r=.01f; c.gate_squared=10000;
    PPWheelKF k={0}; uint64_t us=100000; double squared=0; unsigned count=0;
    for(unsigned n=0;n<5000;++n) {
        us += n%3==0 ? 3000 : 1000;
        const float truth=-10.0f+2.0f*(float)(us-100000)*1e-6f;
        float quantized=roundf(truth*10)/10;
        PPWheelSample s=sample(n+1,us,quantized);
        PPWheelKFReason reason=PPWheelKFCorrect(&k,&c,&s);
        assert(reason==PP_KF_INITIALIZED || reason==PP_KF_OK);
        psd(&k.corrected);
        PPWheelEstimate out;
        assert(PPWheelKFPredict(&k,&c,us+500,8000,&out));
        if(n>500) {
            double error=out.omega-(truth+.017f);
            squared+=error*error; ++count;
        }
    }
    assert(sqrt(squared/count)<.035);
    assert(fabsf(k.corrected.alpha-2)<.2f);
}
int main(void)
{
    initial_and_prediction(); exact_update(); watermarks_and_wrap();
    rejection_and_recovery(); invalid_numeric(); constant_acceleration_and_quantization();
    puts("wheel speed KF: PASS"); return 0;
}
