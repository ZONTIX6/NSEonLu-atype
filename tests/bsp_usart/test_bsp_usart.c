#include "bsp_usart.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static HAL_StatusTypeDef start_result = HAL_OK, abort_result = HAL_OK;
static USARTInstance *device;
static unsigned callbacks;
static uint8_t received[256];
static uint16_t received_length;

/* Model the local HAL contract: a stopped stream does not clear BUSY_RX. */
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *handle,
                                             uint8_t *buffer, uint16_t size)
{
    if (handle->RxState != HAL_UART_STATE_READY) return HAL_BUSY;
    if (start_result != HAL_OK) return start_result;
    handle->RxState = HAL_UART_STATE_BUSY_RX;
    /* UART_Start_Receive_DMA ignores HAL_DMA_Start_IT's HAL_BUSY return. */
    if (handle->hdmarx->State == HAL_DMA_STATE_READY)
    {
        handle->hdmarx->running = 1;
        handle->hdmarx->State = HAL_DMA_STATE_BUSY;
    }
    handle->rx_dma_request = 1;
    handle->rx_buffer = buffer;
    handle->rx_size = size;
    handle->ErrorCode = 0U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *handle)
{
    if (handle->rx_dma_request)
    {
        /* Real AbortReceive clears DMAR even when DMA abort later times out. */
        handle->rx_dma_request = 0;
        if (abort_result != HAL_OK)
        {
            handle->hdmarx->State = HAL_DMA_STATE_TIMEOUT;
            return abort_result;
        }
        handle->hdmarx->State = HAL_DMA_STATE_READY;
    }
    handle->RxState = HAL_UART_STATE_READY;
    handle->hdmarx->running = 0;
    /* RX recovery must leave an active UI transmission alone. */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef *dma)
{
    dma->running = 0;
    dma->State = HAL_DMA_STATE_READY;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, uint8_t *p,
                                  uint16_t n, uint32_t timeout)
{ (void)h; (void)p; (void)n; (void)timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t n)
{ (void)h; (void)p; (void)n; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *h, uint8_t *p, uint16_t n)
{ (void)h; (void)p; (void)n; return HAL_OK; }
uint32_t HAL_GetTick(void) { return 123U; }

static void Receive(void)
{
    ++callbacks;
    received_length = device->recv_data_len;
    memcpy(received, device->recv_buff, received_length);
}

static void Deliver(UART_HandleTypeDef *handle, const uint8_t *data, uint16_t length)
{
    assert(handle->hdmarx->running);
    assert(length <= handle->rx_size);
    memcpy(handle->rx_buffer, data, length);
    handle->hdmarx->running = 0;
    handle->hdmarx->State = HAL_DMA_STATE_READY;
    handle->rx_dma_request = 0;
    handle->RxState = HAL_UART_STATE_READY;
    HAL_UARTEx_RxEventCallback(handle, length);
}

int main(void)
{
    DMA_HandleTypeDef dma = {.State = HAL_DMA_STATE_READY};
    UART_HandleTypeDef uart = {.hdmarx = &dma, .RxState = HAL_UART_STATE_READY,
                               .gState = HAL_UART_STATE_READY};
    USART_Init_Config_s config = {.recv_buff_size = 255U,
        .usart_handle = &uart, .module_callback = Receive};
    device = USARTRegister(&config);
    assert(dma.running);
    assert(dma.disabled_interrupts & DMA_IT_HT);

    /* Reproduce the lost-link recovery failure before touching production. */
    dma.running = 0;
    uart.gState = HAL_UART_STATE_BUSY_TX;
    USARTServiceInit(device);
    assert(dma.running && "lost-link recovery must restart a stalled BUSY_RX DMA");
    assert(uart.gState == HAL_UART_STATE_BUSY_TX);
    const uint8_t frame[] = {0xA5U, 0x03U, 0x00U, 0x04U, 0x55U};
    Deliver(&uart, frame, sizeof(frame));
    assert(callbacks == 1U && received_length == sizeof(frame));
    assert(memcmp(received, frame, sizeof(frame)) == 0);
    assert(dma.running && device->recv_data_len == 0U);

    /* A failed abort must not arm reception into an unreset HAL state. */
    dma.running = 0;
    abort_result = HAL_TIMEOUT;
    USARTServiceInit(device);
    assert(!dma.running);
    abort_result = HAL_OK;
    USARTServiceInit(device);
    assert(dma.running && "retry must reset DMA left TIMEOUT after DMAR was cleared");

    /* HAL's error callback may run with a stale BUSY_RX after a DMA error. */
    dma.running = 0;
    uart.ErrorCode = HAL_UART_ERROR_ORE;
    HAL_UART_ErrorCallback(&uart);
    assert(dma.running);
    Deliver(&uart, frame, sizeof(frame));
    assert(callbacks == 2U);

    /* Diagnostics must distinguish no bytes, HAL failures, and protocol errors. */
    assert(device->stats.rx_event_count == 2U);
    assert(device->stats.rx_byte_count == 10U);
    assert(device->stats.last_rx_tick_ms == 123U);
    assert(device->stats.error_count == 1U);
    assert(device->stats.last_error_code == HAL_UART_ERROR_ORE);
    assert(device->stats.restart_count == 3U);
    assert(device->stats.start_failure_count == 1U);
    assert(device->stats.last_start_status == HAL_OK);
    dma.running = 0;
    uart.RxState = HAL_UART_STATE_READY;
    start_result = HAL_ERROR;
    USARTServiceInit(device);
    assert(device->stats.start_failure_count == 2U);
    assert(device->stats.last_start_status == HAL_ERROR);
    assert(!dma.running);
    start_result = HAL_OK;
    USARTServiceInit(device);
    assert(dma.running && device->stats.last_start_status == HAL_OK);
    puts("PASS: UART stalled DMA, abort failure and error recovery");
    return 0;
}
