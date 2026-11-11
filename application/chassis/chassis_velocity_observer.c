#if CHASSIS_VELOCITY_FUSION_ENABLE != 0 && CHASSIS_VELOCITY_FUSION_ENABLE != 1
#error CHASSIS_VELOCITY_FUSION_ENABLE must be 0 or 1
#endif
#if CHASSIS_VELOCITY_FUSION_CALIBRATED != 0 && CHASSIS_VELOCITY_FUSION_CALIBRATED != 1
#error CHASSIS_VELOCITY_FUSION_CALIBRATED must be 0 or 1
#endif
#if CHASSIS_VELOCITY_FUSION_CALIBRATED && !CHASSIS_VELOCITY_FUSION_ENABLE
#error CHASSIS_VELOCITY_FUSION_CALIBRATED requires CHASSIS_VELOCITY_FUSION_ENABLE
#endif

#if CHASSIS_VELOCITY_FUSION_ENABLE
#include "chassis_internal.h"
#include "chassis_control_config.h"
#include "chassis_velocity_observer.h"
#include "gimbal.h"
#include "ins_task.h"
#include "FreeRTOS.h"
#include "task.h"

#if !defined(ONE_BOARD) || CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED
#error Chassis velocity fusion requires a single board and speed control
#endif

#include "../../config/30_velocity_observation/chassis_velocity_config.h"
volatile ChassisFusedVelocitySnapshot_s chassis_fused_velocity;

bool ChassisGetFusedVelocitySnapshot(ChassisFusedVelocitySnapshot_s *snapshot)
{
    if (!snapshot) return false;
    taskENTER_CRITICAL();
    *snapshot=chassis_fused_velocity;
    const uint32_t now_ms=HAL_GetTick();
    taskEXIT_CRITICAL();
    const uint32_t limit=snapshot->status==CHASSIS_VELOCITY_FUSED ?
        CHASSIS_FUSED_VELOCITY_MAX_AGE_MS : CHASSIS_VELOCITY_MAX_AGE_MS;
    if ((uint32_t)(now_ms-snapshot->wheel.tick_ms)>CHASSIS_VELOCITY_MAX_AGE_MS)
        snapshot->wheel=(ChassisVelocity_s){0};
    if (!snapshot->wheel.valid || (uint32_t)(now_ms-snapshot->tick_ms)>limit)
        snapshot->valid=false;
    return snapshot->valid;
}

#if CHASSIS_VELOCITY_FUSION_CALIBRATED
#include "bsp_dwt.h"
#include "chassis_gimbal_imu.h"
#include "chassis_velocity_center.h"
#include "chassis_velocity_fusion.h"
#include "speed_observation_time.h"
#include <math.h>

#define TWO_PI 6.2831853071795864769f

typedef struct {
    ChassisGimbalImuGeometry_s geometry;
    ChassisVelocityFusionConfig_s filter;
    uint16_t joint_zero_ecd[2]; /* yaw, pitch */
    int8_t joint_sign[2];
} ChassisVelocityFusionCalibration_s;

/* The build directory supplies the measured installation and filter parameters. */
#include "chassis_velocity_fusion_calibration.h"

static PPObservationClock clock;
static ChassisVelocityCenter_s center_history;
static ChassisVelocityFusion_s filter;
static ChassisVelocityFusionOutput_s filtered;
static ChassisVelocityCenterSample_s filtered_sample;
static uint32_t last_imu_sequence;
static bool have_imu_sequence;

static void ResetHistory(void)
{
    center_history.count=0;
    ChassisVelocityFusionReset(&filter);
    filtered=(ChassisVelocityFusionOutput_s){0};
    have_imu_sequence=false;
}

static bool TimeAt(uint32_t cycles, uint32_t tick_ms, uint32_t *time_us)
{
    uint64_t time=0;
    const bool valid=PPObservationReceiveTime(&clock,cycles,tick_ms,CHASSIS_FUSION_RECEIVE_MAX_AGE_US,&time);
    *time_us=(uint32_t)time;
    return valid;
}

