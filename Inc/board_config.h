#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* USART1 belongs exclusively to DBUS on the RoboMaster A board. */
#ifdef UART1_USE
#error UART1_USE is retired: USART1 is DBUS; use REFEREE_ENABLE and LOG_OUTPUT.
#endif
#ifndef REFEREE_ENABLE
#define REFEREE_ENABLE 1
#endif
#ifndef REFEREE_UART_PORT
#define REFEREE_UART_PORT 6 /* A-board connector 16, leftmost socket marked 6. */
#endif
#ifndef LOG_OUTPUT
#define LOG_OUTPUT 1 /* 0: off, 1: RTT, 2: USART2, 3: both */
#endif
#ifndef ATYPE_BENCH_MODE
#define ATYPE_BENCH_MODE 0
#endif
#if REFEREE_ENABLE != 0 && REFEREE_ENABLE != 1
#error REFEREE_ENABLE must be 0 or 1.
#endif
#if REFEREE_UART_PORT != 6 && REFEREE_UART_PORT != 7
#error REFEREE_UART_PORT must be 6 (USART6) or 7 (UART7); other UARTs are reserved.
#endif
#if LOG_OUTPUT < 0 || LOG_OUTPUT > 3
#error LOG_OUTPUT must be 0, 1, 2, or 3.
#endif
#if ATYPE_BENCH_MODE != 0 && ATYPE_BENCH_MODE != 1
#error ATYPE_BENCH_MODE must be 0 or 1.
#endif
#define BOARD_LOG_UART_ENABLED (LOG_OUTPUT == 2 || LOG_OUTPUT == 3)
#define BOARD_LOG_RTT_ENABLED (LOG_OUTPUT == 1 || LOG_OUTPUT == 3)

#define BOARD_REMOTE_UART (&huart1)
#define BOARD_VISION_UART (&huart3)
#define BOARD_VTX_UART (&huart8)
#if REFEREE_UART_PORT == 7
#define BOARD_REFEREE_UART (&huart7)
#else
#define BOARD_REFEREE_UART (&huart6)
#endif
#define BOARD_LOG_UART (&huart2)
#define BOARD_SUPERCAP_CAN (&hcan1)
#define BOARD_POWER_METER_CAN (&hcan1)

#ifndef OLED_USE_I2C
#define OLED_USE_I2C 0 /* A-board SPI1 OLED; I2C2 backend remains optional. */
#endif
#if OLED_USE_I2C != 0 && OLED_USE_I2C != 1
#error OLED_USE_I2C must be 0 or 1.
#endif

/* Optional devices register only when selected. One point-to-point device per UART. */
#ifndef BOARD_AUX_UART2_DEVICE
#define BOARD_AUX_UART2_DEVICE 0
#endif
#ifndef BOARD_AUX_UART7_DEVICE
#define BOARD_AUX_UART7_DEVICE 0
#endif
#ifndef BOARD_OLED_ENABLE
#define BOARD_OLED_ENABLE 0
#endif
#ifndef BOARD_KEY_ENABLE
#define BOARD_KEY_ENABLE 0
#endif
#if BOARD_AUX_UART2_DEVICE < 0 || BOARD_AUX_UART2_DEVICE > 3 || BOARD_AUX_UART7_DEVICE < 0 || BOARD_AUX_UART7_DEVICE > 3
#error Auxiliary UART device must be 0 (none), 1 (TFmini), 2 (HC05), or 3 (bus servo).
#endif
#if BOARD_AUX_UART2_DEVICE && BOARD_LOG_UART_ENABLED
#error USART2 cannot be shared by an auxiliary device and UART logging.
#endif
#if REFEREE_ENABLE && REFEREE_UART_PORT == 7 && BOARD_AUX_UART7_DEVICE
#error UART7 cannot be shared by the referee system and an auxiliary device.
#endif
#if BOARD_AUX_UART2_DEVICE && BOARD_AUX_UART7_DEVICE && BOARD_AUX_UART2_DEVICE == BOARD_AUX_UART7_DEVICE
#error TFmini, HC05 and bus servo each support one UART instance.
#endif
#if BOARD_OLED_ENABLE < 0 || BOARD_OLED_ENABLE > 1 || BOARD_KEY_ENABLE < 0 || BOARD_KEY_ENABLE > 1
#error Optional OLED/key enables must be 0 or 1.
#endif

#endif
