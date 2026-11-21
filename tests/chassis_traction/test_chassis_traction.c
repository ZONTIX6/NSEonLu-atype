#include "chassis_traction.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static ChassisTractionConfig_s c;
static ChassisTractionInput_s i;
static ChassisTractionState_s s;
static ChassisTractionOutput_s o;
static const float projection=.7071067811865475f;
static const float mps_per_rpm=6.283185307179586f*.076f/(60*19);
static void Near(float expected,float actual) { assert(fabsf(expected-actual)<.0001f); }

static void Init(unsigned mode)
{
    const ChassisVelocityConfig_s g={.omni_x=true,.wheel_radius_m=.076f,.reduction_ratio=19,
        .wheel_base_m=.350f,.track_width_m=.300f,.direction={1,1,1,1}};
    c=ChassisTractionDefaultConfig(&g);c.mode=mode;
    c.accel_allowance_mps2=c.yaw_accel_allowance_radps2=0;
    s=(ChassisTractionState_s){0};
    i=(ChassisTractionInput_s){.reference={.valid=true,.vx_mps=1,.tick_ms=100,.gyro_tick_ms=100},
        .now_ms=100,.dt_s=.005f,.online_mask=15,.enabled_mask=15,
        .drive_enabled=true,.targets_match=true};
    for(unsigned w=0;w<4;++w) {
        const float sign=(w==0 || w==2) ? -1:1;
        i.actual_rpm[w]=sign*projection/mps_per_rpm;
        i.raw_current[w]=sign*1000;i.max_current[w]=15000;
        i.feedback_tick_ms[w]=i.now_ms;i.feedback_sequence[w]=1;
    }
}
static void Step(void)
{
    i.now_ms+=5;i.reference.tick_ms=i.reference.gyro_tick_ms=i.now_ms;
    for(unsigned w=0;w<4;++w) {i.feedback_tick_ms[w]=i.now_ms;++i.feedback_sequence[w];}
    assert(ChassisTractionUpdate(&c,&i,&s,&o));
}
static void Steps(unsigned count) { while(count--) Step(); }

static void TestRollingProjectionAndDirections(void)
{
    Init(3);Steps(20);assert(o.valid && !o.spin_mask && !o.lock_mask && !o.intervened_mask);
    Near(-projection,o.expected_mps[0]);Near(projection,o.expected_mps[1]);
    for(unsigned w=0;w<4;++w) Near(i.raw_current[w],o.current[w]);
    Init(3);i.reference.vx_mps=0;i.reference.vy_mps=1;
    const float lateral[4]={1,1,-1,-1};
    for(unsigned w=0;w<4;++w) {i.actual_rpm[w]=lateral[w]*projection/mps_per_rpm;i.raw_current[w]=lateral[w]*1000;}
    Steps(20);assert(!o.spin_mask && !o.lock_mask);
    Init(3);i.reference.vx_mps=0;i.reference.wz_radps=4;
    for(unsigned w=0;w<4;++w) i.actual_rpm[w]=4*.325f*projection/mps_per_rpm;
    Steps(20);assert(!o.spin_mask && !o.lock_mask);Near(4*.325f*projection,o.expected_mps[0]);
    Init(3);c.geometry.direction[1]=-1;i.actual_rpm[1]*=-1;i.raw_current[1]*=-1;
    Steps(20);assert(!o.spin_mask && !o.lock_mask);Near(-projection,o.expected_mps[1]);
    Init(3);c.geometry.omni_x=false;
    for(unsigned w=0;w<4;++w) {float sign=w<2 ? -1:1;i.actual_rpm[w]=sign/mps_per_rpm;i.raw_current[w]=sign*1000;}
    Steps(20);assert(!o.spin_mask && !o.lock_mask);Near(-1,o.expected_mps[1]);
}

static void TestPersistentSingleWheelSpinAndRecovery(void)
{
    Init(2);i.actual_rpm[1]+=2/mps_per_rpm;
    Steps(4);assert(!o.spin_mask);Step();
    assert(o.spin_mask==2 && o.intervened_mask==2);Near(0,o.current[1]);Near(-1000,o.current[0]);
    i.actual_rpm[1]=projection/mps_per_rpm;
    Steps(7);assert(o.spin_mask==2);Step();assert(!o.spin_mask);
    assert(o.current[1]>0 && o.current[1]<1000);
    Steps(40);Near(1000,o.current[1]);assert(!o.intervened_mask);
    /* A short excursion cannot latch. */
    Init(2);i.actual_rpm[1]+=2/mps_per_rpm;Steps(3);
    i.actual_rpm[1]=projection/mps_per_rpm;Steps(10);assert(!o.spin_mask);
}

static void TestShadowAndBoundedOpposingTorque(void)
{
    Init(1);i.actual_rpm[1]+=2/mps_per_rpm;Steps(8);
    assert(o.spin_mask==2 && !o.intervened_mask);Near(1000,o.current[1]);Near(0,o.proposed_current[1]);
    Init(3);i.actual_rpm[1]+=2/mps_per_rpm;i.max_current[1]=72;Steps(8);
    assert(o.active_brake_mask==2);Near(-72,o.current[1]);assert(o.current[1]*i.actual_rpm[1]<0);
    Init(3);i.actual_rpm[0]-=2/mps_per_rpm;Steps(8);Near(300,o.current[0]);
    assert(o.current[0]*i.actual_rpm[0]<0);
    /* Existing stronger braking must not be increased or replaced by weaker braking. */
    i.raw_current[0]=2000;Step();Near(2000,o.current[0]);
    Init(0);i.actual_rpm[1]+=2/mps_per_rpm;Steps(8);
    assert(!o.valid && o.reason==CHASSIS_TRACTION_OFF);Near(1000,o.current[1]);
}

