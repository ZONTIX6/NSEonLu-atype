#include "rtt_telemetry.h"

#include "bsp_log.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>

static const char chassis_power_header[] =
    "tick,i_lf,i_rf,i_lb,i_rb,rpm_lf,rpm_rf,rpm_lb,rpm_rb,"
    "target_lf,target_rf,target_lb,target_rb,p_lf_cw,p_rf_cw,"
    "p_lb_cw,p_rb_cw,p_total_cw,p_cap_cw\r\n";

static char chassis_power_row[RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH];

static int32_t RoundTelemetryValue(float value)
{
    if (!isfinite(value))
        return 0;
    if (value >= (float)INT32_MAX)
        return INT32_MAX;
    if (value <= (float)INT32_MIN)
        return INT32_MIN;
    return (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static int32_t PowerToCentiwatts(float power_w)
{
    if (!isfinite(power_w))
        return 0;
    if (power_w >= (float)INT32_MAX / 100.0f)
        return INT32_MAX;
    if (power_w <= (float)INT32_MIN / 100.0f)
        return INT32_MIN;
    return RoundTelemetryValue(power_w * 100.0f);
}

size_t RTTTelemetryFormatChassisPowerCsv(
    const ChassisPowerTelemetry_s *telemetry,
    uint32_t tick_ms,
    float measured_chassis_power_w,
    char *buffer,
    size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0U)
        return 0U;
    buffer[0] = '\0';
    if (telemetry == NULL)
        return 0U;

    const int written = snprintf(
        buffer,
        buffer_size,
        "%lu,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
        (unsigned long)tick_ms,
        (long)RoundTelemetryValue(telemetry->current_command[0]),
        (long)RoundTelemetryValue(telemetry->current_command[1]),
        (long)RoundTelemetryValue(telemetry->current_command[2]),
        (long)RoundTelemetryValue(telemetry->current_command[3]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[0]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[1]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[2]),
        (long)RoundTelemetryValue(telemetry->actual_speed_rpm[3]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[0]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[1]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[2]),
        (long)RoundTelemetryValue(telemetry->target_speed_rpm[3]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[0]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[1]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[2]),
        (long)PowerToCentiwatts(telemetry->predicted_power_w[3]),
        (long)PowerToCentiwatts(telemetry->predicted_total_power_w),
        (long)PowerToCentiwatts(measured_chassis_power_w));

    if (written < 0 || (size_t)written >= buffer_size)
    {
        buffer[0] = '\0';
        return 0U;
    }
    return (size_t)written;
}

void RTTTelemetryWriteChassisPowerHeader(void)
{
    (void)BSPLogWriteString(chassis_power_header);
}

void RTTTelemetryWriteChassisPowerRow(
    const ChassisPowerTelemetry_s *telemetry,
    uint32_t tick_ms,
    float measured_chassis_power_w)
{
    const size_t length = RTTTelemetryFormatChassisPowerCsv(
        telemetry,
        tick_ms,
        measured_chassis_power_w,
        chassis_power_row,
        sizeof(chassis_power_row));
    if (length > 0U)
        (void)BSPLogWrite(chassis_power_row, (unsigned)length);
}
