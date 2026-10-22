#ifndef TEST_SPI_H
#define TEST_SPI_H
#include <stdint.h>
#include <stddef.h>
typedef struct {volatile uint32_t CR1,SR,DR;} SPI_TypeDef;
typedef struct {unsigned unused;} GPIO_TypeDef;
typedef enum {GPIO_PIN_RESET,GPIO_PIN_SET} GPIO_PinState;
typedef enum {HAL_OK,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct {uint32_t DataSize,Direction;} SPI_InitTypeDef;
typedef struct {SPI_TypeDef *Instance;SPI_InitTypeDef Init;uint32_t State,ErrorCode,Lock;} SPI_HandleTypeDef;
extern SPI_TypeDef test_spi1,test_spi5;
#define SPI1 (&test_spi1)
#define SPI5 (&test_spi5)
#define SPI_DATASIZE_8BIT 0U
#define SPI_DIRECTION_2LINES 0U
#define HAL_SPI_STATE_READY 1U
#define HAL_SPI_STATE_BUSY_TX_RX 2U
#define HAL_SPI_ERROR_NONE 0U
#define HAL_SPI_ERROR_FLAG 1U
#define HAL_SPI_ERROR_MODF 2U
#define HAL_SPI_ERROR_OVR 4U
#define HAL_SPI_ERROR_CRC 8U
#define HAL_UNLOCKED 0U
#define HAL_LOCKED 1U
#define SPI_FLAG_TXE 2U
#define SPI_FLAG_RXNE 1U
#define SPI_FLAG_BSY 128U
#define SPI_FLAG_OVR 64U
#define SPI_FLAG_MODF 32U
#define SPI_FLAG_CRCERR 16U
#define SPI_CR1_SPE 64U
#define __HAL_SPI_ENABLE(h) ((h)->Instance->CR1 |= SPI_CR1_SPE)
#define __HAL_SPI_DISABLE(h) ((h)->Instance->CR1 &= ~SPI_CR1_SPE)
#define __HAL_SPI_CLEAR_OVRFLAG(h) ((h)->Instance->SR &= ~SPI_FLAG_OVR)
#define __HAL_SPI_CLEAR_MODFFLAG(h) ((h)->Instance->SR &= ~SPI_FLAG_MODF)
#define __HAL_SPI_CLEAR_CRCERRFLAG(h) ((h)->Instance->SR &= ~SPI_FLAG_CRCERR)
#define __get_PRIMASK() 0U
#define __disable_irq() ((void)0)
#define __set_PRIMASK(x) ((void)(x))
void HAL_GPIO_WritePin(GPIO_TypeDef *port,uint16_t pin,GPIO_PinState state);
#define DECLARE_SPI(name) HAL_StatusTypeDef name(SPI_HandleTypeDef *h,uint8_t *data,uint16_t n)
DECLARE_SPI(HAL_SPI_Transmit_DMA);DECLARE_SPI(HAL_SPI_Transmit_IT);
DECLARE_SPI(HAL_SPI_Receive_DMA);DECLARE_SPI(HAL_SPI_Receive_IT);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef *,uint8_t *,uint8_t *,uint16_t);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *,uint8_t *,uint8_t *,uint16_t);
#endif
