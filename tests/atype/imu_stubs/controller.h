#pragma once
typedef struct {float Output;} PIDInstance;
enum {
    PID_Integral_Limit = 1,
    PID_Derivative_On_Measurement = 2,
    PID_Trapezoid_Intergral = 4
};
typedef struct {
    float MaxOut,IntegralLimit,DeadBand,Kp,Ki,Kd;
    unsigned Improve;
    float CoefA,CoefB,Output_LPF_RC,Derivative_LPF_RC;
} PID_Init_Config_s;
void PIDInit(PIDInstance *pid,PID_Init_Config_s *config);
float PIDCalculate(PIDInstance *pid,float measured,float target);
