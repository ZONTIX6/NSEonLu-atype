#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../modules/power_meter/power_meter.h"

#include "chassis_power_internal.h"
#include "../../modules/chassis_power/chassis_power.c"
#include "chassis_power_config.h"
#include "../../modules/motor/DJImotor/dji_motor.c"
#include "../../modules/motor/DJImotor/dji_power_adapter.c"

static void SyncTestControl(void)
{
    if(adapter_group.control.version!=adapter_control.version ||
       adapter_group.control.tick_ms!=adapter_control.tick_ms ||
       adapter_group.control.control_enabled!=adapter_control.control_enabled ||
       adapter_group.control.autotune_permitted!=adapter_control.autotune_permitted ||
       adapter_group.control.zero_force!=adapter_control.zero_force ||
       memcmp(adapter_group.control.target_rpm,adapter_control.target_rpm,sizeof adapter_control.target_rpm)!=0)
        (void)DJIPowerAdapterPublish(&adapter_control);
}
static void TestReadMotorCycle(void)
{
    SyncTestControl();
    ReadMotorCycle();
}
static void TestMotorControl(void)
{
    SyncTestControl();
    DJIMotorControl();
}
#define ReadMotorCycle TestReadMotorCycle
#define DJIMotorControl TestMotorControl

static void ApplyChassisPowerGroup(void)
{
    ReadMotorCycle();
    const DJIMotorCycle_s cycle={.count=idx,.motors=dji_motor_instance,.measure=cycle_measure,
        .current=motor_output,.actual_aps=speed_feedback_aps,.target_aps=speed_reference_aps,
        .stop=cycle_stop,.online=cycle_online};
    AdapterLimit(NULL,&cycle);
    int16_t commands[DJI_MOTOR_CNT]={0};bool enabled[DJI_MOTOR_CNT]={false};
    for(unsigned i=0;i<idx;++i) {
        commands[i]=(int16_t)motor_output[i];
        enabled[i]=cycle_stop[i]!=MOTOR_STOP && cycle_online[i] && adapter_cycle_control.control_enabled;
    }
    AdapterRecordFinal(NULL,&cycle,commands,enabled,0x3f,0x3f);
}

static int failures;
static unsigned test_pid_calls;
static char captured_data[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];
static PowerMeterStatus_s test_meter;
static bool publish_stop_during_pid;
static bool stop_during_meter_read, fail_can_transmit;
static bool arm_tick_in_meter, packing_tick_race;
static unsigned packing_tick_calls;
static uint32_t test_tick=123;
static float test_delta_s=.001f;
static bool safety_drive=true, safety_bench=true;
static bool stop_in_can_wait;
bool VehicleSafetyAllowsDrive(uint32_t now) {(void)now;return safety_drive;}
bool VehicleSafetyAllowsAutotune(uint32_t now) {(void)now;return safety_bench;}


static void SetMeter(float power, bool online, uint32_t tick)
{
    test_meter = (PowerMeterStatus_s){.power_w = power, .online = online, .last_rx_tick_ms = tick};
}
static void PublishTelemetry(const ChassisPowerControlTelemetry_s *telemetry)
{
    if (telemetry == NULL) return;
    ChassisPowerControlSnapshot_s control = adapter_control;
    control.legacy_budget_w = chassis_power->input.referee_power_limit_w;
    control.telemetry = *telemetry;
    DJIPowerAdapterPublish(&control);
}

bool PowerMeterGetStatus(PowerMeterStatus_s *status)
{
    *status = test_meter;
    if(stop_during_meter_read) {stop_during_meter_read=false;safety_drive=false;}
    if(arm_tick_in_meter) {arm_tick_in_meter=false;packing_tick_race=true;packing_tick_calls=0;}
    return true;
}


