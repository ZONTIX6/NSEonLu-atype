#include "chassis_remote_fusion.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ChassisRemoteFusionConfig_s Config(void)
{
    return (ChassisRemoteFusionConfig_s){
        .sensor_to_body_quaternion = {1, 0, 0, 0},
        .filter = {.wheel_variance=.01f, .accel_variance=.04f, .jerk_density=.25f,
            .max_velocity_residual_mps=2, .max_age_us=60000, .max_skew_us=0,
            .max_gap_us=30000},
        .max_pair_span_ms=15, .max_wheel_skew_ms=3, .max_timing_jitter_ms=10};
}

static ChassisImuCanSnapshot_s Packet(uint32_t source, uint32_t receive, uint32_t sequence)
{
    return (ChassisImuCanSnapshot_s){
        .sample={.quaternion={1,0,0,0}, .sequence=sequence, .tick_ms=source,
            .source_age_ms=2, .valid=true},
        .first_rx_tick_ms=receive, .last_rx_tick_ms=receive+1, .available=true};
}

static ChassisRemoteFusionOutput_s Run(ChassisRemoteFusion_s *state,
    const ChassisRemoteFusionConfig_s *config, ChassisImuCanSnapshot_s *packet,
    uint32_t now, float vx, float vy)
{
    ChassisRemoteFusionOutput_s out;
    ChassisVelocity_s wheel={.valid=true,.tick_ms=now,.vx_mps=vx,.vy_mps=vy};
    ChassisRemoteFusionUpdate(state,config,&wheel,0,packet,packet!=NULL,now,&out);
    return out;
}

static void Near(float a,float b,float tolerance)
{
    assert(isfinite(a) && fabsf(a-b)<tolerance);
}

static void RemoteClockOffsetAndRepeatedPackets(void)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionReset(&state);
    ChassisRemoteFusionConfig_s config=Config();
    ChassisRemoteFusionOutput_s out={0};
    ChassisImuCanSnapshot_s packet={0};
    for(uint32_t t=100;t<=165;t+=5) {
        if(t==125 || t==145 || t==165) packet=Packet(900000+t-3,t-1,t);
        out=Run(&state,&config,packet.available?&packet:NULL,t,1,-.5f);
    }
    assert(out.valid && out.status==CHASSIS_REMOTE_FUSED && out.timing_estimated);
    assert(out.tick_ms==142 && out.source_tick_ms==900162);
    assert(out.source_step_ms==20 && out.receive_step_ms==20 && out.updates==1);
    Near(out.vx_mps,1,.00001f); Near(out.vy_mps,-.5f,.00001f);
    const uint32_t epoch=out.tick_ms,updates=out.updates;
    out=Run(&state,&config,&packet,170,2,3);
    assert(out.status==CHASSIS_REMOTE_FUSED && out.tick_ms==epoch && out.updates==updates);
    Near(out.paired_wheel_mps[0],1,.00001f);
    Near(out.paired_wheel_mps[1],-.5f,.00001f);
}

static ChassisRemoteFusionOutput_s Prime(ChassisRemoteFusion_s *state,
    const ChassisRemoteFusionConfig_s *config, ChassisImuCanSnapshot_s *packet)
{
    ChassisRemoteFusionReset(state);
    ChassisRemoteFusionOutput_s out={0};
    for(uint32_t t=100;t<=165;t+=5) {
        if(t==125 || t==145 || t==165) *packet=Packet(900000+t-3,t-1,t);
        out=Run(state,config,packet->available?packet:NULL,t,1,0);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED);
    return out;
}

static void HistoricalInterpolationAndAcceleration(void)
{
    ChassisRemoteFusion_s state={0};
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out={0};
    for(uint32_t t=100;t<=185;t+=5) {
        if(t>=125 && (t-125)%20==0) {
            packet=Packet(500000+t-3,t-1,t);
            packet.sample.accel_n_mps2[0]=10;
        }
        out=Run(&state,&config,packet.available?&packet:NULL,t,(float)(t-100)*.01f,0);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED && out.tick_ms==162 && out.pair_span_ms==5);
    Near(out.paired_wheel_mps[0],.62f,.00001f);
    Near(out.vx_mps,.62f,.0001f);
    assert(out.updates==2);
}

static void EveryNewPacketMayWaitForItsLaterWheelBracket(void)
{
    ChassisRemoteFusion_s state={0};
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out={0};
    for(uint32_t t=100;t<=210;t+=5) {
        const bool arrival=t>=125 && (t-125)%20==0;
        if(arrival) {
            packet=Packet(900000+t,t-1,t);
            packet.sample.source_age_ms=0;
        }
        ChassisVelocity_s wheel={.valid=true,.tick_ms=t-2,.vx_mps=1};
        ChassisRemoteFusionUpdate(&state,&config,&wheel,0,packet.available?&packet:NULL,
            packet.available,t,&out);
        if(arrival && t<185) assert(out.status==CHASSIS_REMOTE_NOT_ALIGNED);
        if(arrival && t>=185) {
            assert(out.status==CHASSIS_REMOTE_FUSED && out.tick_ms==t-41);
            const uint32_t epoch=out.tick_ms,updates=out.updates;
            ChassisRemoteFusionUpdate(&state,&config,&wheel,0,&packet,true,t+1,&out);
            assert(out.status==CHASSIS_REMOTE_FUSED && out.tick_ms==epoch && out.updates==updates);
        }
        if(t>=170 && (t-170)%20==0) {
            assert(out.status==CHASSIS_REMOTE_FUSED);
            assert(out.tick_ms==t-26 && out.resets==0);
        }
    }
    assert(out.status==CHASSIS_REMOTE_FUSED && out.updates==3 && out.resets==0);
    Near(out.vx_mps,1,.00001f);
}

