#ifndef TEST_CONTROLLER_H
#define TEST_CONTROLLER_H

typedef struct
{
    float value;
} PIDInstance;

typedef struct
{
    float value;
} PID_Init_Config_s;

void PIDInit(PIDInstance *pid, PID_Init_Config_s *config);
float PIDCalculate(PIDInstance *pid, float measure, float ref);

#endif