#define ASSERT_TRUE(condition)                                                   \
    do                                                                           \
    {                                                                            \
        if (!(condition))                                                        \
        {                                                                        \
            printf("FAIL:%s:%d: %s\n", __FILE__, __LINE__, #condition);         \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

#define ASSERT_STREQ(expected, actual) ASSERT_TRUE(strcmp((expected), (actual)) == 0)

static void AssertControlColumnsMatch(const char *expected)
{
#if CHASSIS_POWER_CALIBRATION_ENABLE
    const size_t prefix_length = strlen(expected) - 2U;
    if (strncmp(expected, captured_data, prefix_length) != 0)
        printf("expected %sactual %s", expected, captured_data);
    ASSERT_TRUE(strncmp(expected, captured_data, prefix_length) == 0);
    ASSERT_TRUE(captured_data[prefix_length] == ',');
#else
    if (strcmp(expected, captured_data) != 0) printf("expected %sactual %s", expected, captured_data);
    ASSERT_STREQ(expected, captured_data);
#endif
}

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

int CANBusIndex(const CAN_HandleTypeDef *handle)
{
    return handle==&hcan1 ? 0 : handle==&hcan2 ? 1 : -1;
}

CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    (void)config;
    return NULL;
}

uint8_t CANTransmit(CANInstance *instance, float timeout)
{
    (void)instance;
    (void)timeout;
    return fail_can_transmit ? 0U : 1U;
}
uint8_t CANTransmitGuarded(CANInstance *instance,float timeout,void (*guard)(void *),void *context)
{
    /* Mailbox waiting can preempt the motor task immediately before enqueue. */
    if(stop_in_can_wait) {stop_in_can_wait=false;safety_drive=false;}
    if(guard) guard(context);
    return CANTransmit(instance,timeout);
}

uint32_t HAL_GetTick(void)
{
    if(packing_tick_race && ++packing_tick_calls>=3) return test_tick+21;
    return test_tick;
}

void PIDInit(PIDInstance *pid, PID_Init_Config_s *config)
{
    (void)pid;
    (void)config;
}

float PIDCalculate(PIDInstance *pid, float measure, float ref)
{
    ++test_pid_calls;
    (void)pid;
    (void)measure;
    if (publish_stop_during_pid) {
        publish_stop_during_pid = false;
        ChassisPowerControlSnapshot_s next = adapter_control;
        ++next.version; next.control_enabled = false;
        DJIPowerAdapterPublish(&next);
    }
    return ref;
}

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{
    (void)config;
    return NULL;
}

void DaemonReload(DaemonInstance *instance)
{
    (void)instance;
}

uint8_t DaemonIsOnline(DaemonInstance *instance)
{
    return instance != NULL ? instance->online : 0U;
}

float DWT_GetDeltaT(uint32_t *count)
{
    *count=1234;
    return test_delta_s;
}

unsigned BSPLogWrite(const void *data, unsigned length)
{
    if (length >= sizeof(captured_data))
        return 0U;
    memcpy(captured_data, data, length);
    captured_data[length] = '\0';
    return length;
}

unsigned BSPLogWriteString(const char *text)
{
    return BSPLogWrite(text, (unsigned)strlen(text));
}

static void PreparePowerGroup(void)
{
    static ChassisPowerInstance_s power_group;
    static DJIMotorInstance motors[POWER_MODEL_MOTOR_COUNT];
    static DaemonInstance daemons[POWER_MODEL_MOTOR_COUNT];
    static CANInstance can[POWER_MODEL_MOTOR_COUNT];

    memset(&power_group, 0, sizeof(power_group));
    memset(motors, 0, sizeof(motors));
    memset(daemons, 0, sizeof(daemons));
    memset(dji_motor_instance, 0, sizeof(dji_motor_instance));
    memset(motor_output, 0, sizeof(motor_output));
    memset(speed_reference_aps, 0, sizeof(speed_reference_aps));
    memset(speed_feedback_aps, 0, sizeof(speed_feedback_aps));
    memset(captured_data, 0, sizeof(captured_data));
    memset(&test_meter, 0, sizeof(test_meter));
    safety_drive=safety_bench=true;
    idx = 4;
    sender_assignment[1].can_handle=&hcan1;

    power_group.state.registered = true;
    power_group.config.stage=CHASSIS_POWER_CONTROL_STAGE;
    power_group.config.meter_enabled=POWER_METER_ENABLE!=0;
    power_group.config.transient=ChassisPowerTransientDefaultConfig();
    adapter_group=(DJIMotorOutputGroup_s){.registered=true,.max_current_command=15000};
    output_group=&adapter_group;
    output_hooks=(DJIMotorOutputHooks_s){.read_control=AdapterReadControl,
        .limit=AdapterLimit,.record_final=AdapterRecordFinal};
    power_group.input.referee_power_limit_w = 100.0f;
    adapter_control = (ChassisPowerControlSnapshot_s){
        .tick_ms=test_tick,
        .nominal_budget_w = 100, .target_power_w = 95,
        .legacy_budget_w = 100, .control_enabled = true};
    power_group.input.attenuation = 1.0f;
    power_group.config.algorithm = (ChassisPowerAlgorithmConfig_s){
        .safety_factor = 1.0f,
        .small_error_threshold_rpm = 0.0f,
        .reserved_power_threshold_w = 4.0f,
        .per_motor_reserved_power_w = 1.0f,
        .max_current_command = 100.0f,
    };

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        adapter_group.motor_indices[i] = (uint8_t)i;
        power_group.config.models[i].current_conversion = 1.0f;
        daemons[i].online = 1U;
        motors[i].daemon = &daemons[i];
        motors[i].measure.rx_sequence=1;
        motors[i].measure.rx_tick_ms=test_tick;
        can[i].can_handle=&hcan1;can[i].rx_id=0x201U+i;
        motors[i].motor_can_instance=&can[i];
        motors[i].stop_flag = MOTOR_ENALBED;
        motors[i].sender_group = 1;
        motors[i].message_num = (uint8_t)i;
        dji_motor_instance[i] = adapter_motors[i] = &motors[i];
        motors[i].motor_type=M3508;
        motor_output[i] = (float)(i + 1U);
    }

    power_instance = &power_group;
    chassis_power->telemetry_count = 0U;
    (void)DJIPowerAdapterPublish(&adapter_control);
}

static void EmitOneRow(void)
{
    for (size_t i = 0; i < CHASSIS_POWER_RTT_DIVIDER; ++i)
        ApplyChassisPowerGroup();
}

static void TestAtomicPublicationAndFinalIntegerCommands(void)
{
    PreparePowerGroup();
    const ChassisPowerControlSnapshot_s control = {
        .version = 42, .tick_ms = 120, .nominal_budget_w = 20,
        .target_power_w = 19, .legacy_budget_w = 20, .control_enabled = true,
        .target_rpm = {1000, -1000, 500, -500},
    };
    DJIPowerAdapterPublish(&control);
    motor_output[0] = 3.9f;
    ApplyChassisPowerGroup();
    ChassisPowerSnapshot_s state;
    ASSERT_TRUE(ChassisPowerGetSnapshot(&state));
    ASSERT_TRUE(state.control_version == 42);
    ASSERT_TRUE(state.raw_current[0] == 3.9f);
    ASSERT_TRUE(state.power.limited_current[0] == 3);
    ASSERT_TRUE(dji_motor_instance[1]->motor_controller.pid_ref == -6000);
    DJIMotorStop(dji_motor_instance[0]);
    ApplyChassisPowerGroup();
    ASSERT_TRUE(chassis_power->state.power.limited_current[0] == 0);
}

static void TestZeroDemandIsNotActualBaseLimiting(void)
{
    PreparePowerGroup();
    for (unsigned w = 0; w < 4; ++w) {
        chassis_power->config.models[w].k0 = 30;
        motor_output[w] = 0;
    }
    ApplyChassisPowerGroup();
    ASSERT_TRUE(!chassis_power->state.base_limited);
}

static void TestTargetSnapshotDoesNotDependOnFeedbackRounding(void)
{
    PreparePowerGroup();
    for (unsigned w = 0; w < 4; ++w) speed_reference_aps[w] = 6000;
    const float feedback[] = {5.10000038f, 2700.3f, 5399.99f};
    for (unsigned n = 0; n < 3; ++n) {
        speed_feedback_aps[0] = feedback[n];
        ApplyChassisPowerGroup();
        ASSERT_TRUE(chassis_power->state.telemetry.target_speed_rpm[0] == 1000);
    }
    motor_output[0] = NAN;
    ApplyChassisPowerGroup();
    ASSERT_TRUE(chassis_power->state.numeric_error && chassis_power->state.transient.fault);
    for (unsigned w = 0; w < 4; ++w) ASSERT_TRUE(chassis_power->state.power.limited_current[w] == 0);
}

static void TestStopPublicationDoesNotMixCanAndDiagnostics(void)
{
    PreparePowerGroup(); idx = 4;
    ChassisPowerControlSnapshot_s control = adapter_control;
    control.version = 10;
    for (unsigned i = 0; i < 4; ++i) {
        control.target_rpm[i] = 1000;
        dji_motor_instance[i]->motor_settings.close_loop_type = SPEED_LOOP;
        dji_motor_instance[i]->motor_settings.outer_loop_type = SPEED_LOOP;
    }
    DJIPowerAdapterPublish(&control);
    publish_stop_during_pid = true;
    DJIMotorControl();
    ASSERT_TRUE(chassis_power->state.control_version == 10);
    for (unsigned i = 0; i < 4; ++i) {
        int16_t sent = (int16_t)((uint16_t)sender_assignment[1].tx_buff[2*i] << 8 |
                                sender_assignment[1].tx_buff[2*i+1]);
        ASSERT_TRUE(sent == chassis_power->state.power.limited_current[i]);
    }
    DJIMotorControl();
    ASSERT_TRUE(chassis_power->state.control_version == 11);
    for (unsigned i = 0; i < 4; ++i) ASSERT_TRUE(chassis_power->state.power.limited_current[i] == 0);
    idx = 0;
}

static void TestFastProtectionObservesOrAppliesByStage(void)
{
    PreparePowerGroup();
    ChassisPowerControlSnapshot_s control = adapter_control;
    control.nominal_budget_w = control.legacy_budget_w = 20;
    control.target_power_w = 19; control.slow_correction_w = 3;
    DJIPowerAdapterPublish(&control);
    for (unsigned i = 0; i < 4; ++i) {
        chassis_power->config.models[i].k4 = .01f;
        motor_output[i] = 40;
    }
    SetMeter(36.1f, true, 123);
    ApplyChassisPowerGroup();
#if POWER_METER_ENABLE
    ASSERT_TRUE(chassis_power->state.transient.fast_w < -8);
    ASSERT_TRUE(chassis_power->state.proposed_current[0] <= 16);
#endif
#if CHASSIS_POWER_CONTROL_STAGE == 0 || !POWER_METER_ENABLE
    ASSERT_TRUE(chassis_power->state.power.limited_current[0] == 22);
#else
    ASSERT_TRUE(chassis_power->state.power.limited_current[0] <= 16);
#endif
}

static void TestFinalGateAndCanResultAreRecorded(void)
{
    PreparePowerGroup();
    sender_enable_flag[1]=1;
    for(unsigned w=0;w<4;++w) {
        adapter_control.target_rpm[w]=1000;
        dji_motor_instance[w]->motor_settings.close_loop_type=SPEED_LOOP;
        dji_motor_instance[w]->motor_settings.outer_loop_type=SPEED_LOOP;
    }
#if POWER_METER_ENABLE
    stop_during_meter_read=true;
    DJIMotorControl();
    ASSERT_TRUE(chassis_power->state.raw_current[0]>0);
    ASSERT_TRUE(chassis_power->state.final_valid);
    for(unsigned w=0;w<4;++w) {
        ASSERT_TRUE(chassis_power->state.final_current[w]==0);
        ASSERT_TRUE(chassis_power->state.telemetry.current_command[w]==0);
        ASSERT_TRUE(sender_assignment[1].tx_buff[2*w]==0 && sender_assignment[1].tx_buff[2*w+1]==0);
    }
#endif
    safety_drive=true;fail_can_transmit=true;
    DJIMotorControl();
    ASSERT_TRUE(!chassis_power->state.can_submitted);
    fail_can_transmit=false;
    DJIMotorControl();
    ASSERT_TRUE(chassis_power->state.can_submitted);
#if POWER_METER_ENABLE
    arm_tick_in_meter=true;
    DJIMotorControl();
    for(unsigned w=0;w<4;++w) {
        unsigned index=adapter_group.motor_indices[w];
        unsigned slot=dji_motor_instance[index]->message_num;
        int16_t packed=(int16_t)((uint16_t)sender_assignment[1].tx_buff[2*slot]<<8 |
                                 sender_assignment[1].tx_buff[2*slot+1]);
        ASSERT_TRUE(chassis_power->state.final_current[w]==packed);
    }
    packing_tick_race=false;
#endif
}

static void TestAdapterRejectsInvalidMembership(void)
{
    PreparePowerGroup();
    output_group=NULL;adapter_group.registered=false;
    DJIPowerAdapterConfig_s config={.count=4,.max_current_command=100};
    for(unsigned w=0;w<4;++w) config.motors[w]=dji_motor_instance[w];
    ASSERT_TRUE(!DJIPowerAdapterInit(NULL));
    config.count=5;ASSERT_TRUE(!DJIPowerAdapterInit(&config));config.count=4;
    config.motors[3]=config.motors[0];ASSERT_TRUE(!DJIPowerAdapterInit(&config));
    config.motors[3]=dji_motor_instance[3];
    config.motors[0]->motor_type=M2006;ASSERT_TRUE(!DJIPowerAdapterInit(&config));
    config.motors[0]->motor_type=M3508;
    DJIMotorInstance unregistered={.motor_type=M3508};
    config.motors[0]=&unregistered;ASSERT_TRUE(!DJIPowerAdapterInit(&config));
    config.motors[0]=NULL;ASSERT_TRUE(!DJIPowerAdapterInit(&config));
    config.motors[0]=dji_motor_instance[0];ASSERT_TRUE(DJIPowerAdapterInit(&config));
    ASSERT_TRUE(!DJIPowerAdapterInit(&config));
}

static void TestControlAndMeasurementSnapshotsReachOneRttRow(void)
{
    PreparePowerGroup();
    const ChassisPowerControlTelemetry_s control = {
        .nominal_budget_w = 30.0f,
        .target_power_w = 28.5f,
        .corrected_budget_w = 27.0f,
        .filtered_power_w = 29.0f,
        .error_w = -0.5f,
        .correction_w = -3.0f,
        .outer_mode = 2U,
    };
    PublishTelemetry(&control);
    SetMeter(12.34f, true, 77U);

    EmitOneRow();

    AssertControlColumnsMatch(
        "123,3000,2850,2700,10000,0,0,1234,2900,-50,-300,"
        "1000,0,2,1,46,15\r\n");
}

static void TestOfflineMeasurementKeepsValueAndTick(void)
{
    PreparePowerGroup();
    SetMeter(9.0f, false, 88U);

    EmitOneRow();

    AssertControlColumnsMatch(
        "123,0,0,0,10000,0,0,900,0,0,0,1000,0,0,0,35,15\r\n");
}

static void TestNonfiniteMeasurementBecomesZero(void)
{
    PreparePowerGroup();
    SetMeter(NAN, true, 99U);

    EmitOneRow();

    AssertControlColumnsMatch(
        "123,0,0,0,10000,0,0,0,0,0,0,1000,0,0,1,24,15\r\n");
}

static void TestNullControlSnapshotLeavesPreviousValue(void)
{
    PreparePowerGroup();
    const ChassisPowerControlTelemetry_s control = {
        .nominal_budget_w = 1.0f,
        .outer_mode = 1U,
    };
    PublishTelemetry(&control);
    PublishTelemetry(NULL);

    EmitOneRow();

    AssertControlColumnsMatch(
        "123,100,0,0,10000,0,0,0,0,0,0,1000,0,1,0,0,15\r\n");
}

static void TestRttDividerRemainsTenMotorCycles(void)
{
    PreparePowerGroup();

    for (size_t i = 1; i < CHASSIS_POWER_RTT_DIVIDER; ++i)
        ApplyChassisPowerGroup();
    ASSERT_STREQ("", captured_data);

    ApplyChassisPowerGroup();
    ASSERT_TRUE(captured_data[0] != '\0');
}

static void TestCalibrationUsesOutputCommandsAndActualRpm(void)
{
    PreparePowerGroup();
    SetMeter(18.0f, true, 117U);
    motor_output[0] = -1.0f;
    motor_output[2] = -3.0f;
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
        speed_feedback_aps[i] = motor_output[i] * 600.0f;
    dji_motor_instance[2]->stop_flag = MOTOR_STOP;
    dji_motor_instance[3]->daemon->online = 0U;

    EmitOneRow();
    const char expected[] =
        "123,0,0,0,10000,0,0,1800,0,0,0,1000,0,0,1,6,3"
#if CHASSIS_POWER_CALIBRATION_ENABLE
        ",-1,2,0,0,-100,200,-300,400,117"
#endif
        "\r\n";
    ASSERT_STREQ(expected, captured_data);
}

static void TestCalibrationPrintsLimitedCurrentsInsteadOfPidDemand(void)
{
    PreparePowerGroup();
    chassis_power->input.referee_power_limit_w = 4.0f;
    const ChassisPowerControlTelemetry_s control = {
        .nominal_budget_w = 4.0f,
        .target_power_w = 4.0f,
        .corrected_budget_w = 4.0f,
        .filtered_power_w = 3.9f,
        .error_w = 0.1f,
        .outer_mode = 2U,
    };
    PublishTelemetry(&control);
    SetMeter(3.9f, true, 117U);
    const float demands[] = {2.0f, -3.0f, 4.0f, -5.0f};
    const float feedback_aps[] = {600.0f, -1200.0f, 1800.0f, -2400.0f};
    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        chassis_power->config.models[i].k4 = 1.0f;
        motor_output[i] = demands[i];
        speed_feedback_aps[i] = feedback_aps[i];
    }

    chassis_power->telemetry_count = CHASSIS_POWER_RTT_DIVIDER - 1U;
    ApplyChassisPowerGroup();
    const char expected[] =
        "123,400,400,400,400,5400,400,390,390,10,0,200,15,2,1,6,15"
#if CHASSIS_POWER_CALIBRATION_ENABLE
        ",1,-1,1,-1,100,-200,300,-400,117"
#endif
        "\r\n";
    ASSERT_STREQ(expected, captured_data);
}

