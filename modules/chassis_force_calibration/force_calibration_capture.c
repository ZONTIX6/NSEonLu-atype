#include "force_calibration_capture.h"
#include <string.h>
#include <math.h>
_Static_assert(sizeof(float)==4,"capture requires float32");
_Static_assert(FC_RECORD_CAPACITY*FC_RECORD_BYTES+sizeof(FcCapture)+sizeof(FcEngine)<=65536,"capture RAM budget");
static void u16(uint8_t *b,uint16_t n) {b[0]=(uint8_t)n;b[1]=(uint8_t)(n>>8);}
static void u32(uint8_t *b,uint32_t n) {u16(b,(uint16_t)n);u16(b+2,(uint16_t)(n>>16));}
bool FcEncodeRecord(const FcRecord *r,uint8_t b[48])
{
    if(!r || !b || r->type<FC_RX || r->type>FC_MARKER || r->wheel>=4) return false;
    memset(b,0,48);b[0]=r->type;b[1]=r->wheel;b[2]=r->state;b[3]=r->flags;
    u32(b+4,r->sequence);u32(b+8,r->time_us);u32(b+12,(uint32_t)r->feedback_time_us);
    u16(b+16,(uint16_t)r->raw_rpm);u16(b+18,(uint16_t)r->raw_current);
    uint32_t bits;memcpy(&bits,&r->used_rpm,4);u32(b+20,bits);u16(b+24,(uint16_t)r->requested);
    for(unsigned w=0;w<4;w++) u16(b+26+2*w,(uint16_t)r->final[w]);
    b[34]=r->temperature;b[35]=r->enabled_mask;u32(b+36,r->cycle_us);
    u16(b+40,r->prepare_us);u16(b+42,r->submit_us);u16(b+44,r->reason);
    b[46]=r->attempted;b[47]=r->successful;return true;
}
void FcCaptureInit(FcCapture *c,uint8_t *storage,uint16_t capacity)
{
    memset(c,0,sizeof *c);c->storage=storage;c->capacity=capacity;
}
void FcCaptureBegin(FcCapture *c,const FcCaptureMeta *meta)
{
    uint8_t *storage=c->storage;uint16_t capacity=c->capacity;
    FcCaptureInit(c,storage,capacity);c->meta=*meta;c->quality=true;
}
static void interval(uint32_t dt,uint32_t *min,uint32_t *max)
{
    if(!*min || dt<*min) *min=dt;
    if(dt>*max) *max=dt;
}
bool FcCaptureAppend(FcCapture *c,const FcRecord *r)
{
    if(!c || !r || c->frozen) return false;
    if(!c->storage || c->count>=c->capacity) {++c->dropped;c->quality=false;return false;}
    uint8_t bytes[48];if(!FcEncodeRecord(r,bytes)) {++c->dropped;c->quality=false;return false;}
    if(r->flags || !isfinite(r->used_rpm)) c->quality=false;
    if(r->type==FC_RX) {
        if(c->has_rx) {
            const uint32_t seq=r->sequence-c->last_rx_seq,dt=r->time_us-c->last_rx_us;
            if(!seq) {++c->rx_duplicates;c->quality=false;}
            else if(seq!=1) {c->rx_gaps+=seq<0x80000000U ? seq-1 : 1;c->quality=false;}
            if(!dt || dt>FC_MAX_FEEDBACK_US) c->quality=false;
            interval(dt,&c->rx_min_us,&c->rx_max_us);
        }
        c->last_rx_us=r->time_us;c->last_rx_seq=r->sequence;c->has_rx=true;
    } else if(r->type==FC_CONTROL) {
        if(c->has_control) interval(r->time_us-c->last_control_us,&c->control_min_us,&c->control_max_us);
        if(r->attempted!=r->successful || r->cycle_us>FC_MAX_CYCLE_US) c->quality=false;
        c->last_control_us=r->time_us;c->has_control=true;
    }
    memcpy(c->storage+(size_t)c->count*48,bytes,48);++c->count;return true;
}
void FcCaptureFreeze(FcCapture *c,FcReason reason,bool complete)
{
    if(c->frozen) return;
    c->reason=reason;c->frozen=true;c->complete=complete && c->quality && !c->dropped;
}
bool FcCaptureRead(const FcCapture *c,uint16_t first,uint8_t count,uint8_t *bytes,size_t capacity)
{
    if(!c || !bytes || !c->frozen || !count || count>2 || (uint32_t)first+count>c->count || capacity<(size_t)count*48) return false;
    memcpy(bytes,c->storage+(size_t)first*48,(size_t)count*48);return true;
}
