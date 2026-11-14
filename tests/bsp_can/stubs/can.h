#ifndef TEST_CAN_H
#define TEST_CAN_H
#include <stdint.h>

typedef enum {HAL_OK,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT} HAL_StatusTypeDef;
typedef enum {HAL_CAN_STATE_RESET,HAL_CAN_STATE_READY,HAL_CAN_STATE_LISTENING,
              HAL_CAN_STATE_ERROR} HAL_CAN_StateTypeDef;
typedef struct {volatile uint32_t MCR,MSR;} CAN_TypeDef;
typedef struct {CAN_TypeDef *Instance;HAL_CAN_StateTypeDef State;uint32_t ErrorCode;} CAN_HandleTypeDef;
typedef struct {uint32_t StdId,ExtId,IDE,RTR,DLC,TransmitGlobalTime;} CAN_TxHeaderTypeDef;
typedef struct {uint32_t StdId,ExtId,IDE,RTR,DLC;} CAN_RxHeaderTypeDef;
typedef struct {
    uint32_t FilterIdHigh,FilterIdLow,FilterMaskIdHigh,FilterMaskIdLow;
    uint32_t FilterFIFOAssignment,FilterBank,FilterMode,FilterScale;
    uint32_t FilterActivation,SlaveStartFilterBank;
} CAN_FilterTypeDef;
extern CAN_HandleTypeDef hcan1,hcan2;

#define CAN_MCR_INRQ 1U
#define CAN_MSR_INAK 1U
#define CAN_TIMEOUT_VALUE 10U
#define HAL_CAN_ERROR_NONE 0U
#define HAL_CAN_ERROR_TIMEOUT 0x00020000U
#define HAL_CAN_ERROR_NOT_INITIALIZED 0x00040000U
#define HAL_CAN_ERROR_NOT_READY 0x00080000U
#define HAL_CAN_ERROR_INTERNAL 0x00800000U
#define CAN_IT_RX_FIFO0_MSG_PENDING 2U
#define CAN_IT_RX_FIFO1_MSG_PENDING 16U
#define CAN_RX_FIFO0 0U
#define CAN_RX_FIFO1 1U
#define CAN_FILTERMODE_IDLIST 1U
#define CAN_FILTERSCALE_16BIT 0U
#define CAN_FILTER_ENABLE 1U
#define CAN_ID_STD 0U
#define CAN_RTR_DATA 0U
#define CLEAR_BIT(REG,BIT) ((REG)&=~(BIT))
#define SET_BIT(REG,BIT) ((REG)|=(BIT))

uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *handle,uint32_t flags);
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *handle,CAN_FilterTypeDef *filter);
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *handle,CAN_TxHeaderTypeDef *header,
                                     uint8_t *data,uint32_t *mailbox);
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *handle,uint32_t fifo);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *handle,uint32_t fifo,
                                     CAN_RxHeaderTypeDef *header,uint8_t *data);
#endif