static bool JointAngle(const GimbalEncoderSample_s *joint, unsigned axis,
                       uint32_t imu_us, float *angle)
{
    uint32_t joint_us;
    const int sign=chassis_velocity_fusion_calibration.joint_sign[axis];
    const unsigned zero=chassis_velocity_fusion_calibration.joint_zero_ecd[axis];
    if ((sign!=1 && sign!=-1) || zero>=8192U || !TimeAt(joint->cycles,joint->tick_ms,&joint_us))
        return false;

    const uint32_t imu_after=imu_us-joint_us, joint_after=joint_us-imu_us;
    float dt_s;
    if (imu_after<=CHASSIS_FUSION_RECEIVE_MAX_SKEW_US) dt_s=(float)imu_after*1e-6f;
    else if (joint_after<=CHASSIS_FUSION_RECEIVE_MAX_SKEW_US) dt_s=-(float)joint_after*1e-6f;
    else return false;

    const float turns=(float)((int)joint->ecd-(int)zero)/8192.0f+(float)joint->rpm*dt_s/60.0f;
    *angle=(float)sign*turns*TWO_PI;
    return true;
}

static bool WheelTime(const ChassisVelocityInput_s *wheels, const uint32_t cycles[4],
                      uint32_t imu_us, uint32_t *wheel_us)
{
    uint32_t earliest_age=0, latest_age=UINT32_MAX;
    const uint32_t now_us=(uint32_t)clock.now_us;
    for (unsigned i=0;i<4;++i) {
        uint32_t time_us;
        if (!TimeAt(cycles[i],wheels->feedback_tick_ms[i],&time_us) ||
            ((uint32_t)(imu_us-time_us)>CHASSIS_FUSION_RECEIVE_MAX_SKEW_US &&
             (uint32_t)(time_us-imu_us)>CHASSIS_FUSION_RECEIVE_MAX_SKEW_US))
            return false;
        const uint32_t age=now_us-time_us;
        if (age>earliest_age) earliest_age=age;
        if (age<latest_age) latest_age=age;
    }
    if (earliest_age-latest_age>CHASSIS_FUSION_RECEIVE_MAX_SKEW_US) return false;
    *wheel_us=now_us-(earliest_age+latest_age)/2U;
    return true;
}