static void AddVehicleMotors(void)
{
    static DJIMotorInstance extra[5];
    static DaemonInstance daemons[5];
    static CANInstance can[5];
    const uint8_t ids[5]={1,2,3,4,7};
    memset(extra,0,sizeof extra);memset(can,0,sizeof can);
    for(unsigned j=0;j<5;++j) {
        extra[j].motor_type=j<2?GM6020:(j==4?M2006:M3508);
        CAN_Init_Config_s config={.can_handle=&hcan2,.tx_id=ids[j]};
        MotorSenderGrouping(&extra[j],&config);
        can[j].can_handle=&hcan2;can[j].rx_id=config.rx_id;
        extra[j].motor_can_instance=&can[j];
        daemons[j].online=true;extra[j].daemon=&daemons[j];
        extra[j].measure.rx_tick_ms=test_tick;
        extra[j].measure.rx_sequence=1;
        extra[j].motor_controller.pid_ref=100+j;
        extra[j].stop_flag=MOTOR_STOP;
        dji_motor_instance[idx++]=&extra[j];
    }
    const unsigned groups[5]={3,3,4,4,3},slots[5]={0,1,2,3,2};
    for(unsigned j=0;j<5;++j) {
        ASSERT_TRUE(extra[j].sender_group==groups[j]);
        ASSERT_TRUE(extra[j].message_num==slots[j]);
        ASSERT_TRUE(sender_assignment[groups[j]].can_handle==&hcan2);
    }
}

