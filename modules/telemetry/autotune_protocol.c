#include "autotune_protocol.h"
#include "runtime_tuning.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PIDAutotune engine,snapshot;
static uint32_t next_session;
static bool previous_drive_owned;
static PIDAutotuneFeedback last_feedback;
static AutotuneConfiguration configurations[4];
static bool configuration_set[4];
static uint32_t reply_elapsed;
static struct {
    bool queued,ready,cancel;
    unsigned generation;
    uint32_t id,session,revision;
    enum {BEGIN,TRIAL,STATUS,END,CAP,CONFIG} operation;
    unsigned version,wheel;
    PIDAutotuneTrial test;
    const char *error;
} mailbox;
void AutotuneSetConfiguration(unsigned wheel,const AutotuneConfiguration *config)
{
    if(wheel>=4) return;
    taskENTER_CRITICAL();
    if(engine.active && engine.v2 && configuration_set[wheel] &&
        memcmp(&configurations[wheel],config,sizeof *config)) PIDAutotuneStop(&engine,PID_AUTO_CONFIG);
    configurations[wheel]=*config;configuration_set[wheel]=true;
    taskEXIT_CRITICAL();
}
static bool Unsigned(const char *s,uint32_t *value)
{
    if (!*s) return false;
    for(const char *p=s;*p;++p) if (*p<'0' || *p>'9') return false;
    char *end; unsigned long long v=strtoull(s,&end,10);
    if (*end || v>UINT32_MAX) return false;
    *value=(uint32_t)v;return true;
}
static bool Float(const char *s,float *value)
{
    if(strlen(s)!=8) return false;
    for(unsigned i=0;i<8;++i) if (!((s[i]>='0' && s[i]<='9') || (s[i]>='a' && s[i]<='f') || (s[i]>='A' && s[i]<='F'))) return false;
    uint32_t bits=(uint32_t)strtoul(s,NULL,16);memcpy(value,&bits,4);return isfinite(*value);
}
void AutotuneSubmit(const char *line)
{
    if((strncmp(line,"@A1,",4) && strncmp(line,"@A2,",4)) || strlen(line)>=256) return;
    taskENTER_CRITICAL();bool busy=mailbox.queued || mailbox.ready;taskEXIT_CRITICAL();
    if(busy) return;
    char copy[256];strcpy(copy,line); char *cells[13],*p=copy;unsigned n=0;
    while(n<13) {cells[n++]=p;char *comma=strchr(p,',');if(!comma) break;*comma=0;p=comma+1;}
    uint32_t id;
    if(n<3 || !Unsigned(cells[1],&id)) return;
    mailbox.id=id;mailbox.error=NULL;mailbox.version=line[2]=='2'?2:1;
    uint32_t session=0,wheel=0,rpm=0,hold=0,cap=0;
    if(n==3 && mailbox.version==2 && !strcmp(cells[2],"CAP")) mailbox.operation=CAP;
    else if(n==4 && mailbox.version==2 && !strcmp(cells[2],"CONFIG") && Unsigned(cells[3],&wheel) && wheel<4)
        {mailbox.operation=CONFIG;mailbox.wheel=wheel;}
    else if(n==4 && !strcmp(cells[2],"BEGIN") && Unsigned(cells[3],&mailbox.revision)) mailbox.operation=BEGIN;
    else if(n==4 && (!strcmp(cells[2],"STATUS") || !strcmp(cells[2],"END")) && Unsigned(cells[3],&session))
        {mailbox.operation=!strcmp(cells[2],"END") ? END : STATUS;mailbox.session=session;}
    else if(n==11 && !strcmp(cells[2],"TRIAL") && Unsigned(cells[3],&session) && Unsigned(cells[4],&wheel) &&
        Unsigned(cells[5],&rpm) && Unsigned(cells[6],&hold) && Unsigned(cells[7],&cap) &&
        Float(cells[8],&mailbox.test.kp) && Float(cells[9],&mailbox.test.ki) && Float(cells[10],&mailbox.test.kd)) {
        mailbox.operation=TRIAL;mailbox.session=session;
        mailbox.test.wheel=wheel;mailbox.test.rpm=(float)rpm;mailbox.test.hold_ms=hold;mailbox.test.current_limit=(float)cap;
    } else mailbox.error="format";
    taskENTER_CRITICAL();
    mailbox.ready=mailbox.error!=NULL;mailbox.queued=!mailbox.ready;
    taskEXIT_CRITICAL();
}
void AutotuneMotorStep(const PIDAutotuneFeedback *f,AutotuneDrive *drive)
{
    taskENTER_CRITICAL();
    bool cancel=mailbox.cancel,queued=mailbox.queued;
    mailbox.cancel=false;
    PIDAutotuneTick(&engine,f);
    last_feedback=*f;
    if(cancel && engine.active) {PIDAutotuneStop(&engine,PID_AUTO_STOPPED);RuntimeTuningEndTrial();}
    bool ending=previous_drive_owned && !engine.active;
    if(!engine.active) RuntimeTuningEndTrial();
    if(queued && !cancel) {
        mailbox.error=NULL;
        if(mailbox.operation==CAP) { /* Read-only capability query, no heartbeat. */ }
        else if(mailbox.operation==CONFIG) {if(!configuration_set[mailbox.wheel]) mailbox.error="unavailable";}
        else if(mailbox.operation==BEGIN) {
            if(engine.active) mailbox.error="busy";
            else if(ending) mailbox.error="guard";
            else if(!f->permitted || !f->feedback_ok || !f->can_ok) mailbox.error="guard";
            else if(!RuntimeTuningBeginTrial(mailbox.revision)) mailbox.error="conflict";
            else {
                if(++next_session==0) ++next_session;
                bool begun=mailbox.version==2 ? PIDAutotuneBeginV2(&engine,next_session,f) : PIDAutotuneBegin(&engine,next_session,f);
                if(!begun) {RuntimeTuningEndTrial();mailbox.error="guard";}
            }
        } else if(mailbox.session!=engine.session || !engine.session || (mailbox.version==2)!=engine.v2) mailbox.error="session";
        else if(mailbox.operation==END) {PIDAutotuneStop(&engine,PID_AUTO_STOPPED);RuntimeTuningEndTrial();}
        else if(mailbox.operation==STATUS) {if(engine.active) PIDAutotuneKeepAlive(&engine,f->now_ms);}
        else if(!engine.active) mailbox.error="expired";
        else {
            PIDAutotuneKeepAlive(&engine,f->now_ms);
            if(mailbox.operation==TRIAL) {
                if(engine.v2 && (mailbox.test.current_limit<500 || mailbox.test.current_limit>5000)) mailbox.error="config";
                else {
                    if(engine.v2 && mailbox.test.wheel<4 && configuration_set[mailbox.test.wheel])
                        mailbox.test.current_limit=fminf(mailbox.test.current_limit,configurations[mailbox.test.wheel].output_limit);
                    if(!PIDAutotuneStart(&engine,&mailbox.test,f)) mailbox.error="config";
                }
            }
        }
        mailbox.queued=false;mailbox.ready=true;
    }
    snapshot=engine;
    reply_elapsed=engine.session ? f->now_ms-engine.begin_ms : 0;
    *drive=(AutotuneDrive){.owned=engine.active,.running=engine.state==PID_AUTO_RUNNING,
        .trial=engine.trial,.test=engine.test,.target_rpm=PIDAutotuneTarget(&engine,f->now_ms)};
    previous_drive_owned=engine.active;
    taskEXIT_CRITICAL();
}
void AutotuneObserve(uint32_t now,float rpm,float raw,float final,bool limited,bool transmitted)
{
    if(engine.active && !transmitted) PIDAutotuneStop(&engine,PID_AUTO_CAN);
    last_feedback.now_ms=now;
    PIDAutotuneObserveV2(&engine,&last_feedback,rpm,raw,final,limited);
    taskENTER_CRITICAL();snapshot=engine;taskEXIT_CRITICAL();
}
size_t AutotuneReply(char *buffer,size_t capacity)
{
    PIDAutotune a;uint32_t id,elapsed;const char *error;unsigned version,operation,wheel;AutotuneConfiguration config;
    taskENTER_CRITICAL();
    if(!mailbox.ready) {taskEXIT_CRITICAL();return 0;}
    a=snapshot;id=mailbox.id;error=mailbox.error;version=mailbox.version;operation=mailbox.operation;
    elapsed=reply_elapsed;wheel=mailbox.wheel;config=configurations[wheel<4?wheel:0];taskEXIT_CRITICAL();
    static const char *const states[]={"idle","ready","running","done","aborted"};
    static const char *const reasons[]={"none","stopped","timeout","guard","feedback","overspeed","limited","no_response","config","can","quality","stationary"};
    int n;
    if(error) n=snprintf(buffer,capacity,"@A%u,%lu,ERR,%s\r\n",version,(unsigned long)id,error);
    else if(operation==CAP) n=snprintf(buffer,capacity,"@A2,%lu,CAP,2,31,180000,1000,20\r\n",(unsigned long)id);
    else if(operation==CONFIG) {
        float values[]={config.coef_a,config.coef_b,config.output_rc,config.derivative_rc,config.speed_smooth,config.current_smooth};
        uint32_t bits[6];memcpy(bits,values,sizeof bits);
        n=snprintf(buffer,capacity,"@A2,%lu,CONFIG,%u,%lu,%08lx,%08lx,%08lx,%08lx,%08lx,%08lx,%lu\r\n",
            (unsigned long)id,wheel,(unsigned long)config.improve,(unsigned long)bits[0],(unsigned long)bits[1],
            (unsigned long)bits[2],(unsigned long)bits[3],(unsigned long)bits[4],(unsigned long)bits[5],(unsigned long)config.motor_flags);
    } else if(version==2) {
        uint32_t kp,ki,kd;memcpy(&kp,&a.test.kp,4);memcpy(&ki,&a.test.ki,4);memcpy(&kd,&a.test.kd,4);
        n=snprintf(buffer,capacity,"@A2,%lu,OK,%lu,%s,%u,%lu,%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%08lx,%08lx,%08lx,%u,%lu\r\n",
            (unsigned long)id,(unsigned long)a.session,states[a.state],a.test.wheel,(unsigned long)a.trial,reasons[a.reason],
            (unsigned long)a.samples,(unsigned long)llround(a.rmse*1000.0),(unsigned long)llround(a.overshoot*1000.0),
            (unsigned long)a.settle_ms,(unsigned long)llround(a.steady*1000.0),
            (unsigned long)(a.jitter>4294967.0f?UINT32_MAX:(uint32_t)llround(a.jitter*1000.0)),(unsigned long)a.limited_pm,
            (unsigned long)a.window_ms,(unsigned long)a.coverage_ms,(unsigned long)a.feedback_count,
            (unsigned long)a.max_feedback_ms,(unsigned long)a.max_stat_ms,(unsigned long)a.stat_limit_ms,
            (unsigned long)a.test.current_limit,(unsigned long)kp,(unsigned long)ki,(unsigned long)kd,a.phase,(unsigned long)elapsed);
    } else n=
        snprintf(buffer,capacity,"@A1,%lu,OK,%lu,%s,%u,%lu,%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu\r\n",
        (unsigned long)id,(unsigned long)a.session,states[a.state],a.test.wheel,(unsigned long)a.trial,reasons[a.reason],
        (unsigned long)a.samples,(unsigned long)lroundf(a.rmse*1000),(unsigned long)lroundf(a.overshoot*1000),
        (unsigned long)a.settle_ms,(unsigned long)lroundf(a.steady*1000),(unsigned long)lroundf(a.jitter*1000),(unsigned long)a.limited_pm);
    return n>0 && (size_t)n<capacity ? (size_t)n : 0;
}
void AutotuneReplySent(void) {taskENTER_CRITICAL();mailbox.ready=false;taskEXIT_CRITICAL();}
void AutotuneCancel(void)
{
    taskENTER_CRITICAL();++mailbox.generation;mailbox.cancel=true;mailbox.queued=false;mailbox.ready=false;taskEXIT_CRITICAL();
}
