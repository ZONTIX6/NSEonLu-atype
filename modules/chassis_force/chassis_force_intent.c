#include "chassis_force_intent.h"
#include <math.h>
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24,"force-v1 requires IEEE754 binary32");
static void put32(uint8_t *p,uint32_t x) {for(unsigned k=0;k<4;++k) p[k]=(uint8_t)(x>>(8*k));}
static uint32_t get32(const uint8_t *p) {return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void putfloat(uint8_t *p,float x) {uint32_t bits;memcpy(&bits,&x,4);put32(p,bits);}
static float getfloat(const uint8_t *p) {uint32_t bits=get32(p);float x;memcpy(&x,&bits,4);return x;}
bool ChassisForceIntentValid(const ChassisForceIntent_s *i)
{
    return i && i->motion_mode<=CHASSIS_FORCE_SPIN && i->source_id<=CHASSIS_FORCE_SOURCE_VTX &&
        i->input_permitted<=1 && i->safety_reset<=1 && i->aux_stopped<=1 && i->bench_permitted<=1 &&
        (i->spin_direction==1 || i->spin_direction==-1) && isfinite(i->input_x) && isfinite(i->input_y) &&
        fabsf(i->input_x)<=1 && fabsf(i->input_y)<=1 && isfinite(i->force_scale) &&
        i->force_scale>=0 && i->force_scale<=1 && isfinite(i->relative_yaw_rad) &&
        fabsf(i->relative_yaw_rad)<=3.141593f && i->source_age_ms<=10 && i->yaw_age_ms<=10;
}
bool ChassisForceIntentEncode(const ChassisForceIntent_s *i,uint8_t b[44])
{
    if(!b || !ChassisForceIntentValid(i)) return false;
    memset(b,0,44);b[0]=1;b[2]=1;b[3]=i->motion_mode;put32(b+4,i->command_seq);
    putfloat(b+8,i->input_x);putfloat(b+12,i->input_y);putfloat(b+16,i->force_scale);putfloat(b+20,i->relative_yaw_rad);
    put32(b+24,i->source_age_ms);put32(b+28,i->aux_age_ms);
    b[32]=i->input_permitted;b[33]=i->safety_reset;b[34]=i->aux_stopped;b[35]=i->bench_permitted;
    b[36]=(uint8_t)i->spin_direction;b[37]=i->source_id;put32(b+40,i->yaw_age_ms);return true;
}
bool ChassisForceIntentDecode(const uint8_t *b,size_t length,ChassisForceIntent_s *i)
{
    if(!i) return false;
    memset(i,0,sizeof *i);
    if(!b || length!=44 || b[0]!=1 || b[1]!=0 || b[2]!=1 || b[38]!=0 || b[39]!=0) return false;
    *i=(ChassisForceIntent_s){.motion_mode=b[3],.command_seq=get32(b+4),
        .input_x=getfloat(b+8),.input_y=getfloat(b+12),.force_scale=getfloat(b+16),.relative_yaw_rad=getfloat(b+20),
        .source_age_ms=get32(b+24),.aux_age_ms=get32(b+28),.input_permitted=b[32],.safety_reset=b[33],
        .aux_stopped=b[34],.bench_permitted=b[35],.spin_direction=b[36]==255 ? -1 : (int8_t)b[36],
        .source_id=b[37],.yaw_age_ms=get32(b+40)};
    return ChassisForceIntentValid(i);
}
bool ChassisForceIntentAccept(ChassisForceIntentReceiver_s *r,const ChassisForceIntent_s *i,uint32_t now)
{
    if(!r || !ChassisForceIntentValid(i)) return false;
    const uint32_t advance=i->command_seq-r->intent.command_seq;
    if(r->valid && (advance==0 || advance>=UINT32_C(0x80000000))) return false;
    r->intent=*i;r->received_ms=now;r->valid=true;return true;
}
bool ChassisForceIntentFresh(const ChassisForceIntentReceiver_s *r,uint32_t now,uint32_t margin)
{
    if(!r || !r->valid || !ChassisForceIntentValid(&r->intent) || margin>10) return false;
    const uint32_t elapsed=now-r->received_ms;
    return elapsed<=10-margin && r->intent.source_age_ms<=10-margin-elapsed &&
        r->intent.yaw_age_ms<=10-margin-elapsed;
}
static void normalize(ChassisForceIntent_s *i)
{
    const float length=hypotf(i->input_x,i->input_y);
    if(length>1) {i->input_x/=length;i->input_y/=length;}
}
void ChassisForceIntentStick(float forward,float left,float full_scale,ChassisForceIntent_s *i)
{
    if(!isfinite(full_scale) || full_scale<=0) {i->input_x=NAN;i->input_y=NAN;return;}
    i->input_x=forward/full_scale;i->input_y=left/full_scale;normalize(i);
}
void ChassisForceIntentKeys(bool w,bool s,bool a,bool d,ChassisForceIntent_s *i)
{
    i->input_x=(float)w-(float)s;i->input_y=(float)a-(float)d;normalize(i);
}
