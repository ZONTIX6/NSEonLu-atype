#ifndef TEST_OLED_MAIN_H
#define TEST_OLED_MAIN_H
#include <stdint.h>
typedef struct {unsigned marker;} GPIO_TypeDef;
typedef struct {unsigned marker;} SPI_HandleTypeDef;
typedef struct {unsigned marker;} I2C_HandleTypeDef;
typedef struct {unsigned marker;} ADC_HandleTypeDef;
extern SPI_HandleTypeDef hspi1;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c2;
extern GPIO_TypeDef test_port;
#define OLED_DC_GPIO_Port (&test_port)
#define OLED_RST_GPIO_Port (&test_port)
#define OLED_DC_Pin 1U
#define OLED_RST_Pin 2U
#define GPIO_PIN_SET 1U
#define GPIO_PIN_RESET 0U
#define HAL_ADC_STATE_REG_EOC 1U
#define HAL_OK 0
#define HAL_IS_BIT_SET(value,mask) (((value)&(mask))!=0)
void HAL_GPIO_WritePin(GPIO_TypeDef *,uint16_t,unsigned);
int HAL_SPI_Transmit(SPI_HandleTypeDef *,uint8_t *,uint16_t,uint32_t);
int HAL_I2C_Master_Transmit(I2C_HandleTypeDef *,uint16_t,uint8_t *,uint16_t,uint32_t);
int HAL_ADC_Start(ADC_HandleTypeDef *);
int HAL_ADC_Stop(ADC_HandleTypeDef *);
int HAL_ADC_PollForConversion(ADC_HandleTypeDef *,uint32_t);
unsigned HAL_ADC_GetState(ADC_HandleTypeDef *);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
#endif
