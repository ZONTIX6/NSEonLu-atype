#include "chassis_speed_capture.h"
#include "crc16.h"
#include <string.h>

_Static_assert(sizeof(((CSCCapture *)0)->raw)+sizeof(((CSCCapture *)0)->cycle)==24064U,
               "CSC1 payload storage budget");
_Static_assert(sizeof(float)==4,"CSC1 requires binary32 float");

void CSCWriteU16(uint8_t *p,uint16_t v) {p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
void CSCWriteU32(uint8_t *p,uint32_t v) {for(unsigned n=0;n<4;++n)p[n]=(uint8_t)(v>>(8*n));}
void CSCWriteU64(uint8_t *p,uint64_t v) {for(unsigned n=0;n<8;++n)p[n]=(uint8_t)(v>>(8*n));}
uint16_t CSCReadU16(const uint8_t *p) {return (uint16_t)(p[0]|(uint16_t)p[1]<<8);}
uint32_t CSCReadU32(const uint8_t *p) {uint32_t v=0;for(unsigned n=0;n<4;++n)v|=(uint32_t)p[n]<<(8*n);return v;}
uint64_t CSCReadU64(const uint8_t *p) {uint64_t v=0;for(unsigned n=0;n<8;++n)v|=(uint64_t)p[n]<<(8*n);return v;}
void CSCWriteF32(uint8_t *p,float v) {uint32_t bits;memcpy(&bits,&v,4);CSCWriteU32(p,bits);}
float CSCReadF32(const uint8_t *p) {uint32_t bits=CSCReadU32(p);float v;memcpy(&v,&bits,4);return v;}

void CSCMetadataInit(CSCMetadata *m,uint32_t id,uint32_t hz)
{
    if(!m)return;
    memset(m,0,sizeof *m);
    CSCWriteU32(m->bytes,1);CSCWriteU32(m->bytes+4,id);CSCWriteU32(m->bytes+8,hz);
    CSCWriteU16(m->bytes+12,CSC_RAW_CAPACITY);CSCWriteU16(m->bytes+14,CSC_CYCLE_CAPACITY);
    CSCWriteU16(m->bytes+16,CSC_RAW_BYTES);CSCWriteU16(m->bytes+18,CSC_CYCLE_BYTES);
    for(unsigned i=0;i<4;++i){m->bytes[20+i]=(uint8_t)i;m->bytes[24+i]=1;}
    CSCWriteU32(m->bytes+28,1);
}
bool CSCArm(CSCCapture *c,uint32_t id,const CSCMetadata *m)
{
    if(!c||!m||c->state!=CSC_IDLE)return false;
    /* Do not clear the large record arrays: counts bound all later reads. */
    c->metadata=*m;CSCWriteU32(c->metadata.bytes+4,id);
    c->capture_id=id;c->raw_count=0;c->cycle_count=0;c->drain_index=0;c->drain_kind=0;
    c->rejected_records=0;c->outside_window=0;c->transport_retries=0;c->freeze_reason=0;
    c->start_tick_ms=0;c->end_tick_ms=0;c->start_cycles=0;c->end_cycles=0;
    memset(c->end_payload,0,sizeof c->end_payload);
    c->state=CSC_ARMED;return true;
}
static bool writable(CSCCapture *c,bool present)
{
    if(!c)return false;
    if(c->state!=CSC_ARMED&&c->state!=CSC_RECORDING){
        ++c->session_outside_window;
        if(c->state==CSC_FROZEN)++c->outside_window;
        return false;
    }
    if(!present){++c->rejected_records;return false;}
    return true;
}
static void stamp(CSCCapture *c,uint32_t tick,uint32_t begin,uint32_t end)
{
    if(c->state==CSC_ARMED){c->start_tick_ms=tick;c->start_cycles=begin;c->state=CSC_RECORDING;}
    c->end_tick_ms=tick;c->end_cycles=end;
}
void CSCFreezeReason(CSCCapture *c,uint8_t reason)
{
    if(c&&(c->state==CSC_ARMED||c->state==CSC_RECORDING)){
        c->freeze_reason=reason>=CSC_FREEZE_RAW_FULL&&reason<=CSC_FREEZE_ERROR ? reason : CSC_FREEZE_ERROR;
        c->state=CSC_FROZEN;
    }
}
void CSCFreeze(CSCCapture *c) {CSCFreezeReason(c,CSC_FREEZE_MANUAL);}
bool CSCAppendRaw(CSCCapture *c,const CSCRawRecord *r)
{
    if(!writable(c,r!=NULL))return false;
    if(r->bytes[0]>=4||r->bytes[2]!=0){++c->rejected_records;return false;}
    stamp(c,CSCReadU32(r->bytes+8),CSCReadU32(r->bytes+12),CSCReadU32(r->bytes+12));
    c->raw[c->raw_count++]=*r;
    if(c->raw_count==CSC_RAW_CAPACITY)CSCFreezeReason(c,CSC_FREEZE_RAW_FULL);
    return true;
}
bool CSCAppendCycle(CSCCapture *c,const CSCCycleRecord *r)
{
    if(!writable(c,r!=NULL))return false;
    stamp(c,CSCReadU32(r->bytes),CSCReadU32(r->bytes+4),CSCReadU32(r->bytes+8));
    c->cycle[c->cycle_count++]=*r;
    if(c->cycle_count==CSC_CYCLE_CAPACITY)CSCFreezeReason(c,CSC_FREEZE_CYCLE_FULL);
    return true;
}
bool CSCBeginDrain(CSCCapture *c)
{
    if(!c)return false;
    if(c->state==CSC_DRAINING)return true;
    if(c->state!=CSC_FROZEN)return false;
    const uint32_t fields[12]={c->raw_count,c->cycle_count,c->freeze_reason,c->rejected_records,
        c->outside_window,c->transport_retries,c->start_tick_ms,c->end_tick_ms,c->start_cycles,
        c->end_cycles,CSCReadU32(c->metadata.bytes+32),1U|(c->rejected_records ? 2U:0U)};
    for(unsigned i=0;i<12;++i)CSCWriteU32(c->end_payload+4*i,fields[i]);
    c->drain_kind=CSC_KIND_METADATA;c->drain_index=0;c->state=CSC_DRAINING;return true;
}
static uint16_t payload_size(uint8_t kind)
{
    switch(kind){case CSC_KIND_METADATA:return CSC_METADATA_BYTES;case CSC_KIND_RAW:return CSC_RAW_BYTES;
    case CSC_KIND_CYCLE:return CSC_CYCLE_BYTES;case CSC_KIND_END:return CSC_END_BYTES;default:return 0;}
}
bool CSCPeekFrame(const CSCCapture *c,uint8_t *dst,uint16_t capacity,uint16_t *length)
{
    if(length)*length=0;
    if(!c||!dst||!length||c->state!=CSC_DRAINING)return false;
    const uint16_t bytes=payload_size(c->drain_kind);
    if(!bytes||capacity<bytes+20U)return false;
    const uint8_t *payload=NULL;
    switch(c->drain_kind){
    case CSC_KIND_METADATA:payload=c->metadata.bytes;break;
    case CSC_KIND_RAW:if(c->drain_index>=c->raw_count)return false;payload=c->raw[c->drain_index].bytes;break;
    case CSC_KIND_CYCLE:if(c->drain_index>=c->cycle_count)return false;payload=c->cycle[c->drain_index].bytes;break;
    case CSC_KIND_END:payload=c->end_payload;break;
    default:return false;
    }
    memcpy(dst,"CSC1",4);CSCWriteU16(dst+4,1);dst[6]=c->drain_kind;dst[7]=0;
    CSCWriteU16(dst+8,bytes);CSCWriteU32(dst+10,c->capture_id);CSCWriteU32(dst+14,c->drain_index);
    memcpy(dst+18,payload,bytes);CSCWriteU16(dst+18+bytes,crc_16(dst,(uint16_t)(18+bytes)));
    *length=(uint16_t)(bytes+20);return true;
}
void CSCFrameAccepted(CSCCapture *c)
{
    if(!c||c->state!=CSC_DRAINING)return;
    switch(c->drain_kind){
    case CSC_KIND_METADATA:c->drain_kind=CSC_KIND_RAW;c->drain_index=0;break;
    case CSC_KIND_RAW:if(++c->drain_index>=c->raw_count){c->drain_kind=CSC_KIND_CYCLE;c->drain_index=0;}break;
    case CSC_KIND_CYCLE:if(++c->drain_index>=c->cycle_count){c->drain_kind=CSC_KIND_END;c->drain_index=0;}break;
    case CSC_KIND_END:c->drain_kind=0;c->drain_index=0;break;
    default:return;
    }
    if(c->drain_kind==CSC_KIND_RAW&&!c->raw_count)c->drain_kind=CSC_KIND_CYCLE;
    if(c->drain_kind==CSC_KIND_CYCLE&&!c->cycle_count)c->drain_kind=CSC_KIND_END;
}
void CSCTransportRetry(CSCCapture *c)
{if(c){++c->session_transport_retries;if(c->state!=CSC_DRAINING)++c->transport_retries;}}
void CSCDrainComplete(CSCCapture *c,bool idle)
{if(c&&c->state==CSC_DRAINING&&!c->drain_kind&&idle)c->state=CSC_IDLE;}
bool CSCFrameIsValid(const uint8_t *p,uint16_t length)
{
    if(!p||length<20||memcmp(p,"CSC1",4)||CSCReadU16(p+4)!=1||p[7]!=0)return false;
    const uint16_t size=payload_size(p[6]);
    return size&&CSCReadU16(p+8)==size&&length==size+20U&&
        CSCReadU16(p+18+size)==crc_16(p,(uint16_t)(size+18));
}
