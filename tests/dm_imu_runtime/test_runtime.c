#include "chassis_imu_can.h"
#include "chassis_remote_fusion.h"
#include "bsp_can.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

CAN_HandleTypeDef hcan1={1},hcan2={2};
static CANInstance endpoint;
static uint32_t tick;
static unsigned registrations,critical;
uint32_t HAL_GetTick(void) { return tick; }
void TestEnterCritical(void) { ++critical; }
void TestExitCritical(void) { assert(critical); --critical; }
CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    /* The selected device must subscribe on the user's CAN2 wiring. */
    assert(config->can_handle==&hcan2 && config->rx_id==0x11);
    endpoint=(CANInstance){.can_handle=config->can_handle,.rx_id=config->rx_id,
        .tx_id=config->tx_id,.can_module_callback=config->can_module_callback};
    ++registrations;
    return &endpoint;
}
void CANSetDLC(CANInstance *instance,uint8_t n)
{ assert(instance==&endpoint && n==8); }

static void Receive(const uint8_t data[8],uint32_t now)
{
    tick=now;endpoint.rx_len=8;
    memcpy(endpoint.rx_buff,data,8);
    endpoint.can_module_callback(&endpoint);
}
static void Group(uint32_t now)
{
    /* Hand-computed resting +Z gravity, zero gyro, identity quaternion. */
    const uint8_t accel[8]={1,0,0,128,0,128,0x56,0x85};
    const uint8_t gyro[8]={2,0,0,128,0,128,0,128};
    const uint8_t quat[8]={4,255,254,0,8,0,32,0};
    Receive(accel,now);Receive(gyro,now+1);Receive(quat,now+2);
}

static void TestReceptionJitter(const ChassisRemoteFusionConfig_s *config)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionReset(&state);
    ChassisRemoteFusionOutput_s out;
    ChassisImuCanSnapshot_s snapshot;
    /* 100 Hz with only +/-1 ms receive jitter. A strict 20 ms gate used to
     * skip the 19 ms candidate, publish at 31 ms and reset the real filter. */
    for(uint32_t now=1000;now<2000;++now) {
        const uint32_t phase=(now-1000U)%40U;
        if(phase==0U || phase==10U || phase==19U || phase==31U) Group(now);
        const bool usable=ChassisImuCanReadSnapshot(&snapshot,now+2U);
        const ChassisVelocity_s wheel={.valid=true,.tick_ms=now,.vx_mps=.4f};
        ChassisRemoteFusionUpdate(&state,config,&wheel,0,&snapshot,usable,now+2U,&out);
        if(now>=1100U) {
            assert(out.valid && out.status==CHASSIS_REMOTE_FUSED);
            assert(fabsf(out.vx_mps-.4f)<.01f);
        }
    }
    assert(out.updates>=40U && out.resets==0U);
}

int main(void)
{
    ChassisImuCanSnapshot_s snapshot;
    assert(!ChassisImuCanReadSnapshot(NULL,0));
    assert(!ChassisImuCanReadSnapshot(&snapshot,0) && !snapshot.available);
    ChassisImuCanInit();ChassisImuCanInit();
    assert(registrations==1 && endpoint.can_module_callback);
    Group(100);
    assert(ChassisImuCanReadSnapshot(&snapshot,102) && !critical);
    assert(snapshot.sample.tick_ms==100 && snapshot.first_rx_tick_ms==100);
    assert(snapshot.last_rx_tick_ms==102 && snapshot.sample.source_age_ms==0);
    assert(fabsf(snapshot.sample.accel_n_mps2[2])<.01f);
    const uint32_t sequence=snapshot.sample.sequence;
    Group(110);
    assert(ChassisImuCanReadSnapshot(&snapshot,112) && snapshot.sample.sequence==sequence);
    assert(snapshot.first_rx_tick_ms==100);
    assert(ChassisImuCanReadSnapshot(&snapshot,160));
    assert(!ChassisImuCanReadSnapshot(&snapshot,161));

    ChassisRemoteFusion_s state;
    ChassisRemoteFusionReset(&state);
    const ChassisRemoteFusionConfig_s config={.sensor_to_body_quaternion={1,0,0,0},
        .filter={.wheel_variance=.01f,.accel_variance=.25f,.jerk_density=1,
            .max_velocity_residual_mps=1.5f,.max_age_us=60000,.max_gap_us=60000},
        .max_pair_span_ms=15,.max_wheel_skew_ms=3,.max_timing_jitter_ms=10};
    ChassisRemoteFusionOutput_s out;
    /* Actual wire frames -> production runtime -> production velocity filter. */
    for(uint32_t now=200;now<=500;now+=2) {
        if(now%10==0 && now<=400) Group(now);
        bool usable=ChassisImuCanReadSnapshot(&snapshot,now+2);
        ChassisVelocity_s wheel={.valid=true,.tick_ms=now,.vx_mps=.4f};
        ChassisRemoteFusionUpdate(&state,&config,&wheel,0,&snapshot,usable,now+2,&out);
        if(now==400) {
            assert(out.valid && out.status==CHASSIS_REMOTE_FUSED && out.updates>=5);
            assert(fabsf(out.vx_mps-.4f)<.01f && out.timing_estimated);
        }
    }
    assert(out.valid && out.status==CHASSIS_REMOTE_IMU_UNAVAILABLE);
    assert(out.vx_mps==.4f);
    for(uint32_t now=520;now<=680;now+=2) {
        if(now%10==0) Group(now);
        bool usable=ChassisImuCanReadSnapshot(&snapshot,now+2);
        ChassisVelocity_s wheel={.valid=true,.tick_ms=now,.vx_mps=.4f};
        ChassisRemoteFusionUpdate(&state,&config,&wheel,0,&snapshot,usable,now+2,&out);
    }
    assert(out.valid && out.status==CHASSIS_REMOTE_FUSED);
    TestReceptionJitter(&config);
    puts("PASS: CAN2 DM IMU -> fused observation, receive jitter, stale fallback and recovery");
    return 0;
}
