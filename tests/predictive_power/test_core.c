#include "power_prediction.h"
#include "power_observer.h"
#include "power_allocator.h"
#include "energy_scheduler.h"
#include "power_supervisor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void near(float a,float b,float e) { assert(isfinite(a) && fabsf(a-b)<=e); }
static PPModel model(void)
{
    /* Synthetic plant, never an on-board calibration. */
    PPModel m={.version=1,.schema=1,.command_scale=1000,.horizon_s=.01f,
        .max_dt_s=.02f,.max_speed=1000,.max_current=30,.max_command=15000,
        .base_w=2,.resistance=1,.mechanical=.1f,.speed_loss=.01f,
        .speed_squared_loss=.00001f,.power_margin_w=1,.speed_error=2,
        .current_error=.01f,.acceleration_bound=100,.braking_valid=false};
    for(unsigned w=0;w<4;++w) {m.current_tau[w]=.01f;m.current_gain[w]=1;m.speed_gain[w]=10;m.speed_drag[w]=1;}
    m.checksum=PPModelChecksum(&m);return m;
}
static void prediction(void)
{
    PPModel m=model();assert(PPModelValid(&m));
    PPState s={.valid=true,.model_version=1};
    float u[4]={1000,2000,3000,4000};PPPrediction p;
    assert(PPPredict(&m,&s,u,.01f,&p));
    near(p.current[0],1-expf(-1),.00001f);
    assert(p.power_upper_w>=p.power_mean_w);
    m.command_delay_ms=10;m.checksum=PPModelChecksum(&m);
    assert(!PPPredict(&m,&s,u,.01f,&p));
    s.applied_valid=true;assert(PPPredict(&m,&s,u,.01f,&p));near(p.current[0],0,0);
    m.command_delay_ms=0;m.checksum=PPModelChecksum(&m);
    for(unsigned k=0;k<100;++k) {
        for(unsigned w=0;w<4;++w) u[w]=(float)((k*137+w*19)%15000);
        assert(PPPredict(&m,&s,u,.01f,&p));
        assert(p.power_upper_w>=p.power_mean_w);
    }
    s.omega[0]=100;u[0]=-100;assert(!PPPredict(&m,&s,u,.01f,&p));
    s.omega[0]=0;u[0]=NAN;assert(!PPPredict(&m,&s,u,.01f,&p));
    m.current_tau[0]=0;m.checksum=PPModelChecksum(&m);assert(!PPModelValid(&m));
    m=model();m.resistance=-1;m.checksum=PPModelChecksum(&m);assert(!PPModelValid(&m));
    m=model();m.version++;assert(!PPModelValid(&m));
    m=model();m.current_tau[0]=.001f;m.command_delay_ms=5;m.max_current=10;
    m.checksum=PPModelChecksum(&m);s=(PPState){.valid=true,.model_version=1,.applied_valid=true};
    s.applied_command[0]=15000;memset(u,0,sizeof u);
    assert(!PPPredict(&m,&s,u,.01f,&p)); /* Delay peak exceeds domain, endpoint does not. */
    m.max_current=30;m.checksum=PPModelChecksum(&m);s.omega[0]=100;s.applied_command[0]=-15000;u[0]=1000;
    assert(!PPPredict(&m,&s,u,.01f,&p)); /* Uncalibrated braking during delay. */
}
static void history(void)
{
    PPObserver o={0};PPFinalActuation f={.tick_ms=UINT32_MAX-5,.submitted=true,.model_version=1};
    f.command[0]=12;assert(PPObserverRecord(&o,&f));
    f.tick_ms=1;f.command[0]=99;f.submitted=false;assert(PPObserverRecord(&o,&f));
    PPFinalActuation found;
    assert(PPObserverCommandAt(&o,2,20,&found));assert(found.command[0]==12);
    assert(!PPObserverCommandAt(&o,30,20,&found));
    f.tick_ms=0;assert(!PPObserverRecord(&o,&f)); /* Out of order. */
    PPMeasurement a={.now_ms=2,.online_mask=15,.model_version=1};
    for(unsigned w=0;w<4;++w) {a.rx_ms[w]=2;a.sequence[w]=10;a.omega[w]=5;}
    assert(PPObserverUpdate(&o,&a));assert(o.state.valid);
    a.omega[0]=99;assert(PPObserverUpdate(&o,&a));near(o.state.omega[0],5,0);
    a.sequence[0]=9;assert(!PPObserverUpdate(&o,&a));
    a.sequence[0]=11;a.now_ms=30;assert(!PPObserverUpdate(&o,&a));
    a.now_ms=2;a.model_version=2;assert(PPObserverUpdate(&o,&a));
    assert(o.state.model_version==2);
    a.current[1]=NAN;assert(!PPObserverUpdate(&o,&a));
    o=(PPObserver){0};f=(PPFinalActuation){.submitted=true,.model_version=1,.config_version=1};
    for(unsigned t=100;t<=110;t+=2) {f.tick_ms=t;f.command[0]=10;assert(PPObserverRecord(&o,&f));}
    assert(PPObserverStableCommand(&o,112,10,1,1,&found));
    f.tick_ms=112;f.command[0]=11;assert(PPObserverRecord(&o,&f));
    assert(!PPObserverStableCommand(&o,114,10,1,1,&found));
    assert(!PPObserverStableCommand(&o,114,10,2,1,&found));
}
static PPAllocationProblem problem(void)
{
    PPAllocationProblem p={.scale=1000,.budget_w=4};
    for(unsigned w=0;w<4;++w) {p.q[w][w]=1;p.linear[w]=-4;p.low[w]=0;p.high[w]=10000;p.power_h[w]=1;}
    return p;
}
static void allocation(void)
{
    PPAllocationProblem p=problem();PPAllocationResult r;
    assert(PPAllocate(&p,&r));assert(r.feasible);
    for(unsigned w=0;w<4;++w) near(r.command[w],1000,1);
    assert(r.power_w<=4.00001f && r.residual<.01f);
    /* Analytic independent solution of min ||x-2||², ||x||² <= 4. */
    near(r.objective,-12,.02f);
    p.budget_w=100;assert(PPAllocate(&p,&r));
    for(unsigned w=0;w<4;++w) near(r.command[w],2000,1);
    p.high[0]=400;assert(PPAllocate(&p,&r));assert(r.command[0]<=400);
    p.low[1]=p.high[1]=0;assert(PPAllocate(&p,&r));assert(r.command[1]==0);
    p=problem();p.power_c=5;assert(!PPAllocate(&p,&r));assert(!r.feasible);
    /* Nonmonotonic power: zero consumes more than an interior integer. */
    p=problem();p.scale=1;p.budget_w=.01f;p.power_c=4;
    for(unsigned w=0;w<4;++w) {p.power_g[w]=-2;p.linear[w]=-2;p.high[w]=2;}
    assert(PPAllocate(&p,&r));for(unsigned w=0;w<4;++w) assert(r.command[w]==1);
    p.q[0][0]=-1;assert(!PPAllocate(&p,&r));
    p=problem();p.power_h[0]=NAN;assert(!PPAllocate(&p,&r));
    /* Fractional boxes contain no integer candidate. */
    p=problem();p.low[0]=.1f;p.high[0]=.9f;assert(!PPAllocate(&p,&r));
    p=problem();p.budget_w=1000;
    p.q[0][1]=p.q[1][0]=-.999f;p.linear[0]=p.linear[1]=-.2f;
    assert(PPAllocate(&p,&r));
    assert(r.command[0]==10000 && r.command[1]==10000);
    /* Coupled exact minimizer is the upper corner, last two wheels remain 2. */
    near(r.objective,-11.8f,.01f);assert(r.residual<.001f);
}
static void energy(void)
{
    PPEnergyConfig c={.buffer_max_j=60,.buffer_floor_j=5,.buffer_terminal_j=10,
        .cap_max_j=100,.cap_floor_j=5,.cap_terminal_j=10,.eta_charge=.8f,.eta_discharge=.8f,
        .charge_max_w=50,.discharge_max_w=100,.input_max_w=120,.load_max_w=200,
        .input_slew_w_s=1000,.path_loss_w=2,.leak_w=1};
    PPEnergyInput i={.now_ms=100,.strategy_version=1,.model_version=1,.capability_version=1,
        .permitted=true,.buffer_valid=true,.buffer_low_j=40,.buffer_high_j=40,
        .referee_w=50,.demand_w=80,.current_request_w=50};
    PPEnergyPlan p={.strategy_version=1,.model_version=1,.capability_version=1};PPEnergyTrace t;
    for(unsigned k=0;k<5;++k) p.load_w[k]=50;
    assert(PPEnergyCheck(&c,&i,&p,&t));near(t.buffer_low_j,39,.001f);
    p.strategy_version=2;assert(!PPEnergyCheck(&c,&i,&p,&t));p.strategy_version=1;
    assert(PPEnergySchedule(&c,&i,&p));assert(p.valid && p.load_w[0]>50);
    assert(PPEnergyCheck(&c,&i,&p,&t));
    i.capacitor=true;i.cap_valid=true;i.strong_confirmation=true;
    i.cap_low_j=i.cap_high_j=70;i.input_low_w=i.input_high_w=50;
    for(unsigned k=0;k<5;++k) {p.load_w[k]=80;p.request_w[k]=50;}
    assert(PPEnergyCheck(&c,&i,&p,&t));near(t.cap_low_j,49.5f,.005f);
    i.cap_valid=false;assert(!PPEnergyCheck(&c,&i,&p,&t));i.cap_valid=true;
    i.strong_confirmation=false;assert(!PPEnergyCheck(&c,&i,&p,&t));
    assert(PPEnergySchedule(&c,&i,&p));assert(p.load_w[0]<=48);
    i.strong_confirmation=true;i.cap_low_j=i.cap_high_j=99;
    for(unsigned k=0;k<5;++k) {p.load_w[k]=0;p.request_w[k]=50;}
    assert(!PPEnergyCheck(&c,&i,&p,&t)); /* Low-load overcharge scenario. */
    c.eta_discharge=NAN;assert(!PPEnergySchedule(&c,&i,&p));
}
static void supervision(void)
{
    PPAllocationProblem p=problem();PPAllocationResult result;assert(PPAllocate(&p,&result));
    PPSupervisorInput i={.now_ms=100,.allowance_ms=90,.permitted=true,.model_valid=true,
        .model_version=1,.allowance_model_version=1,.config_version=2,.allowance_config_version=2,
        .capability_version=3,.allowance_capability_version=3,.elapsed_us=10};
    PPSupervisorOutput o;assert(PPSupervise(&i,&p,&result,&o));assert(o.mode==PP_BASE_ONLY);
    i.now_ms=111;assert(!PPSupervise(&i,&p,&result,&o));assert(o.reason==PP_STALE_ALLOWANCE);
    i.now_ms=100;i.allowance_model_version=2;assert(!PPSupervise(&i,&p,&result,&o));
    i.allowance_model_version=1;i.permitted=false;assert(!PPSupervise(&i,&p,&result,&o));
    i.permitted=true;i.strategy_version=1;assert(!PPSupervise(&i,&p,&result,&o));
    i.strategy_version=0;i.permitted=false;assert(!PPSupervise(&i,&p,&result,&o));
    for(unsigned w=0;w<4;++w) assert(o.command[w]==0);
    i.permitted=true;i.elapsed_us=301;i.baseline_valid=true;
    assert(PPSupervise(&i,&p,&result,&o));assert(o.reason==PP_TIMEOUT && o.fallback);
    p.power_c=10;assert(!PPSupervise(&i,&p,&result,&o));assert(!o.feasible);
}
int main(void)
{
    prediction();history();allocation();energy();supervision();
    puts("PASS: prediction, command history and integer QCQP constraints");return 0;
}
