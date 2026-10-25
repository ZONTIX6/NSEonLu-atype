/* Reuse the driver's HAL/CAN/PID test platform, exercising the real force adapter. */
#define main SpeedRegressionMain
#include "../dji_chassis_power_rtt/test_dji_chassis_power_rtt.c"
#undef main
#undef DJIMotorControl
#undef ReadMotorCycle
#include "../../modules/motor/DJImotor/dji_force_adapter.c"
#include <assert.h>
static uint32_t epoch=1;
uint32_t VehicleSafetyEpoch(void) {return epoch;}
uint64_t DWT_GetTimeline_us(void) {return (uint64_t)test_tick*1000;}

static ChassisForceConfig_s fixture(void)
{
    ChassisForceConfig_s c=ChassisForceDefaultConfig();c.calibration_valid=true;
    c.force_max_n=50;c.spin_torque_nm=2;c.spin_start_torque_nm=.2f;c.spin_torque_max_nm=4;
    c.velocity_limit_mps=4;c.omega_limit_rad_s=12;c.wheel_limit_rpm=12000;
    c.force_rise_n_s=100;c.force_fall_n_s=200;c.torque_rise_nm_s=2;c.torque_fall_nm_s=4;
    c.brake_torque_nm=1;c.brake_stop_rad_s=.2f;c.brake_timeout_ms=500;
    c.stall_current_command=4000;c.stall_rpm=20;c.stall_timeout_ms=200;
    c.temp_derate_c=65;c.temp_stop_c=85;c.speed_prediction_rpm=100;c.startup_current_limit=1500;
    for(unsigned w=0;w<4;++w) c.wheel[w]=(ChassisForceWheel_s){
        .x_m=w<2 ? .2f : -.2f,.y_m=w%2 ? -.2f : .2f,.dx=.7071067812f,
        .dy=w==0 || w==3 ? -.7071067812f : .7071067812f,
        .radius_m=.075f,.reduction=19,.k_out_nm_a=.3f,.current_limit=10000,.force_limit_n=50,.current_slew=1000000};
    c.checksum=ChassisForceConfigChecksum(&c);return c;
}
static ChassisForceActuationSnapshot_s actuation;
static ChassisForceCycleOutput_s observed;
static void cycle(bool refresh)
{
    test_tick+=2;
    if(refresh) {
        actuation.power.tick_ms=test_tick;actuation.command_received_ms=test_tick;
        ++actuation.intent.command_seq;
        for(unsigned w=0;w<4;++w) {
            force_motors[w]->measure.rx_tick_ms=test_tick;++force_motors[w]->measure.rx_sequence;
        }
    }
    SetMeter(40,true,test_tick);assert(DJIForceAdapterPublish(&actuation));
    test_pid_calls=0;DJIMotorControl();assert(test_pid_calls==0);
    assert(DJIForceAdapterGetSnapshot(&observed));
    for(unsigned w=0;w<4;++w) {
        const unsigned slot=force_motors[w]->message_num;
        const int16_t packed=(int16_t)((uint16_t)sender_assignment[1].tx_buff[slot*2]<<8|
            sender_assignment[1].tx_buff[slot*2+1]);
        assert(observed.current[w]==packed);
    }
    assert(observed.command_seq==actuation.intent.command_seq && observed.safety_epoch==epoch);
    assert(fabsf(chassis_power->state.telemetry.predicted_total_power_w-observed.predicted_w)<.0001f);
}
static void zero(void) {for(unsigned w=0;w<4;++w) assert(observed.current[w]==0);}
static void neutral_recovery(void)
{
    actuation.intent.input_x=0;actuation.intent.safety_reset=1;
    for(unsigned k=0;k<5;++k) cycle(true);
    assert(observed.faults==0);
    actuation.intent.input_x=1;actuation.intent.safety_reset=0;
    for(unsigned k=0;k<4;++k) cycle(true);
    assert(!observed.faults);
    if(!actuation.power.zero_force) assert(observed.state==CHASSIS_FORCE_RUN || observed.state==CHASSIS_FORCE_LIMITED);
}
int main(void)
{
    PreparePowerGroup();output_group=NULL;
    chassis_power->config.force_control=true;chassis_power->config.algorithm.max_current_command=10000;
    for(unsigned w=0;w<4;++w) {
        chassis_power->config.models[w]=(MotorPowerModelConfig_s){.k4=1,.current_conversion=1000};
        adapter_motors[w]->motor_settings.close_loop_type=SPEED_LOOP;
        adapter_motors[w]->motor_settings.outer_loop_type=SPEED_LOOP;
        adapter_motors[w]->measure.speed_aps=0;
    }
    chassis_power->slow_config=chassis_power->config;
    const ChassisForceConfig_s c=fixture();
    assert(DJIForceAdapterInit(adapter_motors,&c));sender_enable_flag[1]=1;
    actuation=(ChassisForceActuationSnapshot_s){.intent={.motion_mode=CHASSIS_FORCE_TRANSLATE,
        .input_x=1,.force_scale=1,.spin_direction=1,.input_permitted=1},.config_version=1,.safety_epoch=epoch,
        .power={.nominal_budget_w=100,.target_power_w=98,.power_ceiling_w=98,.control_enabled=true},.growth_allowed=true};
    test_delta_s=.050f;cycle(true);zero();assert(!observed.faults); /* First cycle is a clock baseline. */
    test_delta_s=.002f;for(unsigned k=0;k<7;++k) cycle(true);
    assert(observed.state==CHASSIS_FORCE_RUN && observed.current[0]!=0);
    const uint32_t success_ms=controller.last_sent_ms;
    fail_can_transmit=true;cycle(true);
    assert(observed.faults&CHASSIS_FORCE_ERROR_CAN);
    assert(controller.last_sent_ms==success_ms && !observed.can_submitted);
    fail_can_transmit=false;cycle(true);zero();assert(observed.faults);
    neutral_recovery();
    stop_in_can_wait=true;cycle(true);zero();
    assert(!safety_drive && observed.state==CHASSIS_FORCE_DISABLED);
    safety_drive=true;++epoch;actuation.safety_epoch=epoch;
    for(unsigned k=0;k<5;++k) cycle(true);
    test_tick+=7;cycle(false);zero();assert(observed.faults&CHASSIS_FORCE_ERROR_AGE);
    cycle(true);zero();assert(observed.faults);neutral_recovery();
    actuation.command_faults=CHASSIS_FORCE_ERROR_PROTOCOL;cycle(true);zero();
    assert(observed.faults&CHASSIS_FORCE_ERROR_PROTOCOL);
    actuation.command_faults=0;cycle(true);zero();neutral_recovery();
    /* A fault disables energy/DCDC but must still permit neutral recovery checks. */
    controller.faults=CHASSIS_FORCE_ERROR_NUMERIC;actuation.power.zero_force=true;
    neutral_recovery();actuation.power.zero_force=false;
    for(unsigned k=0;k<4;++k) cycle(true);
    assert(!observed.faults);
    actuation.power.control_enabled=false;actuation.power.zero_force=true;
    for(unsigned w=0;w<4;++w) force_motors[w]->measure.speed_aps=600;
    cycle(true);zero();
    assert(observed.actual_rpm[0]==100 && fabsf(observed.velocity[0])>.001f);
    puts("force adapter: PID bypass, exact bytes, ages, CAN failure and recovery OK");
    return failures ? 1 : 0;
}
