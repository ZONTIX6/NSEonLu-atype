#ifndef VOFA_TELEMETRY_H
#define VOFA_TELEMETRY_H

#include "board_config.h"
#include "usb_telemetry_format.h"

/* Call from the telemetry task with the latest freshness-checked sample. */
#if VOFA_TELEMETRY_MODE == 1
#include "chassis_power_types.h"
void VofaTelemetrySendMeasurement(const USBTelemetrySample_s *sample,
                                  const ChassisPowerSnapshot_s *cycle, uint32_t now_ms);
#else
void VofaTelemetrySend(const USBTelemetrySample_s *sample);
#endif

#endif
