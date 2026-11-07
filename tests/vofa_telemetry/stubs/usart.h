#ifndef TEST_VOFA_USART_H
#define TEST_VOFA_USART_H

#include <stdint.h>

typedef enum {
    HAL_OK,
    HAL_ERROR,
    HAL_BUSY
} HAL_StatusTypeDef;

typedef enum {
    HAL_UART_STATE_READY = 0x20U,
    HAL_UART_STATE_BUSY_TX = 0x21U
} HAL_UART_StateTypeDef;

typedef struct {
    volatile HAL_UART_StateTypeDef gState;
} UART_HandleTypeDef;

extern UART_HandleTypeDef huart7;

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
                                       const uint8_t *data,
                                       uint16_t length);

#endif
