/* Included by the real driver fixture so auxiliary motors use the real registry. */
static void test_power_permission(void)
{
    ChassisForceIntentReceiver_s c={.valid=true,.received_ms=test_tick,
        .intent={.input_permitted=1,.bench_permitted=1,.spin_direction=1}};
    assert(!ForceCalibrationAppPermit(&c,test_tick));
    test_tick+=100;c.received_ms=test_tick;
    assert(ForceCalibrationAppPermit(&c,test_tick));
    c.intent.source_age_ms=11;assert(!ForceCalibrationAppPermit(&c,test_tick));c.intent.source_age_ms=0;
    c.intent.yaw_age_ms=11;assert(!ForceCalibrationAppPermit(&c,test_tick));c.intent.yaw_age_ms=0;
    c.intent.input_x=.1f;assert(!ForceCalibrationAppPermit(&c,test_tick));c.intent.input_x=0;
    safety_bench=false;assert(!ForceCalibrationAppPermit(&c,test_tick));safety_bench=true;
    static DJIMotorInstance aux;static DaemonInstance daemon={.online=1};
    aux.daemon=&daemon;aux.stop_flag=MOTOR_ENALBED;aux.measure.rx_sequence=1;
    aux.measure.rx_tick_ms=test_tick;dji_motor_instance[idx++]=&aux;
    assert(!ForceCalibrationAppPermit(&c,test_tick));aux.stop_flag=MOTOR_STOP;
    assert(!ForceCalibrationAppPermit(&c,test_tick));test_tick+=100;c.received_ms=test_tick;aux.measure.rx_tick_ms=test_tick;
    assert(ForceCalibrationAppPermit(&c,test_tick));
    aux.measure.speed_aps=NAN;assert(!ForceCalibrationAppPermit(&c,test_tick));--idx;
}