static void TestNineMotorPowerAndStopIsolation(void)
{
    PreparePowerGroup();AddVehicleMotors();
    for(unsigned i=4;i<9;++i) motor_output[i]=100+i;
    ApplyChassisPowerGroup();
    for(unsigned i=4;i<9;++i) ASSERT_TRUE(motor_output[i]==100+i);
    safety_drive=false;DJIMotorControl();
    for(unsigned group=0;group<6;++group)
        for(unsigned byte=0;byte<8;++byte) ASSERT_TRUE(sender_assignment[group].tx_buff[byte]==0);
    safety_drive=true;
    dji_motor_instance[8]->stop_flag=MOTOR_ENALBED;
    dji_motor_instance[8]->measure.rx_tick_ms=test_tick-21;
    DJIMotorControl();
    ASSERT_TRUE(sender_assignment[3].tx_buff[4]==0 && sender_assignment[3].tx_buff[5]==0);
    /* One moving non-chassis motor blocks a full-vehicle Flash save. */
    for(unsigned t=1000;t<=1100;t+=2) {
        for(unsigned i=0;i<9;++i) {
            cycle_stop[i]=MOTOR_STOP;cycle_online[i]=true;
            cycle_measure[i].rx_tick_ms=t;cycle_measure[i].speed_aps=0;
            dji_motor_instance[i]->stop_flag=MOTOR_STOP;
        }
        UpdateTuningStopState(t,true);
    }
    ASSERT_TRUE(DJIMotorCanSaveParameters(1100));
    cycle_measure[8].speed_aps=301;UpdateTuningStopState(1100,true);
    ASSERT_TRUE(!DJIMotorCanSaveParameters(1100));
    PreparePowerGroup();AddVehicleMotors();
    DJIMotorInstance *original[9];memcpy(original,dji_motor_instance,sizeof original);
    for(unsigned i=0;i<5;++i) dji_motor_instance[i]=original[i+4];
    for(unsigned i=0;i<4;++i) {dji_motor_instance[i+5]=original[i];adapter_group.motor_indices[i]=i+5;}
    ReadMotorCycle();
    for(unsigned i=0;i<9;++i) motor_output[i]=100+i;
    ApplyChassisPowerGroup();
    for(unsigned i=0;i<5;++i) ASSERT_TRUE(motor_output[i]==100+i);
}

