#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../modules/power_meter/power_meter.h"

#include "../../modules/motor/DJImotor/dji_motor.c"

static int failures;
static char captured_data[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];
static PowerMeterStatus_s test_meter;
static bool publish_stop_during_pid;

static void SetMeter(float power, bool online, uint32_t tick)
{
    test_meter = (PowerMeterStatus_s){.power_w = power, .online = online, .last_rx_tick_ms = tick};
}
static void PublishTelemetry(const ChassisPowerControlTelemetry_s *telemetry)
{
    if (telemetry == NULL) return;
    DJIChassisPowerControlSnapshot_s control = chassis_power->control;
    control.legacy_budget_w = chassis_power->input.referee_power_limit_w;
    control.telemetry = *telemetry;
    DJIChassisPowerPublishControl(&control);
}

bool PowerMeterGetStatus(PowerMeterStatus_s *status)
{
    *status = test_meter;
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

CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    (void)config;
    return NULL;
}

uint8_t CANTransmit(CANInstance *instance, float timeout)
{
    (void)instance;
    (void)timeout;
    return 1U;
}

uint32_t HAL_GetTick(void)
{
    return 123U;
}

void PIDInit(PIDInstance *pid, PID_Init_Config_s *config)
{
    (void)pid;
    (void)config;
}

float PIDCalculate(PIDInstance *pid, float measure, float ref)
{
    (void)pid;
    (void)measure;
    if (publish_stop_during_pid) {
        publish_stop_during_pid = false;
        DJIChassisPowerControlSnapshot_s next = chassis_power->control;
        ++next.version; next.control_enabled = false;
        DJIChassisPowerPublishControl(&next);
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
    (void)count;
    return 0.001f;
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
    static DJIChassisPowerInstance_s power_group;
    static DJIMotorInstance motors[POWER_MODEL_MOTOR_COUNT];
    static DaemonInstance daemons[POWER_MODEL_MOTOR_COUNT];

    memset(&power_group, 0, sizeof(power_group));
    memset(motors, 0, sizeof(motors));
    memset(daemons, 0, sizeof(daemons));
    memset(dji_motor_instance, 0, sizeof(dji_motor_instance));
    memset(motor_output, 0, sizeof(motor_output));
    memset(speed_reference_aps, 0, sizeof(speed_reference_aps));
    memset(speed_feedback_aps, 0, sizeof(speed_feedback_aps));
    memset(captured_data, 0, sizeof(captured_data));
    memset(&test_meter, 0, sizeof(test_meter));
    idx = 4;

    power_group.registered = true;
    power_group.input.referee_power_limit_w = 100.0f;
    power_group.control = (DJIChassisPowerControlSnapshot_s){
        .nominal_budget_w = 100, .target_power_w = 95,
        .legacy_budget_w = 100, .control_enabled = true};
    power_group.input.attenuation = 1.0f;
    power_group.algorithm = (ChassisPowerAlgorithmConfig_s){
        .safety_factor = 1.0f,
        .small_error_threshold_rpm = 0.0f,
        .reserved_power_threshold_w = 4.0f,
        .per_motor_reserved_power_w = 1.0f,
        .max_current_command = 100.0f,
    };

    for (size_t i = 0; i < POWER_MODEL_MOTOR_COUNT; ++i)
    {
        power_group.motor_indices[i] = (uint8_t)i;
        power_group.models[i].current_conversion = 1.0f;
        daemons[i].online = 1U;
        motors[i].daemon = &daemons[i];
        motors[i].stop_flag = MOTOR_ENALBED;
        motors[i].sender_group = 1;
        motors[i].message_num = (uint8_t)i;
        dji_motor_instance[i] = &motors[i];
        motor_output[i] = (float)(i + 1U);
    }

    chassis_power = &power_group;
    chassis_power_rtt_count = 0U;
}

static void EmitOneRow(void)
{
    for (size_t i = 0; i < CHASSIS_POWER_RTT_DIVIDER; ++i)
        ApplyChassisPowerGroup();
}

static void TestAtomicPublicationAndFinalIntegerCommands(void)
{
    PreparePowerGroup();
    const DJIChassisPowerControlSnapshot_s control = {
        .version = 42, .tick_ms = 120, .nominal_budget_w = 20,
        .target_power_w = 19, .legacy_budget_w = 20, .control_enabled = true,
        .target_rpm = {1000, -1000, 500, -500},
    };
    DJIChassisPowerPublishControl(&control);
    motor_output[0] = 3.9f;
    ApplyChassisPowerGroup();
    DJIChassisPowerState_s state;
    ASSERT_TRUE(DJIChassisPowerGetSnapshot(&state));
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
        chassis_power->models[w].k0 = 30;
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
    DJIChassisPowerControlSnapshot_s control = chassis_power->control;
    control.version = 10;
    for (unsigned i = 0; i < 4; ++i) {
        control.target_rpm[i] = 1000;
        dji_motor_instance[i]->motor_settings.close_loop_type = SPEED_LOOP;
        dji_motor_instance[i]->motor_settings.outer_loop_type = SPEED_LOOP;
    }
    DJIChassisPowerPublishControl(&control);
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
    DJIChassisPowerControlSnapshot_s control = chassis_power->control;
    control.nominal_budget_w = control.legacy_budget_w = 20;
    control.target_power_w = 19; control.slow_correction_w = 3;
    DJIChassisPowerPublishControl(&control);
    for (unsigned i = 0; i < 4; ++i) {
        chassis_power->models[i].k4 = .01f;
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
        chassis_power->models[i].k4 = 1.0f;
        motor_output[i] = demands[i];
        speed_feedback_aps[i] = feedback_aps[i];
    }

    chassis_power_rtt_count = CHASSIS_POWER_RTT_DIVIDER - 1U;
    ApplyChassisPowerGroup();
    const char expected[] =
        "123,400,400,400,400,5400,400,390,390,10,0,200,15,2,1,6,15"
#if CHASSIS_POWER_CALIBRATION_ENABLE
        ",1,-1,1,-1,100,-200,300,-400,117"
#endif
        "\r\n";
    ASSERT_STREQ(expected, captured_data);
}

int main(void)
{
    TestZeroDemandIsNotActualBaseLimiting();
    TestTargetSnapshotDoesNotDependOnFeedbackRounding();
    TestFastProtectionObservesOrAppliesByStage();
    TestStopPublicationDoesNotMixCanAndDiagnostics();
    TestAtomicPublicationAndFinalIntegerCommands();
    if (CHASSIS_POWER_CONTROL_STAGE == 0 && POWER_METER_ENABLE) {
        TestControlAndMeasurementSnapshotsReachOneRttRow();
        TestOfflineMeasurementKeepsValueAndTick();
        TestNonfiniteMeasurementBecomesZero();
        TestNullControlSnapshotLeavesPreviousValue();
        TestRttDividerRemainsTenMotorCycles();
        TestCalibrationUsesOutputCommandsAndActualRpm();
        TestCalibrationPrintsLimitedCurrentsInsteadOfPidDemand();
    }

    if (failures != 0)
        return 1;

    puts("PASS: DJI chassis-power RTT integration tests");
    return 0;
}
