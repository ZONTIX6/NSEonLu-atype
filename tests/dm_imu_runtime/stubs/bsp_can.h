#ifndef TEST_BSP_CAN_H
#define TEST_BSP_CAN_H
#include <stdint.h>
typedef struct { unsigned bus; } CAN_HandleTypeDef;
typedef enum { HAL_OK,HAL_ERROR } HAL_StatusTypeDef;
typedef struct CANInstance CANInstance;
struct CANInstance {
    CAN_HandleTypeDef *can_handle;
    uint32_t tx_id,rx_id,tx_mailbox;
    uint8_t tx_buff[8],rx_buff[8],rx_len,dlc;
    void (*can_module_callback)(CANInstance *);
    void *id;
};
typedef struct {
    CAN_HandleTypeDef *can_handle;
    uint32_t tx_id,rx_id;
    void (*can_module_callback)(CANInstance *);
    void *id;
} CAN_Init_Config_s;
extern CAN_HandleTypeDef hcan1,hcan2;
CANInstance *CANRegister(CAN_Init_Config_s *config);
void CANSetDLC(CANInstance *instance,uint8_t length);
#endif
