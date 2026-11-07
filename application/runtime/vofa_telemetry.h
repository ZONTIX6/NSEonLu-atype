#ifndef VOFA_TELEMETRY_H
#define VOFA_TELEMETRY_H

#include "usb_telemetry_format.h"

/* Call from the telemetry task with the latest freshness-checked sample. */
void VofaTelemetrySend(const USBTelemetrySample_s *sample);

#endif
