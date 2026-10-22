#include "board_optional.h"
#include "board_config.h"
#include "usart.h"
#include "TFminiPlus.h"
#include "HC05.h"
#include "servo_motor.h"
#include "oled.h"
#include "key.h"

#if BOARD_AUX_UART2_DEVICE || BOARD_AUX_UART7_DEVICE
static void AuxiliaryUARTInit(UART_HandleTypeDef *uart, unsigned device)
{
    switch(device) {
    case 1: (void)TFminiPlusInit(uart);break;
    case 2: (void)HC05Init(uart);break;
    case 3: {
        Servo_Init_Config_s config={.servo_type=Bus_Servo,._handle=uart,.servo_id=1};
        (void)ServoInit(&config);break;
    }
    default: break;
    }
}
#endif

void BoardOptionalInit(void)
{
#if BOARD_AUX_UART2_DEVICE
    AuxiliaryUARTInit(&huart2,BOARD_AUX_UART2_DEVICE);
#endif
#if BOARD_AUX_UART7_DEVICE
    AuxiliaryUARTInit(&huart7,BOARD_AUX_UART7_DEVICE);
#endif
#if BOARD_OLED_ENABLE
    OLED_init();
#endif
#if BOARD_KEY_ENABLE
    Key_Config_s key={.key_gpio_config={.GPIOx=KEY_GPIO_Port,.GPIO_Pin=KEY_Pin,
        .exti_mode=GPIO_EXTI_MODE_RISING}};
    (void)KeyRegister(&key);
#endif
}
