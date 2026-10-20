const {test}=require('node:test');
const assert=require('node:assert/strict');
const A=require('../autotune.js');
const {rng}=require('../autotune-strategy.js');

// Synthetic first-order wheel, command -> rpm. The PI error is in degrees/s,
// as in the real speed controller. This fixture is not identified hardware.
function plant(seed,{noise=0,drift=0}={}){
  const random=rng(seed);let trials=0;
  return async([kp,ki],o)=>{
    ++trials;const dt=.001,tau=.09,gain=.5*(1+drift*trials);
    let rpm=0,integral=0,error2=0,steady=0,variation=0,previous=0,peak=0,outside=0,limited=0;
    for(let t=0;t<o.hold;++t){
      const measured=rpm+(random()-.5)*noise,error=(o.rpm-measured)*6;
      integral=Math.max(-o.cap,Math.min(o.cap,integral+ki*error*dt));
      const raw=kp*error+integral,command=Math.max(-o.cap,Math.min(o.cap,raw));
      if(Math.abs(raw-command)>Math.max(5,Math.abs(raw)*.05))++limited;
      if(t)variation+=Math.abs(command-previous);previous=command;
      const absolute=Math.abs(o.rpm-measured);error2+=absolute*absolute;
      if(t>=o.hold*.8)steady+=absolute;
      if(absolute>Math.max(o.rpm*.05,10))outside=t+1;
      peak=Math.max(peak,measured);rpm+=(gain*command-rpm)*dt/tau;
    }
    return {version:2,state:'done',reason:limited/o.hold>.05?'limited':'none',samples:o.hold,feedbackCount:o.hold,
      window:o.hold,coverage:o.hold,maxFeedback:1,maxStat:1,statLimit:4,currentLimit:o.cap,
      rmse:Math.sqrt(error2/o.hold),overshoot:Math.max(0,peak-o.rpm),steady:steady/(o.hold*.2),
      settle:outside+100<=o.hold?outside:0xffffffff,jitter:variation/(o.hold/1000),limited:Math.floor(limited/o.hold*1000)};
  };
}
const options={wheel:0,rpm:500,hold:1000,cap:5000,seed:123};
const pid=Array.from({length:4},()=>[4,0,0,15000,3000,0]).flat();
test('dynamic seeded wheel simulation can find improvement from a poor integral setting',async()=>{
  const result=await A.searchWheelV2(plant(17),pid,options);
  assert.equal(result.recommended,true,JSON.stringify({reason:result.reason,comparisons:result.comparisons}));
  assert(result.values[1]>0);assert.equal(result.records.filter(r=>r.stage==='acceptance').length,18);
});
test('dynamic noise and changing plant cannot bypass repeatability and drift rejection',async()=>{
  const result=await A.searchWheelV2(plant(17,{noise:1,drift:.15}),pid,options);
  assert.equal(result.recommended,false);
  assert(['baseline_unstable','drift','no_candidate','no_improvement'].includes(result.reason),result.reason);
});