static void TestAutotuneIsolationAndExplicitZeroForceRearm(void)
{
    PreparePowerGroup();
    AddVehicleMotors();
    AutotuneCancel();
    PIDAutotuneFeedback f={.now_ms=123,.permitted=true,.feedback_ok=true,.can_ok=true};
    AutotuneDrive unused;AutotuneMotorStep(&f,&unused);
    adapter_control.tick_ms=123;
    adapter_control.autotune_permitted=true;
    for(unsigned w=0;w<4;++w) {
        dji_motor_instance[w]->measure.rx_tick_ms=123;
        PIDInstance *pid=&dji_motor_instance[w]->motor_controller.speed_PID;
        pid->Kp=.75f;pid->MaxOut=2000;pid->IntegralLimit=1000;pid->DWT_CNT=7;
    }
    RuntimeTuningCancel();RegisterPIDTuning();
    AutotuneSubmit("@A1,1,BEGIN,1");ReadMotorCycle();ApplyAutotuneCycle();
    char reply[512];ASSERT_TRUE(AutotuneReply(reply,sizeof reply)>0);AutotuneReplySent();
    ASSERT_TRUE(autotune_owned);
    AutotuneSubmit("@A1,2,TRIAL,1,2,500,500,3000,3f800000,3f000000,00000000");
    ReadMotorCycle();ApplyAutotuneCycle();AutotuneReply(reply,sizeof reply);AutotuneReplySent();
    ASSERT_TRUE(dji_motor_instance[2]->motor_controller.speed_PID.Kp==1);
    ASSERT_TRUE(dji_motor_instance[2]->motor_controller.speed_PID.MaxOut==2000);
    ASSERT_TRUE(dji_motor_instance[2]->motor_controller.speed_PID.DWT_CNT==1234);
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(cycle_stop[w]==(w==2?MOTOR_ENALBED:MOTOR_STOP));
    for(unsigned i=4;i<9;++i) ASSERT_TRUE(cycle_stop[i]==MOTOR_STOP && cycle_reference[i]==0);
    AutotuneCancel();ReadMotorCycle();ApplyAutotuneCycle();
    ASSERT_TRUE(!autotune_owned && autotune_inhibit);
    ASSERT_TRUE(dji_motor_instance[2]->motor_controller.speed_PID.Kp==.75f);
    adapter_control.control_enabled=false;
    ReadMotorCycle();ApplyAutotuneCycle();ASSERT_TRUE(autotune_inhibit);
    adapter_control.control_enabled=true;
    ReadMotorCycle();ApplyAutotuneCycle();
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(cycle_stop[w]==MOTOR_STOP);
    adapter_control.zero_force=true;adapter_control.control_enabled=false;
    ReadMotorCycle();ApplyAutotuneCycle();ASSERT_TRUE(!autotune_inhibit);
    adapter_control.zero_force=false;adapter_control.control_enabled=true;
    AutotuneSubmit("@A1,3,BEGIN,1");DJIMotorControl();
    ASSERT_TRUE(AutotuneReply(reply,sizeof reply)>0);AutotuneReplySent();
    adapter_control.autotune_permitted=false;
    DJIMotorControl();DJIMotorControl();
    AutotuneSubmit("@A1,4,STATUS,2");DJIMotorControl();
    ASSERT_TRUE(AutotuneReply(reply,sizeof reply)>0);
    ASSERT_TRUE(strstr(reply,"OK,2,aborted,0,0,guard")!=NULL);
    AutotuneReplySent();
    adapter_control.zero_force=true;adapter_control.control_enabled=false;
    DJIMotorControl();
    adapter_control.zero_force=false;adapter_control.control_enabled=true;
    adapter_control.autotune_permitted=true;
    AutotuneSubmit("@A1,5,BEGIN,1");DJIMotorControl();AutotuneReply(reply,sizeof reply);AutotuneReplySent();
    AutotuneSubmit("@A1,6,TRIAL,3,0,500,500,3000,3f800000,00000000,00000000");
    DJIMotorControl();AutotuneReply(reply,sizeof reply);AutotuneReplySent();
    test_tick=1124;adapter_control.tick_ms=test_tick;
    for(unsigned w=0;w<4;++w)dji_motor_instance[w]->measure.rx_tick_ms=test_tick;
    AutotuneSubmit("@A1,7,BEGIN,1");DJIMotorControl();
    ASSERT_TRUE(!autotune_owned && autotune_inhibit);
    ASSERT_TRUE(dji_motor_instance[0]->motor_controller.speed_PID.MaxOut==2000);
    ASSERT_TRUE(AutotuneReply(reply,sizeof reply)>0 && strstr(reply,"ERR,guard"));AutotuneReplySent();
    test_tick=123;adapter_control.tick_ms=test_tick;
    adapter_control.zero_force=true;adapter_control.control_enabled=false;DJIMotorControl();
}

