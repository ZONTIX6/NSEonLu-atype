#ifndef IMU_CAPTURE_CONFIG_H
#define IMU_CAPTURE_CONFIG_H

/* Dedicated second A-board: sensor/temperature service and diagnostic transport. */
#ifndef IMU_CAPTURE_ONLY
#define IMU_CAPTURE_ONLY 0
#endif
#if IMU_CAPTURE_ONLY != 0 && IMU_CAPTURE_ONLY != 1
#error IMU_CAPTURE_ONLY must be 0 or 1.
#endif
#ifndef IMU_CAPTURE_CAN_TX_ENABLE
#define IMU_CAPTURE_CAN_TX_ENABLE 0
#endif
#ifndef CHASSIS_IMU_CAN_RX_ENABLE
#define CHASSIS_IMU_CAN_RX_ENABLE 0
#endif
#if IMU_CAPTURE_CAN_TX_ENABLE != 0 && IMU_CAPTURE_CAN_TX_ENABLE != 1
#error IMU_CAPTURE_CAN_TX_ENABLE must be 0 or 1.
#endif
#if CHASSIS_IMU_CAN_RX_ENABLE != 0 && CHASSIS_IMU_CAN_RX_ENABLE != 1
#error CHASSIS_IMU_CAN_RX_ENABLE must be 0 or 1.
#endif
#if IMU_CAPTURE_CAN_TX_ENABLE && !IMU_CAPTURE_ONLY
#error IMU_CAPTURE_CAN_TX_ENABLE requires IMU_CAPTURE_ONLY=1.
#endif
#if CHASSIS_IMU_CAN_RX_ENABLE && IMU_CAPTURE_ONLY
#error CHASSIS_IMU_CAN_RX_ENABLE requires the normal robot application.
#endif

#endif
