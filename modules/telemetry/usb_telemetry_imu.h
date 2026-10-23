#ifndef USB_TELEMETRY_IMU_H
#define USB_TELEMETRY_IMU_H

#include "usb_telemetry_format.h"
#include "ins_task.h"

/* Keep the last published values/age for diagnosis, even when INS is invalid. */
void USBTelemetryEncodeIMU(USBTelemetrySample_s *sample,
                          const INSSnapshot_s *imu, uint32_t now_ms);

#endif
