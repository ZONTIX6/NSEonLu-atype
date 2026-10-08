#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../../modules/motor/DJImotor/dji_motor.c"

static int failures;
static char captured_data[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];

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
    ASSERT_TRUE(strncmp(expected, captured_data, prefix_length) == 0);
    ASSERT_TRUE(captured_data[prefix_length] == ',');
#else
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

    power_group.registered = true;
    power_group.input.referee_power_limit_w = 100.0f;
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
    DJIChassisPowerSetControlTelemetry(&control);
    DJIChassisPowerSetMeasuredPower(12.34f, true, 77U);

    EmitOneRow();

    AssertControlColumnsMatch(
        "123,3000,2850,2700,10000,0,0,1234,2900,-50,-300,"
        "1000,0,2,1,46,15\r\n");
}

static void TestOfflineMeasurementKeepsValueAndTick(void)
{
    PreparePowerGroup();
    DJIChassisPowerSetMeasuredPower(9.0f, false, 88U);

    EmitOneRow();

    AssertControlColumnsMatch(
        "123,0,0,0,10000,0,0,900,0,0,0,1000,0,0,0,35,15\r\n");
}

static void TestNonfiniteMeasurementBecomesZero(void)
{
    PreparePowerGroup();
    DJIChassisPowerSetMeasuredPower(NAN, true, 99U);

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
    DJIChassisPowerSetControlTelemetry(&control);
    DJIChassisPowerSetControlTelemetry(NULL);

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
    DJIChassisPowerSetMeasuredPower(18.0f, true, 117U);
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
    DJIChassisPowerSetControlTelemetry(&control);
    DJIChassisPowerSetMeasuredPower(3.9f, true, 117U);
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
    TestControlAndMeasurementSnapshotsReachOneRttRow();
    TestOfflineMeasurementKeepsValueAndTick();
    TestNonfiniteMeasurementBecomesZero();
    TestNullControlSnapshotLeavesPreviousValue();
    TestRttDividerRemainsTenMotorCycles();
    TestCalibrationUsesOutputCommandsAndActualRpm();
    TestCalibrationPrintsLimitedCurrentsInsteadOfPidDemand();

    if (failures != 0)
        return 1;

    puts("PASS: DJI chassis-power RTT integration tests");
    return 0;
}
