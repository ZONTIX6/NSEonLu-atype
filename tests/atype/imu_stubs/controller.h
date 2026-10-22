#pragma once
typedef struct {float Output;} PIDInstance;
typedef struct {float MaxOut,IntegralLimit,DeadBand,Kp,Ki,Kd;unsigned Improve;} PID_Init_Config_s;
void PIDInit(PIDInstance *pid,PID_Init_Config_s *config);
float PIDCalculate(PIDInstance *pid,float measured,float target);
