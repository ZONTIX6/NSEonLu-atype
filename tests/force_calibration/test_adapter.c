#include <stdint.h>
static uint32_t irq_mask;
uint32_t __get_PRIMASK(void) {return irq_mask;}
void __disable_irq(void) {irq_mask=1;}
void __set_PRIMASK(uint32_t n) {irq_mask=n;}
#define main SpeedRegressionMain
#define DJI_TEST_CUSTOM_CLOCK
#include "../dji_chassis_power_rtt/test_dji_chassis_power_rtt.c"
#undef main
#undef DJIMotorControl
#undef ReadMotorCycle
#include "../../modules/motor/DJImotor/dji_force_adapter.c"
#include "dji_force_calibration.h"
#include "board_force_calibration.h"
#include "runtime/force_calibration_app.h"
#include <assert.h>
static uint32_t epoch=1,generation=9;
static bool link_open=true;
uint32_t VehicleSafetyEpoch(void) {return epoch;}
uint32_t DWT_GetCycleCount(void) {return test_tick*168000U;}
uint32_t DWT_GetFrequencyHz(void) {return 168000000;}
bool CDC_IsPortOpen_FS(void) {return link_open;}
uint32_t CDC_GetSession_FS(void) {return generation;}
#include "test_power_permission.c"
static void run(void)
{
    test_tick+=2;SetMeter(2,true,test_tick);
    FcPermit p={.power={.control_enabled=true,.nominal_budget_w=50,.power_ceiling_w=50,.tick_ms=test_tick},
        .command_received_ms=test_tick,.safety_epoch=epoch,.bench_permitted=true,.aux_stopped=true};
    DJIForceCalibrationPublish(&p);
    for(unsigned w=0;w<4;w++) {
        DJIMotorInstance *m=adapter_motors[w];m->measure.rx_tick_ms=test_tick;
        m->measure.rx_cycles=DWT_GetCycleCount();m->measure.raw_rpm=0;m->measure.raw_current=0;
        ++m->measure.rx_sequence;DJIForceCalibrationFeedback(m,0,0,m->measure.rx_cycles);
    }
    test_pid_calls=0;DJIMotorControl();assert(test_pid_calls==0);
}
static FcCommandResult request(FcRequest r)
{
    r.generation=generation;assert(DJIForceCalibrationSubmit(&r));run();
    FcCommandResult reply;assert(DJIForceCalibrationReply(&reply));
    DJIForceCalibrationReplyAccepted(r.request_id);return reply;
}
static void packed_zero(void)
{
    for(unsigned w=0;w<4;w++) {
        unsigned slot=adapter_motors[w]->message_num;
        assert(sender_assignment[1].tx_buff[2*slot]==0 && sender_assignment[1].tx_buff[2*slot+1]==0);
    }
}
#ifndef FC_ADAPTER_MAIN
#define FC_ADAPTER_MAIN main
#endif
int FC_ADAPTER_MAIN(void)
{
    PreparePowerGroup();output_group=NULL;
    for(unsigned w=0;w<4;w++) {adapter_motors[w]->motor_type=M3508;adapter_motors[w]->message_num=(uint8_t[]){0,3,1,2}[w];}
    ChassisForceConfig_s c=ChassisForceDefaultConfig();assert(!ChassisForceConfigValid(&c));
    assert(DJIForceAdapterInit(adapter_motors,&c));sender_enable_flag[1]=1;
    assert(ForceCalibrationAppInit(adapter_motors));test_power_permission();run();packed_zero();
    FcCommandResult r=request((FcRequest){.op=FC_OP_ARM,.request_id=1});assert(!r.error);
    uint32_t session=r.status.session;
    assert(RuntimeTuningTrialActive());assert(!RuntimeTuningBeginTrial(1));
    RuntimeTuningEndTrial();assert(RuntimeTuningTrialActive());
    for(unsigned k=0;k<51;k++) {if(k%20==0) request((FcRequest){.op=FC_OP_PING,.request_id=2,.session=session});run();}
    r=request((FcRequest){.op=FC_OP_START,.request_id=3,.session=session,
        .pulse={.wheel=1,.current=300,.rise_ms=50,.hold_ms=100,.fall_ms=50}});assert(!r.error);
    for(unsigned k=0;k<55;k++) {if(k%20==0) request((FcRequest){.op=FC_OP_PING,.request_id=4,.session=session});run();}
    FcStatus s;DJIForceCalibrationGetStatus(&s);assert(s.final[1]==300);
    for(unsigned w=0;w<4;w++) {
        unsigned slot=adapter_motors[w]->message_num;
        int16_t packed=(int16_t)((uint16_t)sender_assignment[1].tx_buff[2*slot]<<8|sender_assignment[1].tx_buff[2*slot+1]);
        assert(packed==(w==1 ? 300 : 0) && packed==s.final[w]);
    }
    stop_in_can_wait=true;run();packed_zero();DJIForceCalibrationGetStatus(&s);
    assert(s.reason==FC_REASON_PERMISSION && s.zero_submitted);
    safety_drive=true;
    for(unsigned k=0;k<52;k++) run();
    DJIForceCalibrationGetStatus(&s);assert(s.state==FC_ABORTED && !s.complete);
    uint8_t bytes[96];assert(DJIForceCalibrationRead(s.capture_id,0,2,bytes,sizeof bytes));
    assert(!RuntimeTuningTrialActive());
    uint32_t old_capture=s.capture_id;++generation;link_open=false;run();link_open=true;run();
    assert(DJIForceCalibrationRead(old_capture,0,2,bytes,sizeof bytes));
    r=request((FcRequest){.op=FC_OP_ARM,.request_id=6});assert(!r.error);
    FcRequest queued={.op=FC_OP_START,.generation=generation,.session=r.status.session,.request_id=7,
        .pulse={.wheel=1,.current=300,.rise_ms=50,.hold_ms=100,.fall_ms=50}};
    assert(DJIForceCalibrationSubmit(&queued));
    assert(DJIForceCalibrationSubmit(&(FcRequest){.op=FC_OP_STOP,.generation=generation,.request_id=8}));
    run();packed_zero();
    assert(irq_mask==0);puts("calibration real driver: mapping, PID bypass, enqueue stop, freeze, ownership OK");return 0;
}
