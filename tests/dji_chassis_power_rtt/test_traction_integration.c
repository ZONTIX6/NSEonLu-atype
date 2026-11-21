/* Exercise the real driver, PID output hook, power limiter and guarded CAN
 * submission. Only the hardware and PID arithmetic use the existing fixture. */
#define main TractionBaselineRegressionMain
#include "test_dji_chassis_power_rtt.c"
#undef main

static int16_t Sent(unsigned w)
{
    const unsigned slot=dji_motor_instance[w]->message_num;
    return (int16_t)((uint16_t)submitted_bytes[1][2*slot]<<8|submitted_bytes[1][2*slot+1]);
}
static void PrepareTraction(void)
{
    test_tick=1000;test_delta_s=.005f;fail_can_transmit=packing_tick_race=false;
    publish_stop_during_pid=stop_during_meter_read=arm_tick_in_meter=false;
    PreparePowerGroup();
    traction_state=(ChassisTractionState_s){0};
    traction_working=traction_published=(DJITractionSnapshot_s){0};
    memset(sender_enable_flag,0,sizeof sender_enable_flag);sender_enable_flag[1]=1;
    adapter_control.traction=(ChassisTractionReference_s){.valid=true,.tick_ms=test_tick,.gyro_tick_ms=test_tick};
    for(unsigned w=0;w<4;++w) {
        DJIMotorInstance *m=dji_motor_instance[w];
        m->motor_settings.close_loop_type=m->motor_settings.outer_loop_type=SPEED_LOOP;
        m->motor_controller.speed_PID.MaxOut=15000;
        m->motor_controller.speed_PID.Iout=100;
        m->measure.raw_rpm=3000;m->measure.speed_aps=18000;
        adapter_control.target_rpm[w]=1000;
        chassis_power->config.models[w].k4=.0001f;
    }
    SetMeter(10,true,test_tick);
}
static void Cycle(void)
{
    test_tick+=5;adapter_control.tick_ms=test_tick;
    adapter_control.traction.tick_ms=adapter_control.traction.gyro_tick_ms=test_tick;
    for(unsigned w=0;w<4;++w) {
        dji_motor_instance[w]->measure.rx_tick_ms=test_tick;
        ++dji_motor_instance[w]->measure.rx_sequence;
    }
    SetMeter(10,true,test_tick);DJIMotorControl();
}

static void TestTractionReachesSubmittedCommands(void)
{
    PrepareTraction();for(unsigned n=0;n<8;++n) Cycle();
    DJITractionSnapshot_s snapshot;
    ASSERT_TRUE(DJIPowerAdapterGetTractionSnapshot(&snapshot));
    ASSERT_TRUE(snapshot.traction.valid && snapshot.traction.spin_mask==15);
    ASSERT_TRUE(snapshot.cycles==8 && snapshot.spin_cycles>0 && snapshot.can_submitted);
    for(unsigned w=0;w<4;++w) {
#if CHASSIS_TRACTION_MODE == 1
        ASSERT_TRUE(Sent(w)==6000 && snapshot.traction.proposed_current[w]==0);
        ASSERT_TRUE(dji_motor_instance[w]->motor_controller.speed_PID.Iout==100);
#elif CHASSIS_TRACTION_MODE == 2
        ASSERT_TRUE(Sent(w)==0 && snapshot.traction.current[w]==0);
        ASSERT_TRUE(dji_motor_instance[w]->motor_controller.speed_PID.Iout==0);
#else
        ASSERT_TRUE(Sent(w)<0 && Sent(w)>=-300);
        ASSERT_TRUE(snapshot.traction.proposed_current[w]==-300);
        ASSERT_TRUE(dji_motor_instance[w]->motor_controller.speed_PID.Iout==0);
#endif
        ASSERT_TRUE(snapshot.final_current[w]==Sent(w));
        ASSERT_TRUE(chassis_power->state.final_current[w]==Sent(w));
        ASSERT_TRUE(snapshot.raw_current[w]==6000);
    }
#if CHASSIS_TRACTION_MODE == 1
    ASSERT_TRUE(!snapshot.intervention_cycles && !snapshot.submitted_intervention_cycles);
    ASSERT_TRUE(!DJIPowerAdapterTractionLimited(test_tick));
#else
    ASSERT_TRUE(snapshot.intervention_cycles>0 && snapshot.submitted_intervention_cycles>0);
    ASSERT_TRUE(DJIPowerAdapterTractionLimited(test_tick));
    ASSERT_TRUE(!DJIPowerAdapterTractionLimited(test_tick+21));
#endif
    /* Existing final drive permission overrides an active traction brake. */
    stop_in_can_wait=true;Cycle();
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(Sent(w)==0);
    DJIPowerAdapterGetTractionSnapshot(&snapshot);ASSERT_TRUE(!snapshot.enabled_mask);
    ASSERT_TRUE(!DJIPowerAdapterTractionLimited(test_tick));
}

static void TestLostEstimateAndStopsRemoveIntervention(void)
{
    PrepareTraction();for(unsigned n=0;n<8;++n) Cycle();
    adapter_control.traction.valid=false;DJIMotorControl();
    DJITractionSnapshot_s snapshot;DJIPowerAdapterGetTractionSnapshot(&snapshot);
    ASSERT_TRUE(!snapshot.traction.intervened_mask && !snapshot.traction.active_brake_mask);
    ASSERT_TRUE(snapshot.traction.reason==CHASSIS_TRACTION_NO_REFERENCE);
    adapter_control.zero_force=true;DJIMotorControl();
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(Sent(w)==0);
    PrepareTraction();for(unsigned n=0;n<8;++n) Cycle();
    dji_motor_instance[0]->daemon->online=0;DJIMotorControl();
    DJIPowerAdapterGetTractionSnapshot(&snapshot);
    ASSERT_TRUE((snapshot.traction.reason==CHASSIS_TRACTION_BAD_WHEELS ||
        snapshot.traction.reason==CHASSIS_TRACTION_DRIVE_GATED) && !snapshot.traction.intervened_mask);
    ASSERT_TRUE(Sent(0)==0);
    PrepareTraction();for(unsigned n=0;n<8;++n) Cycle();
    const uint32_t accepted=traction_working.submitted_intervention_cycles;
    fail_can_transmit=true;Cycle();DJIPowerAdapterGetTractionSnapshot(&snapshot);
    ASSERT_TRUE(!snapshot.can_submitted && snapshot.submitted_intervention_cycles==accepted);
}

int main(void)
{
    TestTractionReachesSubmittedCommands();TestLostEstimateAndStopsRemoveIntervention();
    if(failures) return 1;
    printf("PASS: traction mode %u, power %u, PID anti-windup and guarded CAN output\n",
        CHASSIS_TRACTION_MODE,CHASSIS_POWER_CONTROL_ENABLE);return 0;
}