static void TestBaselineScenarioStream(bool record)
{
    /* The same sequence is replayed after moving policy and limiting ownership. */
    static const struct { const char *name; unsigned cycles; float rpm, target, power, current;
                          bool online, enabled; } cases[] = {
        {"arm", 60, 0, 0, 0, 0, true, true},
        {"startup", 1, 0, 1000, 0, 5000, true, true},
        {"drive", 20, 600, 1000, 19, 5000, true, true},
        {"brake", 4, 600, -1000, 19, -5000, true, true},
        {"overpower", 80, -600, -1000, 30, -5000, true, true},
        {"recovery", 15, -600, -1000, 10, -5000, true, true},
        {"offline", 15, -600, -1000, 10, -5000, false, true},
        {"tuning-exit", 1, 0, 0, 0, 0, true, false},
    };
    if ((CHASSIS_POWER_CONTROL_STAGE != 0 && CHASSIS_POWER_CONTROL_STAGE != 3) || !POWER_METER_ENABLE) return;
    PreparePowerGroup();
    test_delta_s = .002f;
    chassis_power->config.algorithm.max_current_command = 15000;
    char filename[64];
    snprintf(filename, sizeof filename, "baseline-stage%d.csv", CHASSIS_POWER_CONTROL_STAGE);
    FILE *fixture = fopen(filename, record ? "wb" : "rb");
    ASSERT_TRUE(fixture != NULL);
    if (fixture == NULL) return;
    test_tick = 123;
    for (unsigned n = 0; n < sizeof cases / sizeof cases[0]; ++n) {
      for (unsigned cycle = 0; cycle < cases[n].cycles; ++cycle) {
        test_tick += 2;
        ChassisPowerControlSnapshot_s control = adapter_control;
        control.version = n + 1; control.tick_ms = test_tick;
        control.nominal_budget_w = control.legacy_budget_w = 20;
        control.target_power_w = 19;
        control.control_enabled = cases[n].enabled;
        for (unsigned w = 0; w < 4; ++w) control.target_rpm[w] = cases[n].target;
        DJIPowerAdapterPublish(&control);
        SetMeter(cases[n].power, cases[n].online, test_tick);
        for (unsigned w = 0; w < 4; ++w) {
            chassis_power->config.models[w].k4 = .000001f;
            dji_motor_instance[w]->measure.speed_aps = cases[n].rpm * 6;
            dji_motor_instance[w]->measure.rx_tick_ms = test_tick;
            ++dji_motor_instance[w]->measure.rx_sequence;
            speed_reference_aps[w] = cases[n].target * 6;
            speed_feedback_aps[w] = cases[n].rpm * 6;
            motor_output[w] = cases[n].current;
        }
        ReadMotorCycle(); ApplyChassisPowerGroup();
      }
        const ChassisPowerSnapshot_s *state = &chassis_power->state;
        if (n == 1) ASSERT_TRUE(state->transient.mode == CHASSIS_POWER_TRANSIENT_STARTUP);
        if (n == 4) {
            ASSERT_TRUE(state->transient.negative_learning_allowed);
            ASSERT_TRUE(fabsf(state->power.limited_current[0]) < 5000);
        }
        if (n == 5) ASSERT_TRUE(state->transient.mode == CHASSIS_POWER_TRANSIENT_RECOVERY);
        char row[256], expected[256];
        snprintf(row, sizeof row, "%s,%lu,%.4f,%d,%d,%d,%d,%u,%u,%u\n", cases[n].name,
            (unsigned long)state->control_version, chassis_power->input.referee_power_limit_w,
            (int)state->power.limited_current[0], (int)state->power.limited_current[1],
            (int)state->power.limited_current[2], (int)state->power.limited_current[3],
            state->transient.fault, state->transient.mode, state->transient.meter_fresh);
        if (record) fputs(row, fixture);
        else { ASSERT_TRUE(fgets(expected, sizeof expected, fixture) != NULL);
            if(strcmp(expected,row)!=0) printf("expected %sactual %s",expected,row);
            ASSERT_STREQ(expected, row); }
    }
    if (!record) ASSERT_TRUE(fgetc(fixture) == EOF);
    fclose(fixture);
    ASSERT_TRUE(chassis_power->transient.startup_seen);
    test_tick = 123;
    test_delta_s = .001f;
}

