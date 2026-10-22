#pragma once
typedef struct {float q[4],Yaw,Pitch,Roll,YawTotalAngle;} TestEKF;
extern TestEKF QEKF_INS;
void IMU_QuaternionEKF_Init(float *q,float a,float b,float c,float d,float e);
void IMU_QuaternionEKF_Update(float gx,float gy,float gz,float ax,float ay,float az,float dt);
