/* Reuse the real FIFO/UART fixture, with the default compact wire format. */
#define main FullCaptureMain
#include "test_kalman_capture.c"
#undef main

static void frozen_trace_keeps_unsampled_corrections(void)
{
    const ChassisVelocityControlTrialConfig_s config={.max_speed_mps=2.7f,
        .proportional_gain=.25f,.max_correction_mps=.05f,.deadband_mps=.02f,.max_sample_age_ms=40};
    ChassisVelocityControlTrialInput_s input={.enabled=true,.speed_limit_allowed=true,
        .feedback_allowed=true,.fused_snapshot_valid=true,.fused_status=true,.wheel_valid=true,
        .now_ms=975,.fused_tick_ms=950,.requested_vx_mps=.2f};
    ChassisVelocityControlTrialOutput_s output;
    ChassisVelocityControlTrace_s trace={.control_cycles=65535};
    const float rpm[4]={1000,-1100,1200,-1300};
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,975,65535,rpm,true,true);
    /* No VOFA capture on the active control cycle. The next filter call sees
     * stale feedback and zero current correction, but preserves its history. */
    input.now_ms=1000;
    assert(ChassisVelocityControlTrialApply(&config,&input,&output));
    assert(!output.feedback_active);
    ChassisVelocityControlTrialRecord(&trace,&config,&input,&output,1000,65536,rpm,true,true);
    trace.motor=(DJIVelocityFeedbackEvidence_s){.tick_ms=999,.control_version=65535,
        .submitted_correction_versions=0x12345678U,.flags=255,
        .final_current={-32768,32767,-1234,1234}};
    sample(1000,1);
    velocity.diagnostics.control_trace=trace;
    VofaKalmanCapture(&velocity,&imu,true,&target,1000);
    memset(&velocity,0,sizeof velocity);
    memset(&trace,0,sizeof trace);
    tick=5000;
    VofaKalmanCaptureDrain();
    check_frame(active);
#if CHASSIS_WHEEL_REDUNDANCY_MODE > 0
    assert(FRAME_BYTES==336);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_MODE)==CHASSIS_WHEEL_REDUNDANCY_MODE);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_REASON)==CHASSIS_WHEEL_REDUNDANCY_CORRECTED);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_EXCLUDED_WHEEL)==2);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_USED_WHEEL_MASK)==11);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_CONFIRMATIONS)==2);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_CORRECTED)==1);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_APPLIED)==(CHASSIS_WHEEL_REDUNDANCY_MODE>=2));
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_RESIDUAL_LB_MPS)==.45f);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_SELECTED_VX_MPS)==.45f);
    assert(fabsf(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_REFERENCE_ERROR_MPS)-.07f)<1e-6f);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_THRESHOLD_MPS)==.25f);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_DOMINANCE_RATIO)==1.75f);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_MAX_REFERENCE_ERROR_MPS)==.30f);
    assert(field(active,VOFA_KALMAN_CONTROL_REDUNDANCY_CONFIRM_SAMPLES_REQUIRED)==2);
#else
    assert(FRAME_BYTES==260);
