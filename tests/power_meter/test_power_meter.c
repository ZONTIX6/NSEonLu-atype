#include "power_meter.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;
static CAN_Init_Config_s captured_can_config;
static Daemon_Init_Config_s captured_daemon_config;
static CANInstance fake_can_instance;
static DaemonInstance fake_daemon_instance;
static uint8_t captured_dlc;
static uint32_t fake_tick_ms;
static uint32_t daemon_reload_calls;

#define ASSERT_TRUE(value)                                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(value))                                                                              \
        {                                                                                          \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #value);                              \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)
#define ASSERT_FALSE(value) ASSERT_TRUE(!(value))
#define ASSERT_EQ_U32(expected, actual)                                                             \
    do                                                                                             \
    {                                                                                              \
        if ((uint32_t)(expected) != (uint32_t)(actual))                                            \
        {                                                                                          \
            printf("FAIL %s:%d: expected %lu, got %lu\n", __FILE__, __LINE__,                    \
                   (unsigned long)(uint32_t)(expected), (unsigned long)(uint32_t)(actual));         \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)
#define ASSERT_NEAR(expected, actual, tolerance)                                                    \
    do                                                                                             \
    {                                                                                              \
        if (fabsf((expected) - (actual)) > (tolerance))                                            \
        {                                                                                          \
            printf("FAIL %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__,                  \
                   (double)(expected), (double)(actual));                                          \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)

CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    captured_can_config = *config;
    memset(&fake_can_instance, 0, sizeof(fake_can_instance));
    fake_can_instance.can_handle = config->can_handle;
    fake_can_instance.tx_id = config->tx_id;
    fake_can_instance.rx_id = config->rx_id;
    fake_can_instance.can_module_callback = config->can_module_callback;
    fake_can_instance.id = config->id;
    return &fake_can_instance;
}

void CANSetDLC(CANInstance *instance, uint8_t length)
{
    (void)instance;
    captured_dlc = length;
}

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{
    captured_daemon_config = *config;
    fake_daemon_instance.reload_count = config->reload_count;
    fake_daemon_instance.temp_count = config->init_count;
    fake_daemon_instance.callback = config->callback;
    fake_daemon_instance.owner_id = config->owner_id;
    return &fake_daemon_instance;
}

void DaemonReload(DaemonInstance *instance)
{
    instance->temp_count = instance->reload_count;
    ++daemon_reload_calls;
}

uint8_t DaemonIsOnline(DaemonInstance *instance)
{
    return instance->temp_count > 0U;
}

uint32_t HAL_GetTick(void)
{
    return fake_tick_ms;
}

static void ResetFakes(void)
{
    memset(&captured_can_config, 0, sizeof(captured_can_config));
    memset(&captured_daemon_config, 0, sizeof(captured_daemon_config));
    memset(&fake_can_instance, 0, sizeof(fake_can_instance));
    memset(&fake_daemon_instance, 0, sizeof(fake_daemon_instance));
    captured_dlc = 0U;
    fake_tick_ms = 0U;
    daemon_reload_calls = 0U;
}

static PowerMeterInstance *InitMeter(void)
{
    static CAN_HandleTypeDef can1;
    ResetFakes();
    const PowerMeter_Init_Config_s config = {
        .can_config = {
            .can_handle = &can1,
            .tx_id = POWER_METER_STATUS_CAN_ID,
            .rx_id = POWER_METER_STATUS_CAN_ID,
        },
        .offline_reload_count = 10U,
    };
    return PowerMeterInit(&config);
}

static void DeliverFrame(const uint8_t frame[8], uint8_t dlc)
{
    memcpy(fake_can_instance.rx_buff, frame, sizeof(fake_can_instance.rx_buff));
    fake_can_instance.rx_len = dlc;
    fake_can_instance.can_module_callback(&fake_can_instance);
}

static void TestInitRegistersFrameworkInstances(void)
{
    ResetFakes();
    CAN_HandleTypeDef can1 = {0};
    const PowerMeter_Init_Config_s config = {
        .can_config = {
            .can_handle = &can1,
            .tx_id = POWER_METER_STATUS_CAN_ID,
            .rx_id = POWER_METER_STATUS_CAN_ID,
        },
        .offline_reload_count = 10U,
    };

    PowerMeterInstance *meter = PowerMeterInit(&config);
    ASSERT_TRUE(meter != NULL);
    ASSERT_TRUE(captured_can_config.can_handle == &can1);
    ASSERT_EQ_U32(POWER_METER_STATUS_CAN_ID, captured_can_config.rx_id);
    ASSERT_TRUE(captured_can_config.can_module_callback != NULL);
    ASSERT_TRUE(captured_can_config.id == meter);
    ASSERT_EQ_U32(10U, captured_daemon_config.reload_count);
    ASSERT_TRUE(captured_daemon_config.owner_id == meter);
    ASSERT_EQ_U32(POWER_METER_CAN_DLC, captured_dlc);
}

static void TestStartsOfflineAndRejectsNullArguments(void)
{
    PowerMeterStatus_s status = {0};
    PowerMeterInstance *meter = InitMeter();

    ASSERT_TRUE(PowerMeterGetStatus(meter, &status));
    ASSERT_FALSE(status.online);
    ASSERT_FALSE(PowerMeterGetStatus(NULL, &status));
    ASSERT_FALSE(PowerMeterGetStatus(meter, NULL));
    ASSERT_TRUE(PowerMeterInit(NULL) == NULL);
}

static void TestValidFrameUpdatesSnapshotAndDiagnostics(void)
{
    static const uint8_t frame[8] = {0x00U, 0x0CU, 0x5FU, 0xB4U,
                                     0x0CU, 0xB2U, 0x1FU, 0x1BU};
    PowerMeterStatus_s status = {0};
    PowerMeterInstance *meter = InitMeter();
    fake_tick_ms = 1234U;

    DeliverFrame(frame, 8U);

    ASSERT_TRUE(PowerMeterGetStatus(meter, &status));
    ASSERT_TRUE(status.online);
    ASSERT_NEAR(0.012f, status.shunt_voltage_v, 0.00001f);
    ASSERT_NEAR(24.5f, status.bus_voltage_v, 0.00001f);
    ASSERT_NEAR(3.25f, status.current_a, 0.00001f);
    ASSERT_NEAR(79.63f, status.power_w, 0.00001f);
    ASSERT_EQ_U32(1U, meter->received_frame_count);
    ASSERT_EQ_U32(0U, meter->rejected_frame_count);
    ASSERT_EQ_U32(1234U, meter->last_rx_tick_ms);
    ASSERT_EQ_U32(1U, daemon_reload_calls);
}

static void TestBadLengthDoesNotOverwriteOrFeedDaemon(void)
{
    static const uint8_t valid[8] = {0, 1, 0, 2, 0, 3, 0, 4};
    static const uint8_t invalid[8] = {0, 9, 0, 9, 0, 9, 0, 9};
    PowerMeterStatus_s status_before = {0};
    PowerMeterStatus_s status_after = {0};
    PowerMeterInstance *meter = InitMeter();
    fake_tick_ms = 4321U;
    DeliverFrame(valid, 8U);

    ASSERT_TRUE(PowerMeterGetStatus(meter, &status_before));
    const uint32_t received_before = meter->received_frame_count;
    const uint32_t rejected_before = meter->rejected_frame_count;
    const uint32_t last_rx_before = meter->last_rx_tick_ms;
    const uint32_t reloads_before = daemon_reload_calls;

    fake_tick_ms = 9999U;
    DeliverFrame(invalid, 7U);

    ASSERT_TRUE(PowerMeterGetStatus(meter, &status_after));
    ASSERT_EQ_U32(status_before.online, status_after.online);
    ASSERT_NEAR(status_before.shunt_voltage_v, status_after.shunt_voltage_v, 0.00001f);
    ASSERT_NEAR(status_before.bus_voltage_v, status_after.bus_voltage_v, 0.00001f);
    ASSERT_NEAR(status_before.current_a, status_after.current_a, 0.00001f);
    ASSERT_NEAR(status_before.power_w, status_after.power_w, 0.00001f);
    ASSERT_EQ_U32(received_before, meter->received_frame_count);
    ASSERT_EQ_U32(rejected_before + 1U, meter->rejected_frame_count);
    ASSERT_EQ_U32(last_rx_before, meter->last_rx_tick_ms);
    ASSERT_EQ_U32(reloads_before, daemon_reload_calls);
}

static void TestOfflineCallbackClearsOnlyOnlineState(void)
{
    static const uint8_t frame[8] = {0, 1, 0, 2, 0, 3, 0, 4};
    PowerMeterStatus_s status = {0};
    PowerMeterInstance *meter = InitMeter();
    DeliverFrame(frame, 8U);

    fake_daemon_instance.temp_count = 0U;
    captured_daemon_config.callback(captured_daemon_config.owner_id);

    ASSERT_TRUE(PowerMeterGetStatus(meter, &status));
    ASSERT_FALSE(status.online);
    ASSERT_NEAR(0.04f, status.power_w, 0.00001f);
}

static void TestStaleOfflineCallbackKeepsReloadedMeterOnline(void)
{
    static const uint8_t frame[8] = {0, 1, 0, 2, 0, 3, 0, 4};
    PowerMeterStatus_s status = {0};
    PowerMeterInstance *meter = InitMeter();

    fake_daemon_instance.temp_count = 0U;
    DeliverFrame(frame, 8U);
    captured_daemon_config.callback(captured_daemon_config.owner_id);

    ASSERT_TRUE(PowerMeterGetStatus(meter, &status));
    ASSERT_TRUE(status.online);
}

static void TestZeroReloadUsesDefault(void)
{
    ResetFakes();
    CAN_HandleTypeDef can1 = {0};
    const PowerMeter_Init_Config_s config = {
        .can_config = {
            .can_handle = &can1,
            .tx_id = POWER_METER_STATUS_CAN_ID,
            .rx_id = POWER_METER_STATUS_CAN_ID,
        },
        .offline_reload_count = 0U,
    };

    ASSERT_TRUE(PowerMeterInit(&config) != NULL);
    ASSERT_EQ_U32(10U, captured_daemon_config.reload_count);
    ASSERT_EQ_U32(10U, captured_daemon_config.init_count);
}

int main(void)
{
    TestInitRegistersFrameworkInstances();
    TestStartsOfflineAndRejectsNullArguments();
    TestValidFrameUpdatesSnapshotAndDiagnostics();
    TestBadLengthDoesNotOverwriteOrFeedDaemon();
    TestOfflineCallbackClearsOnlyOnlineState();
    TestStaleOfflineCallbackKeepsReloadedMeterOnline();
    TestZeroReloadUsesDefault();

    if (failures != 0)
        return 1;
    puts("PASS: power meter module tests");
    return 0;
}
