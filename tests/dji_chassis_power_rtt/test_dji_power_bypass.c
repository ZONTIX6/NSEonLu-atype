/* Reuse the driver integration fixture, including the real output and CAN gates. */
#define main PowerControlRegressionMain
#include "test_dji_chassis_power_rtt.c"
#undef main

static void PrepareBypass(void)
{
    test_tick=123;
    test_delta_s=.001f;
    fail_can_transmit=false;
    packing_tick_race=false;
    publish_stop_during_pid=stop_during_meter_read=arm_tick_in_meter=false;
    PreparePowerGroup();
    memset(sender_enable_flag,0,sizeof sender_enable_flag);
    sender_enable_flag[1]=1;
    chassis_power->config.algorithm.max_current_command=15000;
    adapter_control.nominal_budget_w=adapter_control.legacy_budget_w=1;
    adapter_control.target_power_w=adapter_control.power_ceiling_w=1;
    SetMeter(500,true,test_tick);
    for(unsigned w=0;w<4;++w) {
        chassis_power->config.models[w].k4=.0001f;
        dji_motor_instance[w]->motor_settings.close_loop_type=SPEED_LOOP;
        dji_motor_instance[w]->motor_settings.outer_loop_type=SPEED_LOOP;
        adapter_control.target_rpm[w]=1000;
    }
}

static int16_t SubmittedCurrent(unsigned wheel)
{
    const unsigned slot=dji_motor_instance[wheel]->message_num;
    return (int16_t)((uint16_t)submitted_bytes[1][2*slot]<<8 |
                     submitted_bytes[1][2*slot+1]);
}

static void AssertAllStopped(void)
{
    for(unsigned w=0;w<4;++w) {
        ASSERT_TRUE(SubmittedCurrent(w)==0);
        ASSERT_TRUE(chassis_power->state.final_current[w]==0);
    }
}

/* A power-dependent clamp or diagnostic fault must not replace PID demand. */
static void TestBypassKeepsPidDemandAndCurrentBounds(void)
{
    PrepareBypass();
    adapter_control.target_rpm[0]=1000;
    adapter_control.target_rpm[1]=-1000;
    adapter_control.target_rpm[2]=3000;
    adapter_control.target_rpm[3]=-3000;
    DJIMotorControl();
    const int16_t expected[4]={6000,-6000,15000,-15000};
    for(unsigned w=0;w<4;++w) {
        ASSERT_TRUE(SubmittedCurrent(w)==expected[w]);
        ASSERT_TRUE(chassis_power->state.final_current[w]==expected[w]);
        ASSERT_TRUE(chassis_power->state.telemetry.current_command[w]==expected[w]);
    }
    ASSERT_TRUE(chassis_power->state.final_valid && chassis_power->state.can_submitted);
    ASSERT_TRUE(chassis_power->state.submitted_groups==2 && chassis_power->state.successful_groups==2);

    /* The model can fail while raw motor inputs remain sound. */
    chassis_power->config.models[0].k4=NAN;
    DJIMotorControl();
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(SubmittedCurrent(w)==expected[w]);

    fail_can_transmit=true;
    DJIMotorControl();
    ASSERT_TRUE(chassis_power->state.final_valid && !chassis_power->state.can_submitted);
    ASSERT_TRUE(chassis_power->state.submitted_groups==2 && chassis_power->state.successful_groups==0);
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(chassis_power->state.final_current[w]==expected[w]);
}

/* Disabling power control must not disable the independent drive gates. */
static void TestBypassRetainsDriveGates(void)
{
    PrepareBypass();adapter_control.zero_force=true;DJIMotorControl();AssertAllStopped();
    PrepareBypass();adapter_control.control_enabled=false;DJIMotorControl();AssertAllStopped();
    PrepareBypass();safety_drive=false;DJIMotorControl();AssertAllStopped();
    PrepareBypass();adapter_control.tick_ms=test_tick-11;DJIMotorControl();AssertAllStopped();
    PrepareBypass();test_delta_s=.006f;DJIMotorControl();AssertAllStopped();
    PrepareBypass();test_delta_s=NAN;DJIMotorControl();AssertAllStopped();

    PrepareBypass();dji_motor_instance[1]->daemon->online=0;DJIMotorControl();
    ASSERT_TRUE(SubmittedCurrent(1)==0 && chassis_power->state.final_current[1]==0);
    PrepareBypass();SyncTestControl();dji_motor_instance[1]->stop_flag=MOTOR_STOP;DJIMotorControl();
    ASSERT_TRUE(SubmittedCurrent(1)==0 && chassis_power->state.final_current[1]==0);
    PrepareBypass();dji_motor_instance[1]->measure.rx_tick_ms=test_tick-21;DJIMotorControl();
    ASSERT_TRUE(SubmittedCurrent(1)==0 && chassis_power->state.final_current[1]==0);
    PrepareBypass();dji_motor_instance[1]->measure.rx_sequence=0;DJIMotorControl();
    ASSERT_TRUE(SubmittedCurrent(1)==0 && chassis_power->state.final_current[1]==0);

    PrepareBypass();publish_stop_during_pid=true;DJIMotorControl();AssertAllStopped();
    PrepareBypass();stop_in_can_wait=true;DJIMotorControl();AssertAllStopped();
}