static int16_t direct_recorded[DJI_MOTOR_CNT];
static bool direct_validate_ok;
static void DirectPrepare(void *context,const DJIMotorCycle_s *cycle)
{
    (void)context;
    for(unsigned w=0;w<4;++w) {
        ASSERT_TRUE(cycle->actual_aps[w]==600);
        cycle->current[w]=(w%2 ? -1 : 1)*(100.0f+w);
    }
}
static void DirectLimit(void *context,const DJIMotorCycle_s *cycle) {(void)context;(void)cycle;}
static bool DirectValidate(void *context,const DJIMotorCycle_s *cycle,const int16_t *commands,const bool *enabled,uint32_t now)
{(void)context;(void)cycle;(void)commands;(void)enabled;(void)now;return direct_validate_ok && safety_drive;}
static void DirectRecord(void *context,const DJIMotorCycle_s *cycle,const int16_t *commands,
    const bool *enabled,uint8_t submitted,uint8_t success)
{(void)context;(void)cycle;(void)enabled;(void)submitted;(void)success;memcpy(direct_recorded,commands,sizeof direct_recorded);}
static void TestDirectCurrentOwnership(void)
{
    PreparePowerGroup();
    adapter_group.owns_raw_current=true;direct_validate_ok=true;
    output_hooks=(DJIMotorOutputHooks_s){.read_control=AdapterReadControl,.prepare_current=DirectPrepare,
        .limit=DirectLimit,.validate_current=DirectValidate,.record_final=DirectRecord};
    for(unsigned w=0;w<4;++w) {
        dji_motor_instance[w]->measure.speed_aps=600;
        dji_motor_instance[w]->motor_settings.close_loop_type=SPEED_LOOP;
        dji_motor_instance[w]->motor_settings.outer_loop_type=SPEED_LOOP;
        dji_motor_instance[w]->motor_settings.motor_reverse_flag=MOTOR_DIRECTION_REVERSE;
    }
    static DJIMotorInstance other;static DaemonInstance daemon;
    daemon.online=1;other=*dji_motor_instance[0];other.daemon=&daemon;
    other.motor_settings.motor_reverse_flag=MOTOR_DIRECTION_NORMAL;
    other.motor_controller.pid_ref=50;other.sender_group=0;other.message_num=0;
    dji_motor_instance[4]=&other;idx=5;sender_enable_flag[1]=1;
    test_pid_calls=0;
    DJIMotorControl();
    ASSERT_TRUE(test_pid_calls==1);ASSERT_TRUE(direct_recorded[4]==50);
    for(unsigned w=0;w<4;++w) {
        ASSERT_TRUE(direct_recorded[w]==(w%2 ? -1 : 1)*(100+(int)w));
        const uint16_t packed=(uint16_t)sender_assignment[1].tx_buff[w*2]<<8|sender_assignment[1].tx_buff[w*2+1];
        ASSERT_TRUE((int16_t)packed==direct_recorded[w]);
    }
    ASSERT_TRUE(!autotune_owned);
    direct_validate_ok=false;DJIMotorControl();
    for(unsigned w=0;w<4;++w) ASSERT_TRUE(direct_recorded[w]==0);
    ASSERT_TRUE(direct_recorded[4]==50);
    direct_validate_ok=true;stop_in_can_wait=true;DJIMotorControl();
    for(unsigned w=0;w<4;++w) {
        ASSERT_TRUE(direct_recorded[w]==0);
        ASSERT_TRUE(sender_assignment[1].tx_buff[w*2]==0 && sender_assignment[1].tx_buff[w*2+1]==0);
    }
    PreparePowerGroup();
}

