#include "super_cap.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;
static CAN_Init_Config_s captured_can_config;
static Daemon_Init_Config_s captured_daemon_config;
static CANInstance fake_can_instance;
static DaemonInstance fake_daemon_instance;
static uint8_t captured_dlc;
static uint8_t transmit_result = 1U;
static uint32_t transmit_calls;
static uint32_t daemon_reload_calls;
static uint32_t fake_tick_ms;
static SuperCapInstance *instance;

#define ASSERT_TRUE(value)                                                                     \
    do                                                                                         \
    {                                                                                          \
        if (!(value))                                                                          \
        {                                                                                      \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #value);                         \
            ++failures;                                                                        \
        }                                                                                      \
    } while (0)
#define ASSERT_FALSE(value) ASSERT_TRUE(!(value))
#define ASSERT_EQ_U32(expected, actual)                                                         \
    do                                                                                         \
    {                                                                                          \
        if ((uint32_t)(expected) != (uint32_t)(actual))                                        \
        {                                                                                      \
            printf("FAIL %s:%d: expected %lu, got %lu\n", __FILE__, __LINE__,               \
                   (unsigned long)(uint32_t)(expected), (unsigned long)(uint32_t)(actual));     \
            ++failures;                                                                        \
        }                                                                                      \
    } while (0)
#define ASSERT_NEAR(expected, actual, tolerance)                                                \
    do                                                                                         \
    {                                                                                          \
        if (fabsf((expected) - (actual)) > (tolerance))                                        \
        {                                                                                      \
            printf("FAIL %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__,              \
                   (double)(expected), (double)(actual));                                      \
            ++failures;                                                                        \
        }                                                                                      \
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

void CANSetDLC(CANInstance *can_instance, uint8_t length)
{
    (void)can_instance;
    captured_dlc = length;
}

uint8_t CANTransmit(CANInstance *can_instance, float timeout_ms)
{
    (void)can_instance;
    (void)timeout_ms;
    ++transmit_calls;
    return transmit_result;
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

void DaemonReload(DaemonInstance *daemon)
{
    daemon->temp_count = daemon->reload_count;
    ++daemon_reload_calls;
}

uint8_t DaemonIsOnline(DaemonInstance *daemon)
{
    return daemon->temp_count > 0U;
}

uint32_t HAL_GetTick(void)
{
    return fake_tick_ms;
}

static void DeliverFrame(const uint8_t frame[8], uint8_t dlc)
{
    memcpy(fake_can_instance.rx_buff, frame, sizeof(fake_can_instance.rx_buff));
    fake_can_instance.rx_len = dlc;
    fake_can_instance.can_module_callback(&fake_can_instance);
}

static void TestUninitializedOperationsFailAndInitRegistersDependencies(void)
{
    static CAN_HandleTypeDef can1;
    const SuperCapCommand_s command = {0};
    SuperCapStatus_s status = {0};
    const SuperCap_Init_Config_s config = {
        .can_config = {
            .can_handle = &can1,
            .tx_id = SUPERCAP_COMMAND_CAN_ID,
            .rx_id = SUPERCAP_STATUS_CAN_ID,
        },
        .offline_reload_count = 20U,
    };

    ASSERT_FALSE(SuperCapGetStatus(&status));
    ASSERT_FALSE(SuperCapSendCommand(&command));
    ASSERT_FALSE(SuperCapInit(NULL));
    ASSERT_TRUE(SuperCapInit(&config));
    instance = (SuperCapInstance *)captured_can_config.id;
    ASSERT_TRUE(instance != NULL);
    ASSERT_FALSE(SuperCapInit(&config));
    ASSERT_TRUE(captured_can_config.can_handle == &can1);
    ASSERT_EQ_U32(SUPERCAP_COMMAND_CAN_ID, captured_can_config.tx_id);
    ASSERT_EQ_U32(SUPERCAP_STATUS_CAN_ID, captured_can_config.rx_id);
    ASSERT_TRUE(captured_can_config.can_module_callback != NULL);
    ASSERT_TRUE(captured_daemon_config.owner_id == instance);
    ASSERT_EQ_U32(20U, captured_daemon_config.reload_count);
    ASSERT_EQ_U32(SUPERCAP_CAN_DLC, captured_dlc);
}

static void TestCommandUsesOwnedInstance(void)
{
    const SuperCapCommand_s command = {
        .enable_dcdc = true,
        .referee_power_limit_w = 100U,
        .referee_buffer_energy_j = 50U,
    };
    static const uint8_t expected[8] = {0x01U, 0x64U, 0x00U, 0x32U,
                                        0x00U, 0x00U, 0x00U, 0x00U};

    transmit_calls = 0U;
    transmit_result = 1U;
    ASSERT_TRUE(SuperCapSendCommand(&command));
    ASSERT_EQ_U32(1U, transmit_calls);
    ASSERT_TRUE(memcmp(expected, fake_can_instance.tx_buff, sizeof(expected)) == 0);
    ASSERT_FALSE(SuperCapSendCommand(NULL));

    transmit_result = 0U;
    ASSERT_FALSE(SuperCapSendCommand(&command));
}

static void TestStatusUsesOwnedInstanceAndOfflineCallback(void)
{
    static const uint8_t frame[8] = {0x00U, 0x00U, 0x00U, 0xF1U,
                                     0x42U, 0xB4U, 0x00U, 0x80U};
    SuperCapStatus_s status = {0};

    daemon_reload_calls = 0U;
    fake_tick_ms = 1234U;
    DeliverFrame(frame, 8U);

    ASSERT_TRUE(SuperCapGetStatus(&status));
    ASSERT_TRUE(status.online);
    ASSERT_TRUE(status.output_enabled);
    ASSERT_EQ_U32(0U, status.error_code);
    ASSERT_NEAR(120.5f, status.chassis_power_w, 0.001f);
    ASSERT_EQ_U32(180U, status.available_power_limit_w);
    ASSERT_EQ_U32(1U, instance->received_frame_count);
    ASSERT_EQ_U32(1234U, instance->last_rx_tick_ms);
    ASSERT_EQ_U32(1U, daemon_reload_calls);
    ASSERT_FALSE(SuperCapGetStatus(NULL));

    fake_daemon_instance.temp_count = 0U;
    captured_daemon_config.callback(captured_daemon_config.owner_id);
    ASSERT_TRUE(SuperCapGetStatus(&status));
    ASSERT_FALSE(status.online);
    ASSERT_NEAR(120.5f, status.chassis_power_w, 0.001f);
}

int main(void)
{
    TestUninitializedOperationsFailAndInitRegistersDependencies();
    TestCommandUsesOwnedInstance();
    TestStatusUsesOwnedInstanceAndOfflineCallback();

    if (failures != 0)
        return 1;
    puts("PASS: super capacitor module tests");
    return 0;
}