/* Exercise malformed inputs before the driver's integer conversion hides NaN. */
static void TestBypassRejectsNonfiniteMotorInputs(void)
{
    for(unsigned invalid=0;invalid<3;++invalid) {
        PrepareBypass();
        SyncTestControl();
        ReadMotorCycle();
        for(unsigned w=0;w<4;++w) {
            motor_output[w]=6000;
            speed_feedback_aps[w]=1000;
            speed_reference_aps[w]=6000;
        }
        switch(invalid) {
        case 0: motor_output[0]=NAN;break;
        case 1: speed_feedback_aps[0]=NAN;break;
        default: speed_reference_aps[0]=NAN;break;
        }
        const DJIMotorCycle_s cycle={.count=idx,.motors=dji_motor_instance,.measure=cycle_measure,
            .current=motor_output,.actual_aps=speed_feedback_aps,.target_aps=speed_reference_aps,
            .stop=cycle_stop,.online=cycle_online};
        AdapterLimit(NULL,&cycle);
        for(unsigned w=0;w<4;++w) ASSERT_TRUE(isfinite(motor_output[w]) && motor_output[w]==0);
    }
}

static void TestVelocityFeedbackSubmissionEvidence(void)
{
    PrepareBypass();
    velocity_feedback_evidence=(DJIVelocityFeedbackEvidence_s){0};
    have_corrected_version=false;
    DJIVelocityFeedbackEvidence_s evidence;
    ASSERT_TRUE(!DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence));
    adapter_control.velocity_feedback_corrected=true;
    adapter_control.target_rpm[1]=-1000;
    DJIMotorControl();
    ASSERT_TRUE(DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence));
    ASSERT_TRUE(evidence.control_version==0 && evidence.flags==255);
    ASSERT_TRUE(evidence.submitted_correction_versions==1);
    ASSERT_TRUE(evidence.final_current[0]==6000 && evidence.final_current[1]==-6000);
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE(evidence.submitted_correction_versions==1); /* Same version twice. */
    adapter_control.version=1;
    fail_can_transmit=true;
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE(!(evidence.flags & DJI_FEEDBACK_CAN_ACCEPTED));
    ASSERT_TRUE(evidence.submitted_correction_versions==1);
    fail_can_transmit=false;
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE(evidence.submitted_correction_versions==2);
    adapter_control.version=2;
    safety_drive=false;
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE((evidence.flags & 240)==0 && evidence.submitted_correction_versions==2);
    safety_drive=true;
    dji_motor_instance[1]->daemon->online=0;
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE((evidence.flags & 240)!=240 && evidence.submitted_correction_versions==2);
    dji_motor_instance[1]->daemon->online=1;
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE(evidence.submitted_correction_versions==3);

    /* A tuning override of even one target must not count as Kalman use. */
    ++adapter_cycle_control.version;
    speed_reference_aps[0]+=6;
    const DJIMotorCycle_s cycle={.motors=dji_motor_instance,.target_aps=speed_reference_aps};
    const int16_t commands[DJI_MOTOR_CNT]={6000};
    const bool enabled[DJI_MOTOR_CNT]={true,true,true,true};
    AdapterRecordFinal(NULL,&cycle,commands,enabled,2,2);
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE(!(evidence.flags & DJI_FEEDBACK_CORRECTED_TARGETS));
    ASSERT_TRUE(evidence.submitted_correction_versions==3);
    adapter_control.version=UINT32_MAX;
    DJIMotorControl();
    adapter_control.version=0;
    DJIMotorControl();
    DJIPowerAdapterGetVelocityFeedbackEvidence(&evidence);
    ASSERT_TRUE(evidence.control_version==0 && evidence.submitted_correction_versions==5);
}

int main(void)
{
    TestBypassKeepsPidDemandAndCurrentBounds();
    TestBypassRetainsDriveGates();
    TestBypassRejectsNonfiniteMotorInputs();
    TestVelocityFeedbackSubmissionEvidence();
    if(failures) return 1;
    puts("PASS: power bypass preserves PID current, current bounds, and final drive gates");
    return 0;
}
