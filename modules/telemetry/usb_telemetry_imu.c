#include "usb_telemetry_imu.h"
#include <limits.h>
#include <math.h>

void USBTelemetryEncodeIMU(USBTelemetrySample_s *sample,
                          const INSSnapshot_s *imu, uint32_t now_ms)
{
    const attitude_t *a = &imu->attitude;
    sample->imu_age_ms = imu->has_sample ?
        (uint32_t)(now_ms - imu->tick_ms) : UINT32_MAX;
    sample->imu_valid = imu->has_sample && imu->valid && sample->imu_age_ms <= 10U &&
        isfinite(a->Yaw) && isfinite(a->Pitch) && isfinite(a->Roll) && isfinite(a->YawTotalAngle);
    for (unsigned i = 0; i < 3; ++i)
        sample->imu_valid &= isfinite(a->Gyro[i]) && isfinite(a->Accel[i]);
    sample->imu_yaw_mdeg = USBTelemetryFixed(a->Yaw, 1000);
    sample->imu_pitch_mdeg = USBTelemetryFixed(a->Pitch, 1000);
    sample->imu_roll_mdeg = USBTelemetryFixed(a->Roll, 1000);
    sample->imu_yaw_total_mdeg = USBTelemetryFixed(a->YawTotalAngle, 1000);
    sample->imu_gx_mdps = USBTelemetryFixed(a->Gyro[0], 1000);
    sample->imu_gy_mdps = USBTelemetryFixed(a->Gyro[1], 1000);
    sample->imu_gz_mdps = USBTelemetryFixed(a->Gyro[2], 1000);
    sample->imu_ax_mmps2 = USBTelemetryFixed(a->Accel[0], 1000);
    sample->imu_ay_mmps2 = USBTelemetryFixed(a->Accel[1], 1000);
    sample->imu_az_mmps2 = USBTelemetryFixed(a->Accel[2], 1000);
}