static ChassisFusedVelocityStatus_e UpdateFused(ChassisFusedVelocitySnapshot_s *snapshot,
    const ChassisVelocityInput_s *wheels, const uint32_t wheel_cycles[4],
    const INSMotionSnapshot_s *imu, const GimbalEncoderSample_s joints[2], bool joints_valid)
{
    ChassisVelocityCenterSample_s current={0}, center;
    if (!imu->valid || !TimeAt(imu->cycles,imu->tick_ms,&current.imu_us))
        return CHASSIS_VELOCITY_IMU_UNAVAILABLE;
    float yaw,pitch;
    if (!joints_valid || !JointAngle(&joints[0],0,current.imu_us,&yaw) ||
        !JointAngle(&joints[1],1,current.imu_us,&pitch))
        return CHASSIS_VELOCITY_GIMBAL_UNAVAILABLE;
    if (!WheelTime(wheels,wheel_cycles,current.imu_us,&current.wheel_us))
        return CHASSIS_VELOCITY_NOT_ALIGNED;

    ChassisGimbalImuPose_s pose;
    if (!ChassisGimbalImuPose(&chassis_velocity_fusion_calibration.geometry,
                             imu->quaternion,yaw,pitch,&pose))
        return CHASSIS_VELOCITY_REJECTED;
    for (unsigned i=0;i<3;++i) {
        current.imu_offset_n_m[i]=pose.imu_offset_nav_m[i];
        current.accel_n_mps2[i]=imu->accel_n_mps2[i];
    }
    current.wheel_b_mps[0]=snapshot->wheel.vx_mps;
    current.wheel_b_mps[1]=-snapshot->wheel.vy_mps;
    for (unsigned i=0;i<2;++i)
        for (unsigned j=0;j<2;++j)
            current.body_to_nav_xy[i][j]=pose.body_to_nav[i][j];

    ChassisVelocityCenterResult_e center_result=CHASSIS_CENTER_WAIT;
    const uint32_t sequence_delta=imu->sequence-last_imu_sequence;
    if (have_imu_sequence && sequence_delta>=UINT32_C(0x80000000))
        return CHASSIS_VELOCITY_REJECTED;
    if (!have_imu_sequence || sequence_delta) {
        center_result=ChassisVelocityCenterUpdate(&center_history,&current,&center);
        last_imu_sequence=imu->sequence;
        have_imu_sequence=true;
    }
    const uint32_t now_us=(uint32_t)clock.now_us;
    if (center_result==CHASSIS_CENTER_RESET) return CHASSIS_VELOCITY_REJECTED;
    if (center_result==CHASSIS_CENTER_READY) {
        ChassisVelocityFusionInput_s input={.now_us=now_us,.wheel_us=center.wheel_us,
            .imu_us=center.imu_us,.wheel_valid=true,.imu_valid=true};
        for (unsigned i=0;i<2;++i) {
            input.wheel_n_mps[i]=center.body_to_nav_xy[i][0]*center.wheel_b_mps[0]+
                                 center.body_to_nav_xy[i][1]*center.wheel_b_mps[1];
            input.accel_n_mps2[i]=center.accel_n_mps2[i];
        }
        ChassisVelocityFusionUpdate(&filter,&chassis_velocity_fusion_calibration.filter,&input,&filtered);
        filtered_sample=center;
    }
    if (!filtered.valid || !filtered.fused)
        return center_result==CHASSIS_CENTER_READY ? CHASSIS_VELOCITY_REJECTED : CHASSIS_VELOCITY_WARMUP;
    const uint32_t age_us=now_us-filtered.sample_us;
    if (age_us>CHASSIS_FUSED_VELOCITY_MAX_AGE_MS*1000U) return CHASSIS_VELOCITY_REJECTED;

    /* Invert the projected planar map, including the effect of chassis tilt. */
    const float a=filtered_sample.body_to_nav_xy[0][0], b=filtered_sample.body_to_nav_xy[0][1];
    const float c=filtered_sample.body_to_nav_xy[1][0], d=filtered_sample.body_to_nav_xy[1][1];
    const float det=a*d-b*c;
    if (!(det>0.5f)) return CHASSIS_VELOCITY_REJECTED;
    const float x=filtered.velocity_n_mps[0], y=filtered.velocity_n_mps[1];
    const float vx=(d*x-b*y)/det, vy=(c*x-a*y)/det;
    if (!isfinite(vx) || !isfinite(vy)) return CHASSIS_VELOCITY_REJECTED;
    snapshot->vx_mps=vx;
    snapshot->vy_mps=vy;
    snapshot->paired_wheel_mps[0]=filtered_sample.wheel_b_mps[0];
    snapshot->paired_wheel_mps[1]=-filtered_sample.wheel_b_mps[1];
    snapshot->tick_ms=wheels->now_ms-(age_us+999U)/1000U;
    for (unsigned i=0;i<2;++i)
        snapshot->center_accel_n_mps2[i]=filtered_sample.accel_n_mps2[i];
    return CHASSIS_VELOCITY_FUSED;
}
#endif /* CHASSIS_VELOCITY_FUSION_CALIBRATED */

