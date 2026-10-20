#ifndef TEST_CONTROLLER_H
#define TEST_CONTROLLER_H

typedef struct
{
    float value;
    float Kp, Ki, Kd, MaxOut, IntegralLimit, DeadBand;
    float Iout, ITerm, Last_ITerm, Dout, Last_Dout, Output, Last_Output;
    float Last_Measure,Last_Err;
    unsigned DWT_CNT;
    unsigned Improve;
    float CoefA,CoefB,Output_LPF_RC,Derivative_LPF_RC;
} PIDInstance;

typedef struct
{
    float value;
} PID_Init_Config_s;

void PIDInit(PIDInstance *pid, PID_Init_Config_s *config);
float PIDCalculate(PIDInstance *pid, float measure, float ref);

#endif
