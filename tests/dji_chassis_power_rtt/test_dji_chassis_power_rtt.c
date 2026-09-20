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

static void TestMeasurementTupleReachesOneRttRow(void)
{
    PreparePowerGroup();
    DJIChassisPowerSetMeasuredPower(12.34f, true, 77U);

    EmitOneRow();

    ASSERT_STREQ(
        "123,1,2,3,4,0,0,0,0,0,0,0,0,0,0,0,0,0,1234,1,77,15\r\n",
        captured_data);
}

static void TestOfflineMeasurementKeepsValueAndTick(void)
{
    PreparePowerGroup();
    DJIChassisPowerSetMeasuredPower(9.0f, false, 88U);

    EmitOneRow();

    ASSERT_STREQ(
        "123,1,2,3,4,0,0,0,0,0,0,0,0,0,0,0,0,0,900,0,88,15\r\n",
        captured_data);
}

static void TestNonfiniteMeasurementBecomesZero(void)
{
    PreparePowerGroup();
    DJIChassisPowerSetMeasuredPower(NAN, true, 99U);

    EmitOneRow();

    ASSERT_STREQ(
        "123,1,2,3,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,99,15\r\n",
        captured_data);
}

int main(void)
{
    TestMeasurementTupleReachesOneRttRow();
    TestOfflineMeasurementKeepsValueAndTick();
    TestNonfiniteMeasurementBecomesZero();

    if (failures != 0)
        return 1;

    puts("PASS: DJI chassis-power RTT integration tests");
    return 0;
}