#endif
    assert(field(active,VOFA_KALMAN_CONTROL_WHEEL_VX_MPS)==.5f);
    assert(field(active,VOFA_KALMAN_CONTROL_TARGET_VX_MPS)==1.2345f);
    assert(field(active,VOFA_KALMAN_CONTROL_TRIAL_FUSED_AGE_MS)==50);
    assert(field(active,VOFA_KALMAN_CONTROL_TRIAL_AGE_MS)==0);
    assert(field(active,VOFA_KALMAN_CONTROL_TRIAL_CORRECTION_VX_MPS)==0);
    assert(fabsf(field(active,VOFA_KALMAN_CONTROL_LAST_CORRECTION_VX_MPS)-.05f)<1e-6f);
    assert(field(active,VOFA_KALMAN_CONTROL_LAST_CORRECTION_AGE_MS)==25);
    assert(word(active,VOFA_KALMAN_CONTROL_CONTROL_CYCLES_HI)==65537);
    assert(word(active,VOFA_KALMAN_CONTROL_FEEDBACK_CYCLES_HI)==1);
    assert(word(active,VOFA_KALMAN_CONTROL_CORRECTION_CYCLES_HI)==1);
    assert(word(active,VOFA_KALMAN_CONTROL_SUBMITTED_CORRECTION_VERSIONS_HI)==0x12345678U);
    assert(word(active,VOFA_KALMAN_CONTROL_PUBLISHED_CONTROL_VERSION_HI)==65536);
    assert(word(active,VOFA_KALMAN_CONTROL_APPLIED_CONTROL_VERSION_HI)==65535);
    assert(word(active,VOFA_KALMAN_CONTROL_LAST_CORRECTION_CONTROL_VERSION_HI)==65535);
    assert(field(active,VOFA_KALMAN_CONTROL_APPLIED_CONTROL_AGE_MS)==1);
    assert(field(active,VOFA_KALMAN_CONTROL_FINAL_CURRENT_LF)==-32768);
    assert(field(active,VOFA_KALMAN_CONTROL_FINAL_CURRENT_RF)==32767);
    assert(field(active,VOFA_KALMAN_CONTROL_TARGET_RPM_RB)==-1300);
    assert(field(active,VOFA_KALMAN_CONTROL_LAST_CORRECTION_TARGET_RPM_RB)==-1300);
    assert(((unsigned)field(active,VOFA_KALMAN_CONTROL_VALID) & (16384U|32768U))==(16384U|32768U));
    assert(((unsigned)field(active,VOFA_KALMAN_CONTROL_VALID) & (16U|1024U|8192U))==0);
    capture(1020,2); /* Unavailable trace uses explicit unknown ages. */
    finish_uart();
    VofaKalmanCaptureDrain();
    check_frame(active);
    assert(((unsigned)field(active,VOFA_KALMAN_CONTROL_VALID) & (16384U|32768U))==0);
    assert(field(active,VOFA_KALMAN_CONTROL_TRIAL_AGE_MS)==-1);
    assert(field(active,VOFA_KALMAN_CONTROL_TRIAL_FUSED_AGE_MS)==-1);
    assert(field(active,VOFA_KALMAN_CONTROL_LAST_CORRECTION_AGE_MS)==-1);
    assert(field(active,VOFA_KALMAN_CONTROL_APPLIED_CONTROL_AGE_MS)==-1);
    assert(read_imu_calls==0 && clock_reads==0 && stats().pending==0);
}

static void compact_transport_handles_jitter_and_stall(void)
{
    enum { EVENTS=400 };
    unsigned next=0;
    uint32_t next_tick=1000;
    const uint32_t wire_ms=(FRAME_BYTES*10U*1000U+BOARD_UART7_BAUDRATE-1U)/BOARD_UART7_BAUDRATE;
#if CHASSIS_WHEEL_REDUNDANCY_MODE > 0
    assert(wire_ms==15);
#else
    assert(wire_ms==12); /* Full frame formerly needed 20 ms rounded up. */
#endif
    for(tick=1000;tick<9300;++tick) {
        if(huart7.gState==HAL_UART_STATE_BUSY_TX && tick-send_started>=wire_ms) finish_uart();
        if(next<EVENTS && tick==next_tick) {
            capture(tick,1U+next);
            ++next;
            next_tick+=(next & 1U) ? 19U : 21U;
        } else if(next && next<EVENTS) {
            VofaKalmanCapture(&velocity,&imu,true,&target,tick);
        }
        if(tick<3000 || tick>=3070) VofaKalmanCaptureDrain();
    }
    assert(next==EVENTS && sent==EVENTS && stats().pending==0 && stats().dropped==0);
    for(unsigned i=0;i<EVENTS;++i) {
        check_frame(frames[i]);
        assert(word(frames[i],VOFA_KALMAN_CONTROL_TX_SEQUENCE_HI)==i);
    }
    assert(read_imu_calls==0);
}

int main(int argc,char **argv)
{
    assert(argc==2);
    if(!strcmp(argv[1],"evidence")) frozen_trace_keeps_unsampled_corrections();
    else if(!strcmp(argv[1],"jitter")) compact_transport_handles_jitter_and_stall();
    else assert(!"unknown scenario");
    assert(critical==0);
    printf("PASS: compact Kalman capture %s\n",argv[1]);
    return 0;
}
