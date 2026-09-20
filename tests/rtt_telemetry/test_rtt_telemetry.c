#include "rtt_telemetry.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;
static unsigned captured_length;
static char captured_data[512];

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

static void ResetCapture(void)
{
    captured_length = 0U;
    memset(captured_data, 0, sizeof(captured_data));
}

unsigned BSPLogWrite(const void *data, unsigned length)
{
    if (length >= sizeof(captured_data))
        return 0U;
    memcpy(captured_data, data, length);
    captured_data[length] = '\0';
    captured_length = length;
    return length;
}

unsigned BSPLogWriteString(const char *text)
{
    return BSPLogWrite(text, (unsigned)strlen(text));
}

static void TestHeaderIsWrittenExactly(void)
{
    ResetCapture();
    RTTTelemetryWriteChassisPowerHeader();

    ASSERT_STREQ(
        "tick,i_lf,i_rf,i_lb,i_rb,rpm_lf,rpm_rf,rpm_lb,rpm_rb,"
        "target_lf,target_rf,target_lb,target_rb,p_lf_cw,p_rf_cw,"
        "p_lb_cw,p_rb_cw,p_total_cw,p_cap_cw\r\n",
        captured_data);
    ASSERT_TRUE(captured_length == strlen(captured_data));
}

static void TestRepresentativeRowPreservesAllFields(void)
{
    ChassisPowerTelemetry_s telemetry = {
        .current_command = {2000.0f, 2100.0f, 2200.0f, 2300.0f},
        .actual_speed_rpm = {1000.0f, 1100.0f, 1200.0f, 1300.0f},
        .target_speed_rpm = {1010.0f, 1111.0f, 1212.0f, 1313.0f},
        .predicted_power_w = {10.0f, 11.0f, 12.0f, 13.0f},
        .predicted_total_power_w = 46.0f,
    };
    char row[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 123U, 12.34f, row, sizeof(row)) > 0U);
    ASSERT_STREQ(
        "123,2000,2100,2200,2300,1000,1100,1200,1300,"
        "1010,1111,1212,1313,1000,1100,1200,1300,4600,1234\r\n",
        row);
}

static void TestNegativeMeasuredPowerPreservesSign(void)
{
    const ChassisPowerTelemetry_s telemetry = {0};
    char row[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 1U, -1.25f, row, sizeof(row)) > 0U);
    ASSERT_STREQ("1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-125\r\n", row);
}

static void TestNonfiniteMeasuredPowerBecomesZero(void)
{
    const ChassisPowerTelemetry_s telemetry = {0};
    char row[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 1U, NAN, row, sizeof(row)) > 0U);
    ASSERT_STREQ("1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\r\n", row);
}

static void TestMeasuredPowerSaturates(void)
{
    const ChassisPowerTelemetry_s telemetry = {0};
    char row[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 1U, FLT_MAX, row, sizeof(row)) > 0U);
    ASSERT_STREQ(
        "1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2147483647\r\n",
        row);
}

static void TestPredictedPowerSaturates(void)
{
    ChassisPowerTelemetry_s telemetry = {0};
    char row[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];
    telemetry.predicted_power_w[0] = FLT_MAX;

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 1U, 0.0f, row, sizeof(row)) > 0U);
    ASSERT_STREQ(
        "1,0,0,0,0,0,0,0,0,0,0,0,0,2147483647,0,0,0,0,0\r\n",
        row);
}

static void TestInvalidFormattingInputsAreRejected(void)
{
    const ChassisPowerTelemetry_s telemetry = {0};
    char row[8] = "dirty";

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    NULL, 1U, 0.0f, row, sizeof(row)) == 0U);
    ASSERT_TRUE(row[0] == '\0');

    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 1U, 0.0f, NULL, sizeof(row)) == 0U);

    row[0] = 'x';
    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 1U, 0.0f, row, 0U) == 0U);
    ASSERT_TRUE(row[0] == 'x');

    strcpy(row, "dirty");
    ASSERT_TRUE(RTTTelemetryFormatChassisPowerCsv(
                    &telemetry, 123U, 0.0f, row, sizeof(row)) == 0U);
    ASSERT_TRUE(row[0] == '\0');
}

static void TestRowWriterSendsOneCompleteRow(void)
{
    const ChassisPowerTelemetry_s telemetry = {0};

    ResetCapture();
    RTTTelemetryWriteChassisPowerRow(&telemetry, 7U, 1.5f);

    ASSERT_STREQ("7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,150\r\n", captured_data);
    ASSERT_TRUE(captured_length == strlen(captured_data));
}

int main(void)
{
    TestHeaderIsWrittenExactly();
    TestRepresentativeRowPreservesAllFields();
    TestNegativeMeasuredPowerPreservesSign();
    TestNonfiniteMeasuredPowerBecomesZero();
    TestMeasuredPowerSaturates();
    TestPredictedPowerSaturates();
    TestInvalidFormattingInputsAreRejected();
    TestRowWriterSendsOneCompleteRow();

    if (failures != 0)
        return 1;

    puts("PASS: RTT telemetry tests");
    return 0;
}
