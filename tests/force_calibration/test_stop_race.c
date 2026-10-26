#define FC_ADAPTER_MAIN AdapterRegressionMain
#include "test_adapter.c"
static bool preempt_stop;
static bool submit_with_preemption(const FcRequest *r)
{
    if(preempt_stop && r->op==FC_OP_STOP) {preempt_stop=false;run();}
    return DJIForceCalibrationSubmit(r);
}
#define DJIForceCalibrationSubmit submit_with_preemption
#include "../../modules/telemetry/force_calibration_protocol.c"
#undef DJIForceCalibrationSubmit
int main(void)
{
    assert(AdapterRegressionMain()==0);FcCommandResult old;
    if(DJIForceCalibrationReply(&old)) DJIForceCalibrationReplyAccepted(old.request_id);
    ForceCalibrationProtocolInit();FcStatus s;DJIForceCalibrationGetStatus(&s);
    char line[128],reply[512];
    snprintf(line,sizeof line,"@FC1,100,ARM,%lu",(unsigned long)generation);
    ForceCalibrationProtocolSubmit(line,generation);
    snprintf(line,sizeof line,"@FC1,101,STOP,%lu,%lu",(unsigned long)generation,(unsigned long)s.session);
    preempt_stop=true;ForceCalibrationProtocolSubmit(line,generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,"@FC1,100,OK,"));ForceCalibrationProtocolReplySent();
    DJIForceCalibrationGetStatus(&s);
    snprintf(line,sizeof line,"@FC1,102,PING,%lu,%lu",(unsigned long)generation,(unsigned long)s.session);
    ForceCalibrationProtocolSubmit(line,generation);run();
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,"@FC1,102,OK,"));
    puts("STOP preemption: rejected stale session preserves the ARM acknowledgement");return 0;
}
