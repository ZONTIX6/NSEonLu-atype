#ifndef TEST_VTX_MAIN_H
#define TEST_VTX_MAIN_H
#include <stdint.h>
typedef struct {struct {uint32_t FIFOMode;} Init;} DMA_HandleTypeDef;
typedef struct {DMA_HandleTypeDef *hdmarx;uint16_t RxXferSize,RxXferCount;} UART_HandleTypeDef;
#define DMA_FIFOMODE_DISABLE 0U
int HAL_DMA_Init(DMA_HandleTypeDef *dma);
uint32_t HAL_GetTick(void);
#endif
