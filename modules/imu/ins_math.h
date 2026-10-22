#ifndef INS_MATH_H
#define INS_MATH_H
void QuaternionUpdate(float *q,float gx,float gy,float gz,float dt);
void QuaternionToEularAngle(float *q,float *yaw,float *pitch,float *roll);
void EularAngleToQuaternion(float yaw,float pitch,float roll,float *q);
#endif