static void PendingWheelBracketHasABoundedWait(void)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out=Prime(&state,&config,&packet);
    for(uint32_t t=170;t<185;t+=5) out=Run(&state,&config,&packet,t,1,0);
    packet=Packet(900182,184,185);
    packet.sample.source_age_ms=0;
    const ChassisVelocity_s wheel={.valid=true,.tick_ms=183,.vx_mps=1};
    for(uint32_t t=185;t<=200;t+=5) {
        ChassisRemoteFusionUpdate(&state,&config,&wheel,0,&packet,true,t,&out);
        if(t<200) assert(out.status==CHASSIS_REMOTE_FUSED && out.tick_ms==142 && out.updates==1);
    }
    assert(out.status==CHASSIS_REMOTE_NOT_ALIGNED && out.resets==1 && out.updates==1);
    assert(out.tick_ms==wheel.tick_ms && out.valid);
}

static void FixedMountAndOffsetUseSourceIntervals(void)
{
    ChassisRemoteFusion_s state={0};
    ChassisRemoteFusionConfig_s config=Config();
    config.sensor_to_body_quaternion[0]=sqrtf(.5f);
    config.sensor_to_body_quaternion[3]=sqrtf(.5f);
    config.imu_offset_body_m[0]=.2f;
    config.imu_offset_body_m[1]=.1f;
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out={0};
    const float omega=1.7f;
    for(uint32_t t=100;t<=225;t+=5) {
        if(t>=125 && (t-125)%20==0) {
            /* Alternating 2 ms receive jitter must not corrupt the physical
             * interval in the second derivative of the rotating offset. */
            const uint32_t jitter=((t-125)/20)%2 ? 2 : 0;
            packet=Packet(700000+t-3,t-1-jitter,t);
            const float theta=omega*(float)(t-125)*.001f;
            const float half=(theta+1.57079632679f)*.5f;
            packet.sample.quaternion[0]=cosf(half);
            packet.sample.quaternion[3]=sinf(half);
            packet.sample.accel_n_mps2[0]=-omega*omega*(.2f*cosf(theta)-.1f*sinf(theta));
            packet.sample.accel_n_mps2[1]=-omega*omega*(.2f*sinf(theta)+.1f*cosf(theta));
        }
        out=Run(&state,&config,packet.available?&packet:NULL,t,0,0);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED && out.updates==4);
    Near(out.center_accel_n_mps2[0],0,.001f);
    Near(out.center_accel_n_mps2[1],0,.001f);
    Near(out.vx_mps,0,.001f); Near(out.vy_mps,0,.001f);
}

static void LossAndSamePacketRecoveryDoNotReuseMeasurement(void)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out=Prime(&state,&config,&packet);
    const uint32_t updates=out.updates;
    ChassisVelocity_s wheel={.valid=false,.tick_ms=170,.vx_mps=2};
    ChassisRemoteFusionUpdate(&state,&config,&wheel,0,&packet,true,170,&out);
    assert(!out.valid && out.status==CHASSIS_REMOTE_WHEEL_UNAVAILABLE && out.resets>0);
    out=Run(&state,&config,&packet,175,2,0);
    assert(out.valid && out.status==CHASSIS_REMOTE_WARMUP && out.updates==updates);
    Near(out.vx_mps,2,.00001f);
    for(uint32_t t=180;t<=225;t+=5) {
        if(t>=185 && (t-185)%20==0) packet=Packet(900000+t-3,t-1,t);
        out=Run(&state,&config,&packet,t,2,0);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED && out.updates==updates+1);
}

static void StaleOutputAndRemoteBecomeWheelFallback(void)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out=Prime(&state,&config,&packet);
    for(uint32_t t=170;t<=205;t+=5) out=Run(&state,&config,&packet,t,2,3);
    assert(out.valid && out.status!=CHASSIS_REMOTE_FUSED && out.tick_ms==205);
    Near(out.vx_mps,2,.00001f); Near(out.vy_mps,3,.00001f);
    out=Run(&state,&config,&packet,230,2,3);
    assert(out.status==CHASSIS_REMOTE_IMU_UNAVAILABLE && out.updates==1);
}

