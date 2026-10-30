#include "wheel_speed_kf.h"
#include <float.h>
#include <math.h>
#include <string.h>

bool PPWheelKFConfigValid(const PPWheelKFConfig *c)
{
    return c && c->version && c->max_gap_us && c->max_age_us && c->max_horizon_us &&
        c->max_gap_us<UINT32_C(0x80000000) && c->max_age_us<UINT32_C(0x80000000) &&
        c->max_horizon_us<UINT32_C(0x80000000) && c->max_rejections &&
        isfinite(c->q) && c->q>=0 && isfinite(c->r) && c->r>0 &&
        isfinite(c->initial_p00) && c->initial_p00>0 &&
        isfinite(c->initial_p11) && c->initial_p11>0 &&
        isfinite(c->gate_squared) && c->gate_squared>0;
}
void PPWheelKFReset(PPWheelKF *k) {if(k) memset(k,0,sizeof *k);}

/* Double products avoid overflow in the PSD test itself. The stored state and
 * update arithmetic remain float; only roundoff-sized violations are repaired. */
static bool numeric(PPWheelEstimate *e)
{
    if(!isfinite(e->omega) || !isfinite(e->alpha) || !isfinite(e->p00) ||
       !isfinite(e->p01) || !isfinite(e->p11) || !isfinite(e->innovation) ||
       !isfinite(e->innovation_variance) || e->p00<0 || e->p11<0) return false;
    const double diagonal=(double)e->p00*e->p11,cross=(double)e->p01*e->p01;
    const double tolerance=32.0*FLT_EPSILON*(diagonal+cross+FLT_MIN);
    if(diagonal-cross < -tolerance) return false;
    if(cross>diagonal) e->p01=copysignf((float)sqrt(diagonal),e->p01);
    return true;
}
static bool propagate(PPWheelEstimate *e,const PPWheelKFConfig *c,float dt)
{
    if(!numeric(e) || !isfinite(dt) || dt<0) return false;
    const float a=e->p00,b=e->p01,d=e->p11;
    e->omega+=dt*e->alpha;
    e->p00=a+2*dt*b+dt*dt*d+c->q*dt*dt*dt/3;
    e->p01=b+dt*d+c->q*dt*dt/2;
    e->p11=d+c->q*dt;
    return numeric(e);
}
static PPWheelKFReason reject(PPWheelKF *k,PPWheelKFReason reason,bool count)
{
    k->corrected.reason=reason;
    if(count && k->rejected_consecutive<UINT8_MAX) ++k->rejected_consecutive;
    return reason;
}
static PPWheelKFReason new_rejection(PPWheelKF *k,const PPWheelKFConfig *c,
                                     PPWheelKFReason reason)
{
    reject(k,reason,true);
    if(k->rejected_consecutive>=c->max_rejections) {
        k->initialized=false;k->corrected.valid=false;
    }
    return reason;
}
static PPWheelKFReason initialize(PPWheelKF *k,const PPWheelKFConfig *c,
                                  const PPWheelSample *s)
{
    k->corrected=(PPWheelEstimate){.omega=s->omega_rad_s,.p00=c->initial_p00,
        .p11=c->initial_p11,.corrected_time_us=s->rx_time_us,.output_time_us=s->rx_time_us,
        .sequence=s->sequence,.source_epoch=s->source_epoch,.config_version=c->version,
        .valid=true,.reason=PP_KF_INITIALIZED};
    k->initialized=true;k->rejected_consecutive=0;return PP_KF_INITIALIZED;
}
PPWheelKFReason PPWheelKFCorrect(PPWheelKF *k,const PPWheelKFConfig *c,const PPWheelSample *s)
{
    if(!k) return PP_KF_VALUE;
    if(!PPWheelKFConfigValid(c)) {
        k->corrected.valid=false;k->initialized=false;
        return reject(k,PP_KF_CONFIG,false);
    }
    if(!s) return reject(k,PP_KF_VALUE,false);
    if(k->initialized && k->corrected.config_version!=c->version) {
        k->initialized=false;k->corrected.valid=false;
    }
    if(!s->source_epoch) return reject(k,PP_KF_TIME,false);
    if(k->seen && s->source_epoch!=k->seen_source_epoch) {
        uint32_t advance=s->source_epoch-k->seen_source_epoch;
        if(advance>=UINT32_C(0x80000000)) return reject(k,PP_KF_TIME,false);
        k->seen=false;k->initialized=false;k->corrected.valid=false;
    }
    bool time_ok=true;
    if(k->seen) {
        const uint32_t advance=s->sequence-k->seen_sequence;
        if(!s->online) {k->initialized=false;k->corrected.valid=false;}
        if(!advance) return reject(k,s->online ? PP_KF_DUPLICATE : PP_KF_STALE,false);
        if(advance>=UINT32_C(0x80000000)) return reject(k,PP_KF_TIME,false);
        time_ok=s->rx_time_us>k->seen_time_us;
    }
    /* Consume forward receive identities even when their values are rejected.
     * A backwards timestamp never lowers the independent time watermark. */
    k->seen_sequence=s->sequence;k->seen_source_epoch=s->source_epoch;
    if(!k->seen || s->rx_time_us>k->seen_time_us) k->seen_time_us=s->rx_time_us;
    k->seen=true;
    if(!s->online) {
        k->initialized=false;k->corrected.valid=false;
        return reject(k,PP_KF_STALE,false);
    }
    if(!time_ok) return new_rejection(k,c,PP_KF_TIME);
    if(!isfinite(s->omega_rad_s)) return new_rejection(k,c,PP_KF_VALUE);
    if(!k->initialized) return initialize(k,c,s);
    if(s->rx_time_us<=k->corrected.corrected_time_us) return new_rejection(k,c,PP_KF_TIME);
    const uint64_t elapsed=s->rx_time_us-k->corrected.corrected_time_us;
    if(elapsed>c->max_gap_us) return initialize(k,c,s);
    PPWheelEstimate next=k->corrected;
    if(!propagate(&next,c,(float)elapsed*1e-6f)) {
        k->initialized=false;k->corrected.valid=false;
        return reject(k,PP_KF_NUMERIC,false);
    }
    const float residual=s->omega_rad_s-next.omega,variance=next.p00+c->r;
    if(!isfinite(residual) || !isfinite(variance) || variance<=0) {
        k->initialized=false;k->corrected.valid=false;
        return reject(k,PP_KF_NUMERIC,false);
    }
    /* Scale instead of squaring arbitrary residuals in float. */
    const double normalized=(double)residual*residual/variance;
    k->corrected.innovation=residual;k->corrected.innovation_variance=variance;
    if(normalized>c->gate_squared) return new_rejection(k,c,PP_KF_INNOVATION);
    const float a=next.p00,b=next.p01,d=next.p11,k0=a/variance,k1=b/variance;
    next.omega+=k0*residual;next.alpha+=k1*residual;
    next.p00=(1-k0)*(1-k0)*a+k0*k0*c->r;
    next.p01=(1-k0)*(b-k1*a)+k0*k1*c->r;
    next.p11=d-2*k1*b+k1*k1*a+k1*k1*c->r;
    next.innovation=residual;next.innovation_variance=variance;
    if(!numeric(&next)) {
        k->initialized=false;k->corrected.valid=false;
        return reject(k,PP_KF_NUMERIC,false);
    }
    next.sequence=s->sequence;next.corrected_time_us=s->rx_time_us;
    next.output_time_us=s->rx_time_us;next.reason=PP_KF_OK;next.valid=true;
    k->corrected=next;k->rejected_consecutive=0;return PP_KF_OK;
}
bool PPWheelKFPredict(const PPWheelKF *k,const PPWheelKFConfig *c,
                     uint64_t now,uint32_t horizon,PPWheelEstimate *out)
{
    if(!out) return false;
    *out=(PPWheelEstimate){.reason=PP_KF_VALUE};
    if(!PPWheelKFConfigValid(c)) {out->reason=PP_KF_CONFIG;return false;}
    if(!k) return false;
    *out=k->corrected;out->valid=false;
    if(!k->initialized || !k->corrected.valid) return false;
    if(k->corrected.config_version!=c->version) {out->reason=PP_KF_CONFIG;return false;}
    if(now<k->corrected.corrected_time_us || horizon>c->max_horizon_us ||
       now>UINT64_MAX-horizon) {out->reason=PP_KF_TIME;return false;}
    if(now-k->corrected.corrected_time_us>c->max_age_us) {out->reason=PP_KF_STALE;return false;}
    const uint64_t target=now+horizon;
    if(!propagate(out,c,(float)(target-k->corrected.corrected_time_us)*1e-6f)) {
        out->reason=PP_KF_NUMERIC;return false;
    }
    out->output_time_us=target;out->reason=PP_KF_OK;out->valid=true;return true;
}
