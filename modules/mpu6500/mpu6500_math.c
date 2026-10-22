#include "mpu6500_math.h"
#include <math.h>

float MPU6500Temperature(uint8_t high, uint8_t low)
{
    return (int16_t)(((uint16_t)high<<8)|low)/333.87f+21.0f;
}

bool MPU6500InitialQuaternion(const float acceleration[3], float quaternion[4])
{
    const float norm=sqrtf(acceleration[0]*acceleration[0]+acceleration[1]*acceleration[1]+
        acceleration[2]*acceleration[2]);
    if (!isfinite(norm) || norm<0.001f) return false;
    float z=fmaxf(-1.0f,fminf(1.0f,acceleration[2]/norm));
    if (z < -0.999999f) {
        quaternion[0]=0;quaternion[1]=1;quaternion[2]=quaternion[3]=0;
    } else {
        quaternion[0]=sqrtf((1.0f+z)*0.5f);
        quaternion[1]=acceleration[1]/(norm*2.0f*quaternion[0]);
        quaternion[2]=-acceleration[0]/(norm*2.0f*quaternion[0]);
        quaternion[3]=0;
    }
    return true;
}

void MPU6500ExportSample(const float gyro_rad[3], const float ekf_ypr[3],
                         float gyro_deg[3], float vehicle_ypr[3])
{
    for(unsigned i=0;i<3;++i) gyro_deg[i]=gyro_rad[i]*57.2957795f;
    vehicle_ypr[0]=ekf_ypr[0];
    vehicle_ypr[1]=ekf_ypr[2];
    vehicle_ypr[2]=ekf_ypr[1];
}