static void TestBrakeLockReleaseAndReversal(void)
{
    Init(3);i.actual_rpm[1]=0;i.raw_current[1]=-2000;
    Steps(8);assert(o.lock_mask==2 && !o.spin_mask && !o.active_brake_mask);Near(0,o.current[1]);
    i.actual_rpm[1]=projection/mps_per_rpm;Steps(8);
    assert(!o.lock_mask && o.current[1]<0 && o.current[1]>-2000);
    Init(3);i.actual_rpm[0]=0;i.raw_current[0]=2000;
    Steps(8);assert(o.lock_mask==1);Near(0,o.current[0]);
    /* Wheel already reversing while body still moves forward: release the
     * reverse/braking demand; never add an accelerating counter-command. */
    Init(3);i.actual_rpm[1]=-.5f/mps_per_rpm;i.raw_current[1]=-2000;
    Steps(8);assert(o.lock_mask==2 && !o.spin_mask && !o.active_brake_mask);Near(0,o.current[1]);
    Init(3);i.reference.vx_mps=.1f;i.actual_rpm[1]=0;i.raw_current[1]=-2000;
    Steps(8);assert(!o.lock_mask);Near(-2000,o.current[1]);
}

static void TestFreshEvidenceAndFaultFallback(void)
{
    Init(2);i.actual_rpm[1]+=2/mps_per_rpm;Step();
    for(unsigned n=0;n<4;++n) {i.now_ms+=4;i.reference.tick_ms=i.reference.gyro_tick_ms=i.now_ms;
        assert(ChassisTractionUpdate(&c,&i,&s,&o));}
    assert(!o.spin_mask); /* No new CAN samples. */
    Steps(8);assert(o.intervened_mask==2);
    i.reference.tick_ms=i.now_ms-41;
    assert(!ChassisTractionUpdate(&c,&i,&s,&o));
    assert(o.reason==CHASSIS_TRACTION_STALE_REFERENCE && !o.intervened_mask);Near(1000,o.current[1]);
    Step();assert(!o.spin_mask); /* Fault clears persistence and recovery history. */
    for(unsigned fault=0;fault<10;++fault) {
        Init(3);i.actual_rpm[1]+=2/mps_per_rpm;Steps(8);
        switch(fault) {
        case 0:i.reference.valid=false;break;
        case 1:i.reference.gyro_tick_ms=i.now_ms-41;break;
        case 2:i.online_mask=7;break;
        case 3:i.enabled_mask=7;break;
        case 4:i.drive_enabled=false;break;
        case 5:i.targets_match=false;break;
        case 6:i.actual_rpm[2]=NAN;break;
        case 7:i.feedback_tick_ms[2]=i.now_ms-21;break;
        case 8:i.dt_s=.006f;break;
        default:c.brake_gain=NAN;break;
        }
        assert(!ChassisTractionUpdate(&c,&i,&s,&o));
        assert(!o.valid && !o.intervened_mask && !o.active_brake_mask);Near(1000,o.current[1]);
    }
    Init(3);c.geometry.wheel_radius_m=FLT_MAX;i.actual_rpm[1]=FLT_MAX;
    assert(!ChassisTractionUpdate(&c,&i,&s,&o));
    Init(3);i.reference.vx_mps=FLT_MAX;c.relative_slip=FLT_MAX;
    assert(!ChassisTractionUpdate(&c,&i,&s,&o));
}

static void TestLatencyAllowanceAndClockWrap(void)
{
    Init(2);c.accel_allowance_mps2=8;i.reference.vx_mps=0;
    for(unsigned w=0;w<4;++w) {i.actual_rpm[w]=.4f/mps_per_rpm;i.raw_current[w]=1000;}
    for(unsigned n=0;n<10;++n) {
        i.now_ms+=5;i.reference.tick_ms=i.now_ms-40;i.reference.gyro_tick_ms=i.now_ms;
        for(unsigned w=0;w<4;++w) {++i.feedback_sequence[w];i.feedback_tick_ms[w]=i.now_ms;}
        assert(ChassisTractionUpdate(&c,&i,&s,&o));
    }
    assert(!o.spin_mask);Near(.57f,o.threshold_mps[0]);
    Init(3);i.now_ms=UINT32_MAX-10;i.actual_rpm[1]+=2/mps_per_rpm;Steps(8);
    assert(o.intervened_mask==2 && o.current[1]<0);
    /* Transport a 90-degree historical velocity into current body axes. */
    Init(1);i.reference.tick_ms=i.now_ms-40;i.reference.wz_radps=1.5707963267948966f/.04f;
    assert(ChassisTractionUpdate(&c,&i,&s,&o));
    const float rotation=i.reference.wz_radps*.325f*projection;
    Near(rotation+projection,o.expected_mps[0]);Near(rotation-projection,o.expected_mps[2]);
}

int main(void)
{
    TestRollingProjectionAndDirections();TestPersistentSingleWheelSpinAndRecovery();
    TestShadowAndBoundedOpposingTorque();TestBrakeLockReleaseAndReversal();
    TestFreshEvidenceAndFaultFallback();TestLatencyAllowanceAndClockWrap();
    puts("PASS: traction spin, braking lock, hysteresis, timing, faults and wheel geometry");return 0;
}
