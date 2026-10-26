#define FC_ADAPTER_MAIN AdapterRegressionMain
#include "test_adapter.c"
#include "force_calibration_protocol.h"
int main(void)
{
    assert(AdapterRegressionMain()==0);
    FcCommandResult old;if(DJIForceCalibrationReply(&old)) DJIForceCalibrationReplyAccepted(old.request_id);
    ForceCalibrationProtocolInit();char reply[512],saved[512];
    ForceCalibrationProtocolSubmit("@FC1,7,START,10,3,1,4294967296,50,100,50",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply));
    assert(strcmp(reply,"@FC1,7,ERR,10,format\r\n")==0);ForceCalibrationProtocolReplySent();
    const char *invalid[]={"@FC1,8,START,10,3,4,-300,50,100,50","@FC1,8,START,10,3,1,1501,50,100,50",
        "@FC1,8,START,10,3,1,NaN,50,100,50","@FC1,8,START,10,3,1,,50,100,50",
        "@FC1,8,START,10,3,1,300,50,100,50,1","@FC1,8,ARM,4294967296"};
    for(unsigned k=0;k<sizeof invalid/sizeof *invalid;k++) {
        ForceCalibrationProtocolSubmit(invalid[k],generation);
        assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,",ERR,"));ForceCalibrationProtocolReplySent();
    }
    ForceCalibrationProtocolSubmit("@FC1,9,CAP",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,",CAP,10,1,1,48,1280,1500,"));
    strcpy(saved,reply);assert(ForceCalibrationProtocolReply(reply,sizeof reply) && !strcmp(reply,saved));
    ForceCalibrationProtocolReplySent();
    ForceCalibrationProtocolSubmit("@FC1,10,ARM,9",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,",generation"));ForceCalibrationProtocolReplySent();
    run();ForceCalibrationProtocolSubmit("@FC1,11,STATUS,10",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,",OK,10,"));ForceCalibrationProtocolReplySent();
    ForceCalibrationProtocolSubmit("@FC1,12,STOP,10,0",generation);
    ++generation;ForceCalibrationProtocolCancel(generation,FC_REASON_USB);run();
    ForceCalibrationProtocolSubmit("@FC1,13,ARM,11",generation);run();
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,",OK,11,"));
    ForceCalibrationProtocolReplySent();
    ForceCalibrationProtocolSubmit("@FC1,14,READ,11,1,0,2",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,",DATA,11,"));ForceCalibrationProtocolReplySent();
    ForceCalibrationProtocolSubmit("@FC1,15,META,11,1",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,"@FC1,15,META,11,1,1,9,1,300,50,100,50,1500,"));ForceCalibrationProtocolReplySent();
    FcStatus status;DJIForceCalibrationGetStatus(&status);
    snprintf(saved,sizeof saved,"@FC1,16,PING,11,%lu",(unsigned long)status.session);
    ForceCalibrationProtocolSubmit(saved,generation);run();
    ForceCalibrationProtocolSubmit("@FC1,17,STOP,10,0",generation);
    assert(ForceCalibrationProtocolReply(reply,sizeof reply) && strstr(reply,"@FC1,16,OK,11,"));ForceCalibrationProtocolReplySent();
    puts("FC1 protocol: strict integers, generations, reconnect, old capture and stable replies OK");return 0;
}
