#define FC_ADAPTER_MAIN AdapterRegressionMain
#include "test_adapter.c"
int main(void)
{
    assert(AdapterRegressionMain()==0);FcCommandResult old;
    if(DJIForceCalibrationReply(&old)) DJIForceCalibrationReplyAccepted(old.request_id);
    FcCommandResult r=request((FcRequest){.op=FC_OP_ARM,.request_id=100});assert(!r.error);
    uint32_t session=r.status.session;
    for(unsigned k=0;k<51;k++) {if(k%20==0) request((FcRequest){.op=FC_OP_PING,.request_id=101,.session=session});run();}
    r=request((FcRequest){.op=FC_OP_START,.request_id=102,.session=session,
        .pulse={.wheel=1,.current=-300,.rise_ms=50,.hold_ms=100,.fall_ms=50}});assert(!r.error);
    for(unsigned k=0;k<55;k++) {if(k%20==0) request((FcRequest){.op=FC_OP_PING,.request_id=103,.session=session});run();}
    fail_can_transmit=true;run();FcStatus s;DJIForceCalibrationGetStatus(&s);
    assert(s.reason==FC_REASON_CAN && !s.zero_submitted);uint8_t bytes[96];
    assert(!DJIForceCalibrationRead(s.capture_id,0,2,bytes,sizeof bytes));
    run();packed_zero();DJIForceCalibrationGetStatus(&s);assert(!s.zero_submitted);
    fail_can_transmit=false;run();packed_zero();
    for(unsigned k=0;k<52;k++) run();
    DJIForceCalibrationGetStatus(&s);assert(s.zero_submitted && s.state==FC_ABORTED && !s.complete);
    bool failed_nonzero=false;
    for(unsigned first=0;first<s.records;first+=2) {
        uint8_t count=(uint8_t)(s.records-first>=2?2:1);
        assert(DJIForceCalibrationRead(s.capture_id,(uint16_t)first,count,bytes,sizeof bytes));
        for(unsigned k=0;k<count;k++) {
            const uint8_t *b=bytes+k*48;
            if(b[0]==FC_CONTROL && b[46]==2 && b[47]==0 && (int16_t)((uint16_t)b[28]|(uint16_t)b[29]<<8)==-300) failed_nonzero=true;
        }
    }
    assert(failed_nonzero);puts("real CAN failure: failed command retained, successful zero required for export");return 0;
}
