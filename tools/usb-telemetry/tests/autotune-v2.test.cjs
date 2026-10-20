const {test}=require('node:test');
const assert=require('node:assert/strict');
const A=require('../autotune.js');
const pid=Array.from({length:4},()=>[.75,0,0,15000,3000,0]).flat();
const options={wheel:0,rpm:500,hold:1000,cap:3000,seed:123};
function metric(error=30,extra={}){return {version:2,state:'done',reason:'none',samples:1000,feedbackCount:500,
  window:1000,coverage:1000,maxFeedback:2,maxStat:1,statLimit:4,currentLimit:3000,
  rmse:error,overshoot:0,steady:3,settle:200,jitter:100,limited:0,...extra};}
test('A2 weighted score and hard gates are independent',()=>{
  assert.equal(typeof A.evaluate,'function');
  const e=A.evaluate(metric(100),options);
  assert(Math.abs(e.score-(.45*.2+.15*.006+.1*.2+.05*100/3000))<1e-10);
  assert.equal(e.passes,true);
  assert.equal(A.evaluate(metric(1,{overshoot:51}),options).passes,false);
  assert.equal(A.evaluate(metric(1,{coverage:949}),options).valid,false);
  assert.equal(A.evaluate(metric(1,{maxStat:5}),options).valid,false);
});
test('seeded search escapes Ki=0 and uses eighteen fresh paired acceptance trials',async()=>{
  assert.equal(typeof A.searchWheelV2,'function');const rows=[];
  const result=await A.searchWheelV2(async(values,o)=>metric(15+Math.abs(values[0]-1.5)*95+Math.abs(values[1]-1.5)*18),pid,{...options,onTrial:r=>rows.push(r)});
  assert.equal(result.recommended,true);assert(result.values[1]>0);assert.equal(result.values[2],0);
  assert.equal(result.comparisons[0].gates.overshoot.threshold,30);
  assert.equal(result.comparisons[0].gates.steady.threshold,10);
  assert.equal(rows.filter(r=>r.stage==='acceptance').length,18);
  assert.equal(rows.filter(r=>r.stage==='baseline').length,3);
  assert(rows.length<=50);assert(result.improvement>=.1);
  const search=rows.filter(r=>r.stage==='search');assert(search.length<=18);
  assert.equal(new Set(search.map(r=>r.values.join(','))).size,search.length);
  assert.deepEqual([...new Set(rows.filter(r=>r.stage==='acceptance').map(r=>r.rpm))].sort((a,b)=>a-b),[300,500,700]);
});
test('noisy baseline stops after one complete retry and retains all records',async()=>{
  let calls=0;const result=await A.searchWheelV2(async()=>metric(++calls%3===0?150:50),pid,options);
  assert.equal(result.recommended,false);assert.equal(result.reason,'baseline_unstable');assert.equal(calls,6);assert.equal(result.records.length,6);
});
test('good original gains, one lucky score and cross-speed degradation cannot be recommended',async()=>{
  for(const scenario of ['good','lucky','highSpeed','currentTV']){
    let n=0;
    const result=await A.searchWheelV2(async(values,o)=>{
      ++n;const original=values[0]===.75&&values[1]===0;
      if(scenario==='good')return metric(original?10:40);
      if(scenario==='lucky')return metric(n===4?1:original?50:65);
      return metric(original?100:o.rpm===700&&scenario==='highSpeed'?120:10,{jitter:scenario==='currentTV'&&!original?120:100});
    },pid,options);
    assert.equal(result.recommended,false,scenario);
  }
});
test('drift and three invalid candidates terminate without combining conditions',async()=>{
  let controls=0;
  const drift=await A.searchWheelV2(async(v,o)=>metric(o.stage==='control'&&++controls?180:100),pid,options);
  assert.equal(drift.reason,'drift');assert.equal(drift.recommended,false);
  const limited=await A.searchWheelV2(async(v,o)=>metric(100,{reason:o.stage==='search'?'limited':'none',limited:o.stage==='search'?100:0}),pid,options);
  assert.equal(limited.reason,'invalid_candidates');assert.equal(limited.records.filter(r=>r.stage==='search').length,3);
  const low=await A.searchWheelV2(async(v,o)=>metric(o.stage==='control'?26:20,{steady:0,settle:0,jitter:0}),pid,options);
  assert.equal(low.reason,'drift','drift uses relative baseline even below the spread denominator floor');
});
test('a gain at the search edge is reported without silently widening bounds',async()=>{
  const original=pid.slice();original[0]=4;
  const r=await A.searchWheelV2(async values=>metric(20+Math.abs(values[0]-5)*100),original,options);
  assert.equal(r.recommended,true);assert.equal(r.values[0],5);assert.equal(r.boundary,true);
  assert(r.records.every(row=>row.values[0]<=5));
  const zero=await A.searchWheelV2(async values=>metric(20+Math.abs(values[0]-1.5)*100+values[1]*30),pid,options);
  assert.equal(zero.recommended,true);assert.equal(zero.values[1],0);assert.equal(zero.boundary,true);
});
test('budget ends exploration before expiry, cancellation throws and baseline is never clipped',async()=>{
  let now=0;
  const result=await A.searchWheelV2(async()=>{now+=40000;return metric();},pid,{...options,now:()=>now});
  assert.equal(result.recommended,false);assert.equal(result.reason,'budget');
  const signal=new AbortController();let calls=0;
  await assert.rejects(A.searchWheelV2(async()=>{++calls;signal.abort();return metric();},pid,{...options,signal:signal.signal}),/停止/);assert.equal(calls,1);
  const bad=pid.slice();bad[0]=0;
  await assert.rejects(A.searchWheelV2(async()=>{throw Error('must not run');},bad,options),/范围|适用/);
  await assert.rejects(A.searchWheelV2(async()=>metric(),pid,{...options,rpm:100}),/三档/);
});
test('A2 negotiates capability and ignores obsolete session/trial echoes',async()=>{
  const client=new A.AutotuneClient({send:()=>{},timeoutMs:100});
  assert.equal(typeof client.capabilities,'function');const cap=client.capabilities();
  client.receive('@A2,1,CAP,2,31,180000,1000,20');assert.equal((await cap).version,2);
  const begin=client.begin(1);client.receive('@A2,2,OK,9,ready,0,0,none,0,0,0,4294967295,0,0,0,0,0,0,0,0,20,0,00000000,00000000,00000000,0,0');await begin;
  const trial=client.trial(9,[.75,0,0],options);
  const reply='@A2,3,OK,9,running,0,1,none,0,0,0,4294967295,0,0,0,0,0,0,0,0,20,3000,3f400000,00000000,00000000,0,0';
  client.receive(reply.replace(',OK,9,',',OK,8,'));assert(client.pending);
  client.receive(reply.replace(',0,1,none,',',0,2,none,'));assert(client.pending);
  client.receive(reply);assert.equal((await trial).trial,1);
  const ended=client.end(9);client.receive('@A2,4,OK,9,aborted,0,1,stopped,0,0,0,4294967295,0,0,0,0,0,0,0,0,20,3000,3f400000,00000000,00000000,0,0');await ended;
  const next=client.begin(1);client.receive('@A2,5,OK,10,ready,0,0,none,0,0,0,4294967295,0,0,0,0,0,0,0,0,20,0,00000000,00000000,00000000,0,0');assert.equal((await next).session,10);
});
test('reports preserve failures and CSV distinguishes output TV from measured ripple',()=>{
  assert.equal(typeof A.reportCSV,'function');const csv=A.reportCSV([{wheel:0,stage:'search',rpm:500,values:[.75,0,0],metrics:metric(30,{reason:'limited'}),evaluation:{valid:false,passes:false,failures:['limited']}}]);
  assert.match(csv,/current_tv_raw_per_s/);assert.match(csv,/limited/);
});
test('transport failure and cancellation keep the attempted trial in the report',async()=>{
  const rows=[];
  await assert.rejects(A.searchWheelV2(async()=>{throw new Error('USB lost');},pid,{...options,onTrial:r=>rows.push(r)}),/USB lost/);
  assert.equal(rows.length,1);assert.equal(rows[0].evaluation.valid,false);assert.match(rows[0].error,/USB lost/);
  const aborter=new AbortController(),cancelled=[];
  await assert.rejects(A.searchWheelV2(async()=>{aborter.abort();return metric();},pid,{...options,signal:aborter.signal,onTrial:r=>cancelled.push(r)}),/停止/);
  assert.equal(cancelled.length,1);
});
