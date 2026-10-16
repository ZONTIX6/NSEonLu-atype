#include "runtime_tuning.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned applied;
static bool accept(const float *v) { ++applied; return v[0] != 2; }
static void make_set(char *set, uint32_t id, uint32_t revision, const float *pid)
{
    snprintf(set, 512, "@T1,%lu,SET,pid,%lu", (unsigned long)id, (unsigned long)revision);
    for (unsigned i = 0; i < 24; ++i) {
        uint32_t bits; memcpy(&bits, &pid[i], 4);
        char cell[16]; snprintf(cell, sizeof cell, ",%08lx", (unsigned long)bits);
        strcat(set, cell);
    }
}
static void reply_contains(const char *part)
{
    char reply[1024];
    assert(RuntimeTuningReply(reply, sizeof reply) > 0);
    assert(strstr(reply, part));
    RuntimeTuningReplySent();
}
int main(void)
{
    float pid[24];
    for (unsigned i = 0; i < 4; ++i) {
        const float wheel[] = {.75f, 0, 0, 15000, 3000, 0};
        memcpy(&pid[i * 6], wheel, sizeof wheel);
    }
    RuntimeTuningRegister(TUNING_PID, pid, 0);
    RuntimeTuningSubmit("@T1,1,GET,pid");
    reply_contains("@T1,1,OK,pid,1,0,3f400000");
    char set[512];
    make_set(set, 2, 1, pid);
    RuntimeTuningSubmit(set);
    char reply[1024];
    assert(RuntimeTuningReply(reply, sizeof reply) == 0);
    RuntimeTuningProcess(TUNING_SPIN, accept);
    assert(applied == 0);
    RuntimeTuningProcess(TUNING_PID, accept);
    assert(applied == 1);
    reply_contains("@T1,2,OK,pid,2,0,");
    RuntimeTuningSubmit(set);
    reply_contains("@T1,2,ERR,pid,conflict");
    RuntimeTuningSubmit("@T1,3,SET,pid,2,7fc00000");
    reply_contains("ERR,pid,count");
    RuntimeTuningSubmit("@T1,4,GET,missing");
    reply_contains("ERR,missing,group");
    RuntimeTuningSubmit("@T1,5,GET,spin");
    reply_contains("ERR,spin,unavailable");
    /* Same valid payload, new revision; cancellation must prevent application. */
    make_set(set, 6, 2, pid);
    RuntimeTuningSubmit(set);
    RuntimeTuningCancel();
    RuntimeTuningProcess(TUNING_PID, accept);
    assert(applied == 1 && RuntimeTuningReply(reply, sizeof reply) == 0);
    pid[0] = 2;
    make_set(set, 7, 2, pid);
    RuntimeTuningSubmit(set);
    RuntimeTuningProcess(TUNING_PID, accept);
    reply_contains("@T1,7,ERR,pid,constraint");
    RuntimeTuningSubmit("@T1,8,GET,pid");
    reply_contains("@T1,8,OK,pid,2,0,3f400000");
    uint32_t nan_bits = 0x7fc00000;
    memcpy(pid, &nan_bits, sizeof nan_bits);
    make_set(set, 9, 2, pid);
    RuntimeTuningSubmit(set);
    reply_contains("@T1,9,ERR,pid,value");
    assert(applied == 2);
    puts("Runtime tuning tests passed");
}
