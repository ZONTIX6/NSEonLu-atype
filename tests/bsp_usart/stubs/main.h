#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <stdint.h>

typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct {
    uint32_t FIFOMode, Mode, PeriphDataAlignment, MemDataAlignment;
} DMA_InitTypeDef;
typedef struct {
    DMA_InitTypeDef Init;
    uint32_t disabled_interrupts;
    uint32_t State;
    int running;
} DMA_HandleTypeDef;
typedef struct {
    DMA_HandleTypeDef *hdmarx;
    uint32_t RxState, gState, ErrorCode;
    uint8_t *rx_buffer;
    uint16_t rx_size;
    int rx_dma_request;
} UART_HandleTypeDef;

#define HAL_UART_STATE_READY 0x20U
#define HAL_UART_STATE_BUSY_RX 0x22U
#define HAL_UART_STATE_BUSY_TX 0x21U
#define HAL_UART_ERROR_ORE 8U
#define DMA_FIFOMODE_DISABLE 0U
#define DMA_NORMAL 0U
#define DMA_PDATAALIGN_BYTE 0U
#define DMA_MDATAALIGN_BYTE 0U
#define DMA_IT_HT 0x10U
#define HAL_DMA_STATE_READY 1U
#define HAL_DMA_STATE_BUSY 2U
#define HAL_DMA_STATE_TIMEOUT 3U
#define __HAL_DMA_DISABLE_IT(h, it) ((h)->disabled_interrupts |= (it))

HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *);
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *, uint8_t *, uint16_t);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *, uint16_t);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *);
uint32_t HAL_GetTick(void);

#endif
