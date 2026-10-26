#include "force_calibration_protocol.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#if CHASSIS_FORCE_CALIBRATION_ENABLE
#include "dji_force_calibration.h"
#include "board/board_force_calibration.h"
#include "chassis_power_config.h"
#include "chassis_energy_config.h"
#ifndef ATYPE_BENCH_MODE
#define ATYPE_BENCH_MODE 0
#endif
#endif
static char response[512];
static size_t response_size;
static uint32_t connection;
#if CHASSIS_FORCE_CALIBRATION_ENABLE
static uint32_t waiting_id;
#endif
static bool waiting,adapter_reply;
static void error(uint32_t id,const char *message)
{
    int n=snprintf(response,sizeof response,"@FC1,%lu,ERR,%lu,%s\r\n",(unsigned long)id,(unsigned long)connection,message);
    response_size=n>0 && (size_t)n<sizeof response ? (size_t)n : 0;
}
static bool integer(const char *s,bool sign,int64_t *out)
{
    bool negative=sign && *s=='-';if(negative) ++s;
    if(!*s) return false;
    uint64_t n=0;
    for(;*s;s++) {if(*s<'0' || *s>'9') return false;n=n*10+(unsigned)(*s-'0');if(n>UINT32_MAX) return false;}
    if(sign && n>(negative ? 2147483648ULL : INT32_MAX)) return false;
    *out=negative ? -(int64_t)n : (int64_t)n;return true;
}
#if CHASSIS_FORCE_CALIBRATION_ENABLE
static long milli(float n)
{
    if(!isfinite(n)) return LONG_MIN;
    double value=(double)n*1000;
    return value>INT32_MAX ? INT32_MAX : value<INT32_MIN ? INT32_MIN : (long)value;
}
static void ok(uint32_t id,const FcStatus *s)
{
    unsigned w=s->pulse.wheel<4 ? s->pulse.wheel : 0;
    int n=snprintf(response,sizeof response,
        "@FC1,%lu,OK,%lu,%lu,%lu,%lu,%lu,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%ld,%u,%lu,%ld,%ld\r\n",
        (unsigned long)id,(unsigned long)connection,(unsigned long)s->session,(unsigned long)s->capture_id,
        (unsigned long)s->capture_session,(unsigned long)s->capture_generation,(unsigned)s->state,(unsigned)s->reason,
        s->records,s->complete,s->zero_submitted,s->requested,s->final[0],s->final[1],s->final[2],s->final[3],
        s->raw_rpm[w],milli(s->used_rpm[w]),s->temperature[w],(unsigned long)s->feedback_age_us[w],milli(s->load_w),milli(s->budget_w));
    response_size=n>0 && (size_t)n<sizeof response ? (size_t)n : 0;
}
static uint16_t crc16(const char *s,size_t size)
{
    uint16_t crc=0xffff;
    for(size_t i=0;i<size;i++) {crc^=(uint8_t)s[i];for(unsigned bit=0;bit<8;bit++) crc=(crc&1) ? (uint16_t)((crc>>1)^0xa001) : (uint16_t)(crc>>1);}
    return crc;
}
#endif
void ForceCalibrationProtocolInit(void) {response_size=0;waiting=false;adapter_reply=false;connection=0;}
void ForceCalibrationProtocolCancel(uint32_t generation,FcReason reason)
{
    connection=generation;waiting=false;response_size=0;adapter_reply=false;
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    DJIForceCalibrationCancel(reason);
    FcCommandResult r;if(DJIForceCalibrationReply(&r)) DJIForceCalibrationReplyAccepted(r.request_id);
#else
    (void)reason;
#endif
}
void ForceCalibrationProtocolSubmit(const char *line,uint32_t generation)
{
    if(!line || strncmp(line,"@FC1,",5)) return;
    connection=generation;
    size_t length=strlen(line);if(length>=512) {ForceCalibrationProtocolCancel(generation,FC_REASON_PROTOCOL);return;}
    char copy[512];memcpy(copy,line,length+1);char *fields[13],*p=copy;unsigned count=0;
    while(count<13) {fields[count++]=p;char *comma=strchr(p,',');if(!comma) break;*comma=0;p=comma+1;}
    int64_t id;
    if(count<3 || !integer(fields[1],false,&id)) {ForceCalibrationProtocolCancel(generation,FC_REASON_PROTOCOL);return;}
    const bool stop=!strcmp(fields[2],"STOP");
    if((response_size || waiting) && !stop) return; /* Keep the original reply stable until accepted. */
    if(!strcmp(fields[2],"CAP") && count==3) {
#if CHASSIS_FORCE_CALIBRATION_ENABLE
        FcLimits l=BoardForceCalibrationLimits();
        int n=snprintf(response,sizeof response,
            "@FC1,%lu,CAP,%lu,1,1,48,1280,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,0,1,4,2,3,850,900\r\n",
            (unsigned long)id,(unsigned long)generation,l.max_current,l.max_on_ms,l.max_rpm,l.stop_temperature_c,
            l.stationary_rpm,l.stationary_ms,l.heartbeat_ms,l.pre_ms,l.tail_ms,
            CHASSIS_POWER_SOURCE_MODE,CHASSIS_POWER_CONTROL_STAGE,ATYPE_BENCH_MODE);
#else
        int n=snprintf(response,sizeof response,"@FC1,%lu,CAP,%lu,0,1,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,4,2,3,850,900\r\n",
            (unsigned long)id,(unsigned long)generation);
#endif
        response_size=n>0 && (size_t)n<sizeof response ? (size_t)n : 0;return;
    }
    int64_t values[9]={0};bool format=count>=4 && count<=12;
    for(unsigned k=3;format && k<count;k++) format=integer(fields[k],k==6 && !strcmp(fields[2],"START"),&values[k-3]);
    unsigned required=0;
    const char *op=fields[2];
    if(!strcmp(op,"STATUS") || !strcmp(op,"ARM")) required=4;
    else if(!strcmp(op,"PING") || stop || !strcmp(op,"CLEAR") || !strcmp(op,"META")) required=5;
    else if(!strcmp(op,"START")) required=10;
    else if(!strcmp(op,"READ")) required=7;
    if(!format || count!=required) {
        if(stop && (response_size || waiting)) return;
        error((uint32_t)id,"format");
#if CHASSIS_FORCE_CALIBRATION_ENABLE
        DJIForceCalibrationCancel(FC_REASON_PROTOCOL);
#endif
        return;
    }
    if((uint32_t)values[0]!=generation) {
        if(stop && (response_size || waiting)) return;
        error((uint32_t)id,"generation");return;
    }
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    FcStatus s;DJIForceCalibrationGetStatus(&s);
    if(stop) {
        if(values[1] && (uint32_t)values[1]!=s.session && (response_size || waiting)) return;
    }
    if(!strcmp(op,"STATUS")) {ok((uint32_t)id,&s);return;}
    if(!strcmp(op,"META")) {
        FcCaptureMeta meta;
        if(!DJIForceCalibrationMetadata((uint32_t)values[1],&meta)) {error((uint32_t)id,"capture");return;}
        const FcLimits *l=&meta.limits;const FcPulse *p=&meta.pulse;
        int n=snprintf(response,sizeof response,
            "@FC1,%lu,META,%lu,%lu,%lu,%lu,%u,%d,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\r\n",
            (unsigned long)id,(unsigned long)generation,(unsigned long)meta.capture_id,(unsigned long)meta.session,
            (unsigned long)meta.generation,p->wheel,p->current,p->rise_ms,p->hold_ms,p->fall_ms,
            l->max_current,l->max_on_ms,l->max_rpm,l->stop_temperature_c,l->stationary_rpm,l->stationary_ms,
            l->heartbeat_ms,l->pre_ms,l->tail_ms,meta.speed_lpf_pm,meta.current_lpf_pm);
        response_size=n>0 && (size_t)n<sizeof response ? (size_t)n : 0;return;
    }
    if(!strcmp(op,"READ")) {
        if(!DJIForceCalibrationCaptureFrozen()) {error((uint32_t)id,"not_frozen");return;}
        if(!s.zero_submitted) {error((uint32_t)id,"not_zero");return;}
        if((uint32_t)values[1]!=s.capture_id) {error((uint32_t)id,"capture");return;}
        uint8_t bytes[96];
        if(values[2]>UINT16_MAX || values[3]<1 || values[3]>2 ||
           !DJIForceCalibrationRead(s.capture_id,(uint16_t)values[2],(uint8_t)values[3],bytes,sizeof bytes)) {error((uint32_t)id,"index");return;}
        int n=snprintf(response,sizeof response,"@FC1,%lu,DATA,%lu,%lu,%lu,%u,%u,%u,",
            (unsigned long)id,(unsigned long)generation,(unsigned long)s.capture_session,(unsigned long)s.capture_id,
            (unsigned)values[2],(unsigned)values[3],s.records);
        const char hex[]="0123456789abcdef";
        for(unsigned k=0;k<(unsigned)values[3]*48;k++) {response[n++]=hex[bytes[k]>>4];response[n++]=hex[bytes[k]&15];}
        const uint16_t crc=crc16(response,(size_t)n);
        n+=snprintf(response+n,sizeof response-(size_t)n,",%04x\r\n",crc);response_size=(size_t)n;return;
    }
    FcRequest r={.request_id=(uint32_t)id,.generation=generation,.session=(uint32_t)values[1]};
    if(!strcmp(op,"ARM")) r.op=FC_OP_ARM;
    else if(!strcmp(op,"START")) {
        FcLimits l=BoardForceCalibrationLimits();
        if(values[2]>=4 || !values[3] || values[3]<-l.max_current || values[3]>l.max_current ||
           values[4]<1 || values[5]<1 || values[6]<1 || values[4]+values[5]+values[6]>l.max_on_ms) {error(r.request_id,"range");return;}
        r.op=FC_OP_START;r.pulse=(FcPulse){.wheel=(uint8_t)values[2],.current=(int16_t)values[3],
            .rise_ms=(uint16_t)values[4],.hold_ms=(uint16_t)values[5],.fall_ms=(uint16_t)values[6]};
    } else if(!strcmp(op,"PING")) r.op=FC_OP_PING;
    else if(stop) r.op=FC_OP_STOP;
    else {r.op=FC_OP_CLEAR;r.capture_id=(uint32_t)values[1];}
    if(r.op!=FC_OP_ARM && r.op!=FC_OP_CLEAR && r.session!=s.session && !(stop && !r.session)) {error(r.request_id,"session");return;}
    if(!DJIForceCalibrationSubmit(&r)) {
        if(stop && (response_size || waiting)) return;
        error(r.request_id,"busy");return;
    }
    if(stop) {response_size=0;adapter_reply=false;}
    waiting=true;waiting_id=r.request_id;
#else
    error((uint32_t)id,"disabled");
#endif
}
size_t ForceCalibrationProtocolReply(char *buffer,size_t capacity)
{
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    if(waiting && !response_size) {
        FcCommandResult r;
        if(DJIForceCalibrationReply(&r) && r.request_id==waiting_id) {
            const char *errors[]={"","busy","generation","session","guard","range","capture","ownership"};
            if(r.error) error(r.request_id,errors[r.error]);else ok(r.request_id,&r.status);
            adapter_reply=true;
        }
    }
#endif
    if(!buffer || capacity<=response_size || !response_size) return 0;
    memcpy(buffer,response,response_size+1);return response_size;
}
void ForceCalibrationProtocolReplySent(void)
{
#if CHASSIS_FORCE_CALIBRATION_ENABLE
    if(adapter_reply) DJIForceCalibrationReplyAccepted(waiting_id);
#endif
    response_size=0;adapter_reply=false;waiting=false;
}
