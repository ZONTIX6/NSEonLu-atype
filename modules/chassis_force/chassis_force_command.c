#include "chassis_force_command.h"
#include <math.h>
#include <stddef.h>
static float approach(float from,float to,float step)
{
    return from+fmaxf(-step,fminf(step,to-from));
}
bool ChassisForceCommand(const ChassisForceConfig_s *c,const ChassisForceIntent_s *i,
    float omega,uint32_t now,float dt,ChassisForceCommandState_s *s,ChassisForceCommandOutput_s *o)
{
    if(!c || !i || !s || !o) return false;
    *o=(ChassisForceCommandOutput_s){0};
    if(!isfinite(dt) || dt<=0 || dt>.005f || !isfinite(omega) || !isfinite(i->input_x) ||
        !isfinite(i->input_y) || !isfinite(i->force_scale) || i->force_scale<0 || i->force_scale>1 ||
        !isfinite(i->relative_yaw_rad) || i->motion_mode>CHASSIS_FORCE_SPIN ||
        (i->spin_direction!=1 && i->spin_direction!=-1)) return false;
    if(i->motion_mode==CHASSIS_FORCE_ZERO || !i->input_permitted) {*s=(ChassisForceCommandState_s){0};return true;}
    const float radius=hypotf(i->input_x,i->input_y);
    const float amplitude=radius<=c->input_deadzone ? 0 :
        c->force_max_n*i->force_scale*powf((fminf(radius,1)-c->input_deadzone)/(1-c->input_deadzone),c->input_gamma);
    const float desired[2]={radius>0 ? amplitude*i->input_x/radius : 0,radius>0 ? amplitude*i->input_y/radius : 0};
    const float dx=desired[0]-s->force_g[0],dy=desired[1]-s->force_g[1],distance=hypotf(dx,dy);
    const float rate=hypotf(desired[0],desired[1])>hypotf(s->force_g[0],s->force_g[1]) ? c->force_rise_n_s : c->force_fall_n_s;
    const float scale=distance>0 ? fminf(1,rate*dt/distance) : 0;
    s->force_g[0]+=dx*scale;s->force_g[1]+=dy*scale;
    const float co=cosf(i->relative_yaw_rad),si=sinf(i->relative_yaw_rad);
    o->high[0]=co*s->force_g[0]-si*s->force_g[1];
    o->high[1]=si*s->force_g[0]+co*s->force_g[1];
    o->force_g[0]=s->force_g[0];o->force_g[1]=s->force_g[1];
    if(i->motion_mode==CHASSIS_FORCE_SPIN) {
        s->braking=false;s->brake_timed_out=false;
        float target=i->spin_direction*c->spin_torque_nm;
        if(s->torque_nm*target<0) target=0;
        const float torque_rate=fabsf(target)>fabsf(s->torque_nm) ? c->torque_rise_nm_s : c->torque_fall_nm_s;
        s->torque_nm=approach(s->torque_nm,target,torque_rate*dt);o->low[2]=s->torque_nm;
    } else {
        if(s->previous_mode==CHASSIS_FORCE_SPIN) {
            s->torque_nm=0;
#if CHASSIS_FORCE_EXIT_MODE == CHASSIS_FORCE_EXIT_BRAKE
            s->braking=fabsf(omega)>c->brake_stop_rad_s;
            s->brake_sign=omega>=0 ? 1 : -1;s->brake_start_ms=now;
#endif
        }
        if(s->braking) {
            const bool timeout=(uint32_t)(now-s->brake_start_ms)>=c->brake_timeout_ms;
            if(fabsf(omega)<=c->brake_stop_rad_s || omega*s->brake_sign<=0 || timeout) {
                s->braking=false;s->torque_nm=0;s->brake_timed_out=timeout;
            } else {
                s->torque_nm=approach(s->torque_nm,-s->brake_sign*c->brake_torque_nm,c->torque_rise_nm_s*dt);
                o->high[2]=s->torque_nm;
            }
        }
    }
    (void)now;s->previous_mode=i->motion_mode;o->braking=s->braking;o->brake_timed_out=s->brake_timed_out;
    return true;
}
