#include "chassis_traction.h"
#include "../../config/30_velocity_observation/chassis_traction_config.h"
#include <math.h>
#include <stddef.h>

#define PROJECTION_45 0.7071067811865475244f
#define TWO_PI 6.2831853071795864769f

ChassisTractionConfig_s ChassisTractionDefaultConfig(const ChassisVelocityConfig_s *geometry)
{
    ChassisTractionConfig_s c={
        .mode=CHASSIS_TRACTION_MODE,
        .absolute_slip_mps=CHASSIS_TRACTION_ABSOLUTE_SLIP_MPS,
        .relative_slip=CHASSIS_TRACTION_RELATIVE_SLIP,
        .min_wheel_mps=CHASSIS_TRACTION_MIN_WHEEL_MPS,
        .min_ground_mps=CHASSIS_TRACTION_MIN_GROUND_MPS,
        .release_ratio=CHASSIS_TRACTION_RELEASE_RATIO,
        .recovery_s=CHASSIS_TRACTION_RECOVERY_S,
        .accel_allowance_mps2=CHASSIS_TRACTION_ACCEL_ALLOWANCE_MPS2,
        .yaw_accel_allowance_radps2=CHASSIS_TRACTION_YAW_ACCEL_ALLOWANCE_RADPS2,
        .brake_gain=CHASSIS_TRACTION_BRAKE_GAIN,
        .max_brake_current=CHASSIS_TRACTION_MAX_BRAKE_CURRENT,
        .enter_ms=CHASSIS_TRACTION_ENTER_MS,.release_ms=CHASSIS_TRACTION_RELEASE_MS,
        .max_reference_age_ms=CHASSIS_TRACTION_MAX_REFERENCE_AGE_MS,
        .max_wheel_age_ms=CHASSIS_TRACTION_MAX_WHEEL_AGE_MS};
    if(geometry) c.geometry=*geometry;
    return c;
}

static bool ConfigValid(const ChassisTractionConfig_s *c)
{
    if(!c || c->mode>3) return false;
    const float positive[]={c->geometry.wheel_radius_m,c->geometry.reduction_ratio,
        c->geometry.wheel_base_m,c->geometry.track_width_m,c->absolute_slip_mps,
        c->min_wheel_mps,c->min_ground_mps,c->release_ratio,c->recovery_s};
    for(unsigned i=0;i<sizeof positive/sizeof positive[0];++i)
        if(!isfinite(positive[i]) || positive[i]<=0) return false;
    const float nonnegative[]={c->relative_slip,c->accel_allowance_mps2,
        c->yaw_accel_allowance_radps2,c->brake_gain,c->max_brake_current};
    for(unsigned i=0;i<sizeof nonnegative/sizeof nonnegative[0];++i)
        if(!isfinite(nonnegative[i]) || nonnegative[i]<0) return false;
    if(c->release_ratio>=1 || c->max_brake_current>32767 || !c->enter_ms ||
       c->enter_ms>1000 || !c->release_ms || c->release_ms>1000 ||
       !c->max_reference_age_ms || c->max_reference_age_ms>100 ||
       !c->max_wheel_age_ms || c->max_wheel_age_ms>20) return false;
    for(unsigned w=0;w<4;++w)
        if(c->geometry.omni_x && c->geometry.direction[w]!=1 && c->geometry.direction[w]!=-1)
            return false;
    return true;
}

static float Sign(float x) { return x<0 ? -1.0f:1.0f; }

/* Hysteresis and evidence timers advance only on new CAN feedback. Reading one
 * unchanged sample at 1 kHz must not manufacture persistent-slip evidence. */
static void Latch(bool entry, bool sustain, float residual, float release_threshold,
    float dt, float enter_s, float release_s, bool *active, float *enter, float *release)
{
    if(!sustain) { *active=false; *enter=*release=0; return; }
    if(*active) {
        *release=residual<=release_threshold ? *release+dt:0;
        if(*release>=release_s) { *active=false; *enter=*release=0; }
    } else {
        *enter=entry ? *enter+dt:0;
        if(*enter>=enter_s) { *active=true; *release=0; }
    }
}

