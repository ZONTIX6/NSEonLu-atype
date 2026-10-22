#include "mpu6500_math.h"
#include "ins_math.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    assert(fabsf(MPU6500Temperature(0,0)-21)<0.001f);
    assert(fabsf(MPU6500Temperature(0xfe,0xb2)-19.9996f)<0.01f);
    float q[4],up[3]={0,0,9.81f},down[3]={0,0,-9.81f},zero[3]={0};
    assert(MPU6500InitialQuaternion(up,q) && q[0]==1 && q[1]==0);
    assert(MPU6500InitialQuaternion(down,q) && q[0]==0 && q[1]==1);
    assert(!MPU6500InitialQuaternion(zero,q));
    float gyro[3]={1,-1,0.5f},ekf[3]={10,20,30},deg[3],angles[3];
    MPU6500ExportSample(gyro,ekf,deg,angles);
    assert(fabsf(deg[0]-57.29578f)<0.001f && fabsf(deg[1]+57.29578f)<0.001f);
    assert(angles[0]==10 && angles[1]==30 && angles[2]==20);
    EularAngleToQuaternion(90,0,0,q);
    float yaw,pitch,roll;
    QuaternionToEularAngle(q,&yaw,&pitch,&roll);
    assert(fabsf(yaw-90)<0.001f && fabsf(pitch)<0.001f && fabsf(roll)<0.001f);
    float identity[4]={1,0,0,0};QuaternionUpdate(identity,0,0,1,0.002f);
    assert(fabsf(identity[3]-0.001f)<0.000001f);
    puts("PASS: signed MPU temperature, deg/s export, pitch/roll and degenerate gravity");
}
