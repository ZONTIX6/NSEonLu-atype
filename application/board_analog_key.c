#include "board_analog_key.h"
#include "adc.h"
#include <stddef.h>

bool BoardAnalogKeyRead(void *context,OLED_button_e *button)
{
    (void)context;
    if(button==NULL) return false;
    *button=OLED_BUTTON_NONE;
    const bool started=HAL_ADC_Start(&hadc1)==HAL_OK;
    const bool valid=started && HAL_ADC_PollForConversion(&hadc1,1)==HAL_OK &&
        HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc1),HAL_ADC_STATE_REG_EOC);
    const uint32_t value=valid ? HAL_ADC_GetValue(&hadc1) : 0;
    const bool stopped=HAL_ADC_Stop(&hadc1)==HAL_OK;
    if(!valid || !stopped) return false;
    if(value>4000) *button=OLED_BUTTON_NONE;
    else if(value>3000) *button=OLED_BUTTON_DOWN;
    else if(value>2000) *button=OLED_BUTTON_UP;
    else if(value>1000) *button=OLED_BUTTON_RIGHT;
    else if(value>500) *button=OLED_BUTTON_LEFT;
    else if(value==0) *button=OLED_BUTTON_MID;
    return true;
}
