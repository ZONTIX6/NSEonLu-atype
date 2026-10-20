#include "autotune_protocol.h"
#include "runtime_tuning.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static PIDAutotuneFeedback f={.now_ms=100,.permitted=true,.feedback_ok=true,.can_ok=true};
static AutotuneDrive drive;
static char reply[512];
static void command(const char *line,const char *expected)
{
    AutotuneSubmit(line);AutotuneMotorStep(&f,&drive);
    assert(AutotuneReply(reply,sizeof reply)>0);
    if(!strstr(reply,expected)) {printf("unexpected: %s expected: %s\n",reply,expected);fflush(stdout);}
    assert(strstr(reply,expected));AutotuneReplySent();
}
int main(void)
{
    float values[24]={0};RuntimeTuningRegister(TUNING_PID,values,0);
    command("@A1,1,BEGIN,2","ERR,conflict");
    command("@A1,2,BEGIN,1","OK,1,ready");assert(drive.owned && RuntimeTuningTrialActive());
    command("@A1,3,TRIAL,9,0,500,500,3000,3f400000,00000000,00000000","ERR,session");
    command("@A1,4,TRIAL,1,0,500,500,3000,3f400000,00000000,00000000","OK,1,running");
    assert(drive.running && drive.target_rpm==0);
    f.now_ms=400;command("@A1,5,STATUS,1","OK,1,running");assert(drive.target_rpm==500);
    f.feedback_ok=false;AutotuneMotorStep(&f,&drive);assert(!drive.owned && !RuntimeTuningTrialActive());
    command("@A1,6,STATUS,1","OK,1,aborted,0,1,feedback");
    command("@A1,7,TRIAL,1,0,500,500,3000,3f400000,00000000,00000000","ERR,expired");
    command("@A1,8,END,1","OK,1,aborted");
    f.feedback_ok=true;command("@A1,9,BEGIN,1","OK,2,ready");
    AutotuneCancel();AutotuneMotorStep(&f,&drive);assert(!drive.owned && !RuntimeTuningTrialActive());
    command("@A1,10,STATUS,2","OK,2,aborted,0,0,stopped");
    command("@A1,11,TRIAL,2,0,500,500,3000,7fc00000,00000000,00000000","ERR,format");
    command("@A2,12,CAP","@A2,12,CAP,2,31,180000,1000,20");
    command("@A2,13,CONFIG,4","ERR,format");
    command("@A2,14,BEGIN,1","@A2,14,OK,3,ready");
    command("@A1,15,STATUS,3","ERR,session");
    command("@A2,16,TRIAL,3,0,500,500,3000,3f400000,00000000,00000000","@A2,16,OK,3,running");
    assert(strlen(reply)<512 && strstr(reply,"3f400000,00000000,00000000,0,0"));
    AutotuneSubmit("@A2,99,STATUS,3");AutotuneMotorStep(&f,&drive);
    assert(AutotuneReply(reply,8)==0 && AutotuneReply(reply,sizeof reply)>0);AutotuneReplySent();
    command("@A2,17,END,3","@A2,17,OK,3,aborted");
    command("@A2,18,TRIAL,4294967296,0,500,500,3000,3f400000,00000000,00000000","ERR,format");
    AutotuneConfiguration config={.improve=5,.coef_a=1,.coef_b=2,.speed_smooth=.85f,.current_smooth=.9f,.output_limit=2000};
    AutotuneSetConfiguration(0,&config);
    command("@A2,19,CONFIG,0","CONFIG,0,5,3f800000,40000000");
    command("@A2,20,BEGIN,1","OK,4,ready");
    command("@A2,21,TRIAL,4,0,500,500,5001,3f400000,00000000,00000000","ERR,config");
    command("@A2,22,TRIAL,4,0,500,500,3000,3f400000,00000000,00000000","OK,4,running");
    assert(drive.test.current_limit==2000);
    config.improve=6;AutotuneSetConfiguration(0,&config);AutotuneMotorStep(&f,&drive);
    assert(!drive.owned && !RuntimeTuningTrialActive());
    command("@A2,23,STATUS,4",",aborted,0,1,config,");
    puts("Autotune protocol/session/lock/cancellation tests passed");
}