int main(int argc, char **argv)
{
    TestBaselineScenarioStream(argc > 1 && strcmp(argv[1], "--record-baseline") == 0);
    PreparePowerGroup();
    for (unsigned tick=1000;tick<=1100;++tick) {
        for (unsigned i=0;i<idx;++i) {
            cycle_stop[i]=MOTOR_STOP; cycle_online[i]=true;
            cycle_measure[i].rx_tick_ms=tick; cycle_measure[i].speed_aps=0;
            dji_motor_instance[i]->stop_flag=MOTOR_STOP;
        }
        UpdateTuningStopState(tick,true);
        ASSERT_TRUE(DJIMotorCanSaveParameters(tick)==(tick>=1100));
    }
    ASSERT_TRUE(!DJIMotorCanSaveParameters(1106)); /* Stale publication. */
    dji_motor_instance[0]->stop_flag=MOTOR_ENALBED;
    ASSERT_TRUE(!DJIMotorCanSaveParameters(1101)); /* Enable raced with snapshot. */
    dji_motor_instance[0]->stop_flag=MOTOR_STOP;
    UpdateTuningStopState(1101,false);
    ASSERT_TRUE(!DJIMotorCanSaveParameters(1101)); /* Zero CAN frame failed. */
    cycle_measure[0].speed_aps=1000;
    UpdateTuningStopState(1102,true);
    ASSERT_TRUE(!DJIMotorCanSaveParameters(1102)); /* Coasting motor. */
    ASSERT_TRUE(!tuning_stop_seen);
    cycle_measure[0].speed_aps=0; cycle_online[0]=false;
    UpdateTuningStopState(1103,true);
    ASSERT_TRUE(!DJIMotorCanSaveParameters(1103));
    ASSERT_TRUE(!tuning_stop_seen);
    cycle_online[0]=true;cycle_measure[0].rx_tick_ms=1000;
    UpdateTuningStopState(1104,true);
    ASSERT_TRUE(!tuning_stop_seen); /* Per-motor feedback age is independent of task age. */
    /* Live PID writes apply in the motor cycle and reject a bad whole group. */
    PreparePowerGroup();
    chassis_power->config.algorithm.max_current_command = 15000;
    RegisterPIDTuning();
    char tuning_set[512] = "@T1,1,SET,pid,1";
    const float wheel[] = {1.25f, .1f, 0, 12000, 2000, 1};
    for (unsigned i = 0; i < 24; ++i) {
        uint32_t bits; memcpy(&bits, &wheel[i % 6], sizeof bits);
        char cell[16]; snprintf(cell, sizeof cell, ",%08lx", (unsigned long)bits);
        strcat(tuning_set, cell);
    }
    RuntimeTuningSubmit(tuning_set);
    DJIMotorControl();
    for (unsigned i = 0; i < 4; ++i) {
        ASSERT_TRUE(dji_motor_instance[i]->motor_controller.speed_PID.Kp == 1.25f);
        ASSERT_TRUE(dji_motor_instance[i]->motor_controller.speed_PID.MaxOut == 12000);
    }
    char tuning_reply[512];
    ASSERT_TRUE(RuntimeTuningReply(tuning_reply, sizeof tuning_reply) > 0);
    ASSERT_TRUE(strstr(tuning_reply, "@T1,1,OK,pid,2") != NULL);
    RuntimeTuningReplySent();
    /* Direct consumer validation also guarantees all-or-nothing updates. */
    float invalid[24];
    for (unsigned i = 0; i < 24; ++i) invalid[i] = wheel[i % 6];
    invalid[21] = 16000;
    ASSERT_TRUE(!ApplyPIDTuning(invalid));
    ASSERT_TRUE(dji_motor_instance[0]->motor_controller.speed_PID.MaxOut == 12000);
    RuntimeTuningCancel();
    PreparePowerGroup();
    chassis_power->config.algorithm.max_current_command = 15000;
    adapter_control.power_ceiling_w = 4;
    SetMeter(0, true, 123);
    for (unsigned w = 0; w < 4; ++w) {
        chassis_power->config.models[w].k4 = 0.0001f;
        motor_output[w] = 1000;
        speed_reference_aps[w] = speed_feedback_aps[w] = 6000;
    }
    ApplyChassisPowerGroup();
    float ceiling_power = 0;
    for (unsigned w = 0; w < 4; ++w)
        ceiling_power += PowerModelPredict(&chassis_power->config.models[w],
            chassis_power->state.power.limited_current[w], 1000);
    ASSERT_TRUE(ceiling_power <= 4.0001f);
    TestZeroDemandIsNotActualBaseLimiting();
    TestFinalGateAndCanResultAreRecorded();
    TestAdapterRejectsInvalidMembership();
    TestTargetSnapshotDoesNotDependOnFeedbackRounding();
    TestFastProtectionObservesOrAppliesByStage();
    TestStopPublicationDoesNotMixCanAndDiagnostics();
    TestAtomicPublicationAndFinalIntegerCommands();
    TestAutotuneIsolationAndExplicitZeroForceRearm();
    TestNineMotorPowerAndStopIsolation();
    TestDirectCurrentOwnership();
    if (CHASSIS_POWER_CONTROL_STAGE == 0 && POWER_METER_ENABLE) {
        TestControlAndMeasurementSnapshotsReachOneRttRow();
        TestOfflineMeasurementKeepsValueAndTick();
        TestNonfiniteMeasurementBecomesZero();
        TestNullControlSnapshotLeavesPreviousValue();
        TestRttDividerRemainsTenMotorCycles();
        TestCalibrationUsesOutputCommandsAndActualRpm();
        TestCalibrationPrintsLimitedCurrentsInsteadOfPidDemand();
    }

    CAN_HandleTypeDef unknown_bus={0};
    Motor_Init_Config_s invalid_motor={.motor_type=M3508,
        .can_init_config={.can_handle=&unknown_bus,.tx_id=1}};
    ASSERT_TRUE(DJIMotorInit(&invalid_motor)==NULL);

    if (failures != 0)
        return 1;

    puts("PASS: DJI chassis-power RTT integration tests");
    return 0;
}
