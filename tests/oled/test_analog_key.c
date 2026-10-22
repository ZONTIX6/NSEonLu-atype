#include "board_analog_key.h"
#include "adc.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
ADC_HandleTypeDef hadc1;
static unsigned raw,state=1,stops;
static int start_result,poll_result,stop_result;
int HAL_ADC_Start(ADC_HandleTypeDef *h) {assert(h==&hadc1);return start_result;}
int HAL_ADC_PollForConversion(ADC_HandleTypeDef *h,uint32_t t) {assert(h==&hadc1 && t==1);return poll_result;}
unsigned HAL_ADC_GetState(ADC_HandleTypeDef *h) {(void)h;return state;}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h) {(void)h;return raw;}
int HAL_ADC_Stop(ADC_HandleTypeDef *h) {(void)h;++stops;return stop_result;}
int main(void)
{
    OLED_button_e button;
    const unsigned values[]={0,501,1001,2001,3001,4001,100};
    const OLED_button_e expected[]={OLED_BUTTON_MID,OLED_BUTTON_LEFT,OLED_BUTTON_RIGHT,
        OLED_BUTTON_UP,OLED_BUTTON_DOWN,OLED_BUTTON_NONE,OLED_BUTTON_NONE};
    assert(!BoardAnalogKeyRead(NULL,NULL));
    for(unsigned i=0;i<7;++i) {raw=values[i];assert(BoardAnalogKeyRead(NULL,&button));assert(button==expected[i]);}
    raw=0;start_result=1;assert(!BoardAnalogKeyRead(NULL,&button) && button==OLED_BUTTON_NONE);
    start_result=0;poll_result=1;assert(!BoardAnalogKeyRead(NULL,&button) && button==OLED_BUTTON_NONE);
    poll_result=0;state=0;assert(!BoardAnalogKeyRead(NULL,&button) && button==OLED_BUTTON_NONE);
    state=1;stop_result=1;assert(!BoardAnalogKeyRead(NULL,&button) && button==OLED_BUTTON_NONE);
    assert(stops==11);
    puts("PASS: analog key ranges and ADC failure never create a middle-key event");
}
