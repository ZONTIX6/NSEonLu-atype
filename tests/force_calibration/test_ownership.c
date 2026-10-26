#include "runtime_tuning.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned enters,preempt_at,applied;
static bool acquired;
void FcTestEnterCritical(void)
{
    if(preempt_at && ++enters==preempt_at) {
        preempt_at=0;acquired=RuntimeTuningBeginCalibration();
    }
}
static bool apply(const float *values) {(void)values;++applied;return true;}
int main(void)
{
    float values[19]={0};char line[512],reply[512];
    RuntimeTuningPersistenceInit(NULL);RuntimeTuningRegister(TUNING_POWER,values,3);
    strcpy(line,"@T1,1,SET,power,1");for(unsigned k=0;k<19;k++) strcat(line,",00000000");
    enters=0;preempt_at=2;RuntimeTuningSubmit(line);
    assert(acquired && RuntimeTuningTrialActive());
    RuntimeTuningProcess(TUNING_POWER,apply);assert(applied==0);
    assert(RuntimeTuningReply(reply,sizeof reply) && strstr(reply,",busy"));
    RuntimeTuningEndCalibration();RuntimeTuningCancel();
    enters=0;preempt_at=2;acquired=false;RuntimeTuningSubmit("@T1,2,SAVE,power,1");
    assert(acquired && RuntimeTuningTrialActive());RuntimeTuningEndCalibration();
    TuningStoredImage image;assert(!RuntimeTuningPendingImage(&image));
    assert(RuntimeTuningReply(reply,sizeof reply) && strstr(reply,",busy"));
    puts("ownership: preempted SET/SAVE cannot cross calibration owner");return 0;
}
