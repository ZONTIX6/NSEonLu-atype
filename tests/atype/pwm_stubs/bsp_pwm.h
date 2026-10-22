#ifndef TEST_PWM_H
#define TEST_PWM_H
typedef struct {unsigned marker;} TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim3;
#define TIM_CHANNEL_2 4U
typedef struct {float duty;} PWMInstance;
typedef struct {TIM_HandleTypeDef *htim;unsigned channel;float period,dutyratio;} PWM_Init_Config_s;
PWMInstance *PWMRegister(PWM_Init_Config_s *config);
void PWMSetDutyRatio(PWMInstance *instance,float ratio);
#endif
