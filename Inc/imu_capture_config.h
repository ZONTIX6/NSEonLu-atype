#ifndef IMU_CAPTURE_CONFIG_H
#define IMU_CAPTURE_CONFIG_H

/* Dedicated second A-board: sensor/temperature service and UART telemetry only. */
#ifndef IMU_CAPTURE_ONLY
#define IMU_CAPTURE_ONLY 0
#endif
#if IMU_CAPTURE_ONLY != 0 && IMU_CAPTURE_ONLY != 1
#error IMU_CAPTURE_ONLY must be 0 or 1.
#endif

#endif
