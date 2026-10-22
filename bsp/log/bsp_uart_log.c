#include "board_config.h"
/**
 * @file bsp_uart_log.c
 * @brief Non-blocking USART1 DMA transport for the BSP log module.
 */

#include "bsp_uart_log.h"

#include "main.h"
#include "usart.h"

#include <string.h>

/* USART2 is reserved for logging only when LOG_OUTPUT selects UART transport. */
#define BSP_UART_LOG_RING_SIZE 4096U

static uint8_t tx_ring[BSP_UART_LOG_RING_SIZE];
static uint16_t tx_head;
static uint16_t tx_tail;
static uint16_t tx_queued;
static uint16_t tx_dma_length;
static BSPUARTLogDiagnostics_s diagnostics;

static uint16_t FreeBytes(void)
{
    return (uint16_t)(BSP_UART_LOG_RING_SIZE - 1U - tx_queued);
}

static void StartNextDmaSegment(void)
{
    uint16_t length;
    HAL_StatusTypeDef status;

    if (tx_dma_length != 0U || tx_queued == 0U)
        return;

    length = (uint16_t)(BSP_UART_LOG_RING_SIZE - tx_tail);
    if (length > tx_queued)
        length = tx_queued;

    status = HAL_UART_Transmit_IT(BOARD_LOG_UART, &tx_ring[tx_tail], length);
    if (status != HAL_OK)
    {
        ++diagnostics.tx_errors;
        return;
    }
    tx_dma_length = length;
}

void BSPUARTLogInit(void)
{
    tx_head = 0U;
    tx_tail = 0U;
    tx_queued = 0U;
    tx_dma_length = 0U;
    diagnostics.dropped_messages = 0U;
    diagnostics.tx_errors = 0U;

    BOARD_LOG_UART->Init.BaudRate = 115200U;
    BOARD_LOG_UART->Init.WordLength = UART_WORDLENGTH_8B;
    BOARD_LOG_UART->Init.StopBits = UART_STOPBITS_1;
    BOARD_LOG_UART->Init.Parity = UART_PARITY_NONE;
    BOARD_LOG_UART->Init.Mode = UART_MODE_TX_RX;
    BOARD_LOG_UART->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    BOARD_LOG_UART->Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(BOARD_LOG_UART) != HAL_OK)
        ++diagnostics.tx_errors;
}

unsigned BSPUARTLogWrite(const void *data, unsigned length)
{
    uint32_t primask;
    unsigned first_length;

    if (data == NULL || length == 0U || length >= BSP_UART_LOG_RING_SIZE)
    {
        if (length >= BSP_UART_LOG_RING_SIZE)
            ++diagnostics.dropped_messages;
        return 0U;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    if (length > FreeBytes())
    {
        /* A failed DMA start may have filled the retained queue. Retry it
         * even when this message cannot fit, without a capture-task poll. */
        StartNextDmaSegment();
        ++diagnostics.dropped_messages;
        __set_PRIMASK(primask);
        return 0U;
    }

    first_length = BSP_UART_LOG_RING_SIZE - tx_head;
    if (first_length > length)
        first_length = length;
    memcpy(&tx_ring[tx_head], data, first_length);
    if (first_length < length)
        memcpy(tx_ring, (const uint8_t *)data + first_length, length - first_length);
    tx_head = (uint16_t)((tx_head + length) % BSP_UART_LOG_RING_SIZE);
    tx_queued = (uint16_t)(tx_queued + length);
    StartNextDmaSegment();
    __set_PRIMASK(primask);
    return length;
}

void BSPUARTLogGetDiagnostics(BSPUARTLogDiagnostics_s *out)
{
    if (out == NULL)
        return;
    *out = diagnostics;
}

void BSPUARTLogPoll(void)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    StartNextDmaSegment();
    __set_PRIMASK(primask);
}
bool BSPUARTLogIsIdle(void)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    const bool idle = tx_queued == 0U && tx_dma_length == 0U;
    __set_PRIMASK(primask);
    return idle;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t primask;

    if (huart != BOARD_LOG_UART)
        return;

    primask = __get_PRIMASK();
    __disable_irq();
    tx_tail = (uint16_t)((tx_tail + tx_dma_length) % BSP_UART_LOG_RING_SIZE);
    tx_queued = (uint16_t)(tx_queued - tx_dma_length);
    tx_dma_length = 0U;
    StartNextDmaSegment();
    __set_PRIMASK(primask);
}