static void GapsRebootsAndJitterRejectThenReacquire(void)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out=Prime(&state,&config,&packet);
    for(uint32_t t=170;t<=205;t+=5) {
        if(t==205) packet=Packet(900202,204,205); /* One lost 50 Hz packet. */
        out=Run(&state,&config,&packet,t,1,0);
    }
    assert(out.status==CHASSIS_REMOTE_REJECTED && out.resets==1);
    for(uint32_t t=210;t<=265;t+=5) {
        if(t>=225 && (t-225)%20==0) packet=Packet(900000+t-3,t-1,t);
        out=Run(&state,&config,&packet,t,1,0);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED && out.updates==2);
    packet=Packet(5,269,1); /* Source restarted; receiver did not. */
    out=Run(&state,&config,&packet,270,1,0);
    assert(out.status==CHASSIS_REMOTE_REJECTED);
    for(uint32_t t=275;t<=330;t+=5) {
        if(t>=290 && (t-290)%20==0) packet=Packet(t-265,t-1,t-269);
        out=Run(&state,&config,&packet,t,1,0);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED);
    packet=Packet(85,369,81); /* Source +20 ms, receiver +40 ms. */
    out=Run(&state,&config,&packet,370,1,0);
    assert(out.status==CHASSIS_REMOTE_REJECTED);
}

static void NoHistoryAndWideWheelSkewNeverFakeAlignment(void)
{
    ChassisRemoteFusion_s state={0};
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet=Packet(5000,99,1);
    ChassisRemoteFusionOutput_s out=Run(&state,&config,&packet,100,4,0);
    assert(out.status==CHASSIS_REMOTE_NOT_ALIGNED && out.valid && !out.updates);
    ChassisVelocity_s wheel={.valid=true,.tick_ms=105,.vx_mps=5};
    ChassisRemoteFusionUpdate(&state,&config,&wheel,4,&packet,true,105,&out);
    assert(out.status==CHASSIS_REMOTE_NOT_ALIGNED && out.valid);
    Near(out.vx_mps,5,.00001f);
    ChassisRemoteFusionReset(&state);
    (void)Run(&state,&config,NULL,100,0,0);
    packet=Packet(6000,113,2); /* Target111 bracketed only by100 and120: too wide. */
    out=Run(&state,&config,&packet,120,1,0);
    assert(out.status==CHASSIS_REMOTE_NOT_ALIGNED);
}

static void InvalidDataCannotPublishFusion(void)
{
    ChassisRemoteFusion_s state;
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out=Prime(&state,&config,&packet);
    config.filter.accel_variance=NAN;
    out=Run(&state,&config,&packet,170,1,0);
    assert(out.valid && out.status==CHASSIS_REMOTE_CONFIG_INVALID);
    config=Config(); config.imu_offset_body_m[0]=INFINITY;
    out=Run(&state,&config,&packet,175,1,0);
    assert(out.status==CHASSIS_REMOTE_CONFIG_INVALID);
    config=Config(); config.sensor_to_body_quaternion[0]=2;
    out=Run(&state,&config,&packet,180,1,0);
    assert(out.status==CHASSIS_REMOTE_CONFIG_INVALID);
    config=Config(); packet=Packet(900182,184,185);
    packet.sample.accel_n_mps2[0]=NAN;
    out=Run(&state,&config,&packet,185,1,0);
    assert(out.status==CHASSIS_REMOTE_REJECTED);
    out=Run(&state,&config,&packet,190,NAN,0);
    assert(!out.valid && out.status==CHASSIS_REMOTE_WHEEL_UNAVAILABLE);
}

static void BothClocksAndSequenceCanWrap(void)
{
    ChassisRemoteFusion_s state={0};
    ChassisRemoteFusionConfig_s config=Config();
    ChassisImuCanSnapshot_s packet={0};
    ChassisRemoteFusionOutput_s out={0};
    const uint32_t receiver_start=UINT32_MAX-50,source_start=UINT32_MAX-40;
    for(uint32_t offset=0;offset<=85;offset+=5) {
        const uint32_t now=receiver_start+offset;
        if(offset>=25 && (offset-25)%20==0)
            packet=Packet(source_start+offset,now-1,UINT32_MAX-45+offset);
        out=Run(&state,&config,packet.available?&packet:NULL,now,0,.4f);
    }
    assert(out.status==CHASSIS_REMOTE_FUSED && out.updates==2);
    assert(out.tick_ms==receiver_start+62);
    Near(out.vy_mps,.4f,.00001f);
}

int main(void)
{
    RemoteClockOffsetAndRepeatedPackets();
    HistoricalInterpolationAndAcceleration();
    EveryNewPacketMayWaitForItsLaterWheelBracket();
    PendingWheelBracketHasABoundedWait();
    FixedMountAndOffsetUseSourceIntervals();
    LossAndSamePacketRecoveryDoNotReuseMeasurement();
    StaleOutputAndRemoteBecomeWheelFallback();
    GapsRebootsAndJitterRejectThenReacquire();
    NoHistoryAndWideWheelSkewNeverFakeAlignment();
    InvalidDataCannotPublishFusion();
    BothClocksAndSequenceCanWrap();
    puts("PASS: remote chassis velocity fusion");
    return 0;
}
