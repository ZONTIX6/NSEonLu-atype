#ifndef RTT_TELEMETRY_H
#define RTT_TELEMETRY_H

#include "power_model.h"

#include <stddef.h>
#include <stdint.h>

#define RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH 256U

size_t RTTTelemetryFormatChassisPowerCsv(
    const ChassisPowerTelemetry_s *telemetry,
    uint32_t tick_ms,
    float measured_chassis_power_w,
    char *buffer,
    size_t buffer_size);

void RTTTelemetryWriteChassisPowerHeader(void);

void RTTTelemetryWriteChassisPowerRow(
    const ChassisPowerTelemetry_s *telemetry,
    uint32_t tick_ms,
    float measured_chassis_power_w);

#endif // RTT_TELEMETRY_H