void ChassisObserveFusedVelocity(const ChassisVelocityConfig_s *config)
{
    DJIMotorInstance *const motors[4]={motor_lf,motor_rf,motor_lb,motor_rb};
    ChassisVelocityInput_s wheels={0};
    ChassisFusedVelocitySnapshot_s snapshot={0};
    INSMotionSnapshot_s imu={0};
    GimbalEncoderSample_s joints[2]={0};
#if CHASSIS_VELOCITY_FUSION_CALIBRATED
    uint32_t wheel_cycles[4]={0};
#endif
    taskENTER_CRITICAL();
    for (unsigned i=0;i<4;++i) {
        const DJIMotorInstance *motor=motors[i];
        if (!motor || !motor->measure.rx_sequence || !DaemonIsOnline(motor->daemon)) continue;
        wheels.online_mask|=(uint8_t)(1U<<i);
        wheels.rpm[i]=(float)motor->measure.raw_rpm;
        wheels.feedback_tick_ms[i]=motor->measure.rx_tick_ms;
#if CHASSIS_VELOCITY_FUSION_CALIBRATED
        wheel_cycles[i]=motor->measure.rx_cycles;
#endif
    }
    (void)INSGetMotionSnapshot(&imu);
    const bool joints_valid=GimbalGetEncoderSnapshot(joints);
#if CHASSIS_VELOCITY_FUSION_CALIBRATED
    const uint32_t cycles=DWT_GetCycleCount();
#endif
    wheels.now_ms=HAL_GetTick();
    taskEXIT_CRITICAL();

    ChassisVelocityDiagnostics_s *diagnostics=&snapshot.diagnostics;
    diagnostics->available=true;
    diagnostics->tick_ms=wheels.now_ms;
    diagnostics->wheel_online_mask=wheels.online_mask;
    for (unsigned i=0;i<4;++i) {
        diagnostics->raw_rpm[i]=wheels.rpm[i];
        diagnostics->wheel_tick_ms[i]=wheels.feedback_tick_ms[i];
        diagnostics->imu_quaternion[i]=imu.quaternion[i];
    }
    for (unsigned i=0;i<3;++i) {
        diagnostics->imu_accel_n_mps2[i]=imu.accel_n_mps2[i];
        diagnostics->imu_gyro_sensor_radps[i]=imu.gyro_sensor_radps[i];
    }
    diagnostics->imu_tick_ms=imu.tick_ms;
    diagnostics->imu_sequence=imu.sequence;
    diagnostics->imu_valid=imu.valid;
    diagnostics->joints_valid=joints_valid;
    for (unsigned i=0;i<2;++i) {
        diagnostics->joint_ecd[i]=joints[i].ecd;
        diagnostics->joint_rpm[i]=joints[i].rpm;
        diagnostics->joint_tick_ms[i]=joints[i].tick_ms;
    }

    snapshot.valid=ChassisEstimateVelocity(config,&wheels,&snapshot.wheel);
    snapshot.vx_mps=snapshot.wheel.vx_mps;
    snapshot.vy_mps=snapshot.wheel.vy_mps;
    snapshot.tick_ms=snapshot.wheel.tick_ms;
    snapshot.status=snapshot.valid ? CHASSIS_VELOCITY_UNCALIBRATED : CHASSIS_VELOCITY_WHEEL_UNAVAILABLE;
#if CHASSIS_VELOCITY_FUSION_CALIBRATED
    const bool clock_valid=PPObservationClockUpdate(&clock,cycles,wheels.now_ms,DWT_GetFrequencyHz());
    if (snapshot.valid)
        snapshot.status=clock_valid ? UpdateFused(&snapshot,&wheels,wheel_cycles,&imu,joints,joints_valid) :
                                      CHASSIS_VELOCITY_CLOCK_INVALID;
    if (snapshot.status!=CHASSIS_VELOCITY_FUSED && snapshot.status!=CHASSIS_VELOCITY_WARMUP)
        ResetHistory();
#endif
    taskENTER_CRITICAL();
    chassis_fused_velocity=snapshot;
    taskEXIT_CRITICAL();
}
#endif /* CHASSIS_VELOCITY_FUSION_ENABLE */