bool ChassisTractionUpdate(const ChassisTractionConfig_s *c,
    const ChassisTractionInput_s *i, ChassisTractionState_s *s, ChassisTractionOutput_s *o)
{
    if(!o) return false;
    *o=(ChassisTractionOutput_s){0};
    for(unsigned w=0;w<4;++w) {
        o->current[w]=o->proposed_current[w]=i ? i->raw_current[w]:0;
        o->scale[w]=1;
    }
    ChassisTractionReason_e reason=CHASSIS_TRACTION_READY;
    if(!s || !i || !ConfigValid(c)) reason=CHASSIS_TRACTION_BAD_CONFIG;
    else if(!c->mode) reason=CHASSIS_TRACTION_OFF;
    else if(!i->drive_enabled || !i->targets_match) reason=CHASSIS_TRACTION_DRIVE_GATED;
    else if(!i->reference.valid || !isfinite(i->reference.vx_mps) ||
        !isfinite(i->reference.vy_mps) || !isfinite(i->reference.wz_radps))
        reason=CHASSIS_TRACTION_NO_REFERENCE;
    else if(i->now_ms-i->reference.tick_ms>c->max_reference_age_ms ||
        i->now_ms-i->reference.gyro_tick_ms>c->max_reference_age_ms)
        reason=CHASSIS_TRACTION_STALE_REFERENCE;
    else if(!isfinite(i->dt_s) || i->dt_s<=0 || i->dt_s>.005f)
        reason=CHASSIS_TRACTION_BAD_TIMING;
    else if((i->online_mask & i->enabled_mask & 15U)!=15U)
        reason=CHASSIS_TRACTION_BAD_WHEELS;
    if(reason==CHASSIS_TRACTION_READY) for(unsigned w=0;w<4;++w)
        if(!isfinite(i->raw_current[w]) || !isfinite(i->actual_rpm[w]) ||
           !isfinite(i->max_current[w]) || i->max_current[w]<=0 ||
           i->max_current[w]>32767 || !i->feedback_sequence[w] ||
           i->now_ms-i->feedback_tick_ms[w]>c->max_wheel_age_ms)
            reason=CHASSIS_TRACTION_BAD_WHEELS;
    if(reason!=CHASSIS_TRACTION_READY) {
        if(s) *s=(ChassisTractionState_s){0};
        o->reason=reason;
        return reason==CHASSIS_TRACTION_OFF;
    }

    const float age_s=(float)(i->now_ms-i->reference.tick_ms)*.001f;
    /* Transport the historical translation into current public chassis axes,
     * following the existing public wheel-wz sign. Acceleration is not predicted. */
    const float angle=i->reference.wz_radps*age_s, co=cosf(angle), si=sinf(angle);
    const float vx=co*i->reference.vx_mps-si*i->reference.vy_mps;
    const float vy=si*i->reference.vx_mps+co*i->reference.vy_mps;
    const float lever=(c->geometry.wheel_base_m+c->geometry.track_width_m)*.5f;
    const float projection=c->geometry.omni_x ? PROJECTION_45:1;
    const float rotation=i->reference.wz_radps*lever*projection;
    float expected[4];
    if(c->geometry.omni_x) {
        expected[0]=(-vx+vy)*projection+rotation;
        expected[1]=( vx+vy)*projection+rotation;
        expected[2]=(-vx-vy)*projection+rotation;
        expected[3]=( vx-vy)*projection+rotation;
        for(unsigned w=0;w<4;++w) expected[w]*=c->geometry.direction[w];
    } else {
        expected[0]=-vx-vy-rotation; expected[1]=-vx+vy-rotation;
        expected[2]= vx-vy-rotation; expected[3]= vx+vy-rotation;
    }
    const float scale=TWO_PI*c->geometry.wheel_radius_m/(60*c->geometry.reduction_ratio);
    const float gyro_age_s=(float)(i->now_ms-i->reference.gyro_tick_ms)*.001f;
    const float uncertainty=c->accel_allowance_mps2*age_s+
        c->yaw_accel_allowance_radps2*lever*projection*fmaxf(age_s,gyro_age_s);
    /* Reject finite-input arithmetic overflow before it can latch braking. */
    for(unsigned w=0;w<4;++w) if(!isfinite(expected[w]) ||
        !isfinite(i->actual_rpm[w]*scale) || !isfinite(uncertainty) ||
        !isfinite(c->absolute_slip_mps+c->relative_slip*fabsf(expected[w])+uncertainty)) {
        *s=(ChassisTractionState_s){0};o->reason=CHASSIS_TRACTION_BAD_WHEELS;return false;
    }
    o->valid=true;
    for(unsigned w=0;w<4;++w) {
        ChassisTractionWheel_s *a=&s->wheel[w];
        const float actual=i->actual_rpm[w]*scale, ground=expected[w];
        const float sign=Sign(actual), ground_sign=Sign(ground);
        const float threshold=c->absolute_slip_mps+c->relative_slip*fabsf(ground)+uncertainty;
        const float spin=fabsf(actual)-fmaxf(0,sign*ground);
        const float lock=fabsf(ground)-fmaxf(0,ground_sign*actual);
        const bool spin_possible=fabsf(actual)>=c->min_wheel_mps &&
            (sign*ground>=0 || fabsf(ground)<c->min_ground_mps);
        const bool lock_possible=fabsf(ground)>=c->min_ground_mps;
        const bool new_sample=!a->seen || i->feedback_sequence[w]!=a->sequence;
        if(new_sample) {
            uint32_t step=a->seen ? i->feedback_tick_ms[w]-a->tick_ms:0;
            if(!a->seen || step>c->max_wheel_age_ms ||
               (uint32_t)(i->feedback_sequence[w]-a->sequence)>=UINT32_C(0x80000000)) {
                *a=(ChassisTractionWheel_s){.drive_scale=1,.brake_scale=1};step=0;
            }
            a->seen=true;a->sequence=i->feedback_sequence[w];a->tick_ms=i->feedback_tick_ms[w];
            if(a->spin_sign!=sign) {a->spinning=false;a->spin_enter_s=a->spin_release_s=0;}
            if(a->lock_sign!=ground_sign) {a->locking=false;a->lock_enter_s=a->lock_release_s=0;}
            a->spin_sign=sign;a->lock_sign=ground_sign;
            const float dt=(float)step*.001f;
            Latch(spin_possible && spin>threshold && i->raw_current[w]*sign>0,
                spin_possible,spin,threshold*c->release_ratio,dt,
                (float)c->enter_ms*.001f,(float)c->release_ms*.001f,
                &a->spinning,&a->spin_enter_s,&a->spin_release_s);
            Latch(lock_possible && lock>threshold && i->raw_current[w]*ground_sign<0,
                lock_possible,lock,threshold*c->release_ratio,dt,
                (float)c->enter_ms*.001f,(float)c->release_ms*.001f,
                &a->locking,&a->lock_enter_s,&a->lock_release_s);
        }
        a->drive_scale=a->spinning ? 0:fminf(1,a->drive_scale+i->dt_s/c->recovery_s);
        a->brake_scale=a->locking ? 0:fminf(1,a->brake_scale+i->dt_s/c->recovery_s);
        float current=i->raw_current[w];
        if(current*sign>0) {current*=a->drive_scale;o->scale[w]=a->drive_scale;}
        if(current*ground_sign<0 && lock_possible) {
            current*=a->brake_scale;o->scale[w]=fminf(o->scale[w],a->brake_scale);
        }
        /* Never add a reverse-driving command during a lock-up. Active braking
         * is restricted to confirmed overspin, current wheel direction and cap. */
        if(c->mode==3 && a->spinning && !a->locking && spin_possible &&
           spin>threshold*c->release_ratio) {
            const float cap=fminf(c->max_brake_current,i->max_current[w]);
            const float brake=fminf(cap,c->brake_gain*fmaxf(0,spin-threshold*c->release_ratio));
            if(current*sign>-brake) {current=-sign*brake;o->active_brake_mask|=(uint8_t)(1U<<w);}
        }
        o->actual_mps[w]=actual;o->expected_mps[w]=ground;
        o->slip_mps[w]=fabsf(ground)>=c->min_ground_mps ? actual*ground_sign-fabsf(ground):spin;
        o->threshold_mps[w]=threshold;o->proposed_current[w]=current;
        if(a->spinning) o->spin_mask|=(uint8_t)(1U<<w);
        if(a->locking) o->lock_mask|=(uint8_t)(1U<<w);
        if(c->mode>=2) {
            o->current[w]=current;
            if(current!=i->raw_current[w]) o->intervened_mask|=(uint8_t)(1U<<w);
        }
    }
    return true;
}
