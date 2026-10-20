(function(root,factory){const node=typeof module==='object'&&module.exports;const api=factory(node?require('./autotune-metrics.js'):root.AutotuneMetrics,node?require('./autotune-budget.js'):root.AutotuneBudget);if(node)module.exports=api;else root.AutotuneStrategy=api;})(globalThis,function(M,B){
  'use strict';
  const check=signal=>{if(signal?.aborted)throw new Error('已停止自动整定。');};
  const point=v=>[Math.fround(Math.min(5,Math.max(.01,v[0]))),Math.fround(Math.min(10,Math.max(0,v[1]))),Math.fround(v[2])];
  const key=v=>v.join(',');
  const rng=seed=>()=>{seed|=0;seed=seed+0x6d2b79f5|0;let t=Math.imul(seed^seed>>>15,1|seed);t^=t+Math.imul(t^t>>>7,61|t);return ((t^t>>>14)>>>0)/4294967296;};
  function speeds(rpm){const out=[.6,1,1.4].map(k=>Math.round(Math.min(1500,Math.max(100,rpm*k))));if(new Set(out).size!==3)throw new Error('请调整搜索转速，以生成不同的低、中、高三档转速。');return out;}
  function integralPoints(kp,ki,kd){return (ki>0?[0,.5,1,2,4].map(k=>[kp,ki*k,kd]):[0,...[.25,.5,1,2].map(t=>kp/t)].map(i=>[kp,i,kd])).map(point);}
  function compare(a,b){const close=Math.abs(a.score-b.score)<=Math.max(Math.min(a.score,b.score)*.01,1e-9);return close?(a.tv-b.tv||a.values[0]-b.values[0]||a.values[1]-b.values[1]):a.score-b.score;}
  async function searchWheel(run,original,options){
    const {wheel,rpm,hold,cap,signal,onTrial=()=>{},onProgress=()=>{}}=options;
    const initial=original.slice(wheel*6,wheel*6+3).map(Math.fround);
    if(initial.length!==3||!initial.every(Number.isFinite)||initial[0]<.01||initial[0]>5||initial[1]<0||initial[1]>10)throw new Error('原 PID 不在 V2 搜索范围内，此配置不适用；不会裁剪原参数。');
    if(![wheel,rpm,hold,cap].every(Number.isInteger)||wheel<0||wheel>3||rpm<100||rpm>1500||hold<500||hold>2000||cap<500||cap>5000)throw new Error('试验配置超出范围。');
    const profiles=speeds(rpm),seed=options.seed??(Date.now()>>>0),random=rng(seed),budget=new B.TrialBudget(options);
    const records=[],tested=new Set([key(initial)]),candidates=[];
    let candidateCount=0,invalid=0,baseline=null,driftDue=false;
    const finish=(reason,more={})=>({wheel,seed,original:initial,values:initial,baseline,records,budget:budget.progress(),recommended:false,reason,boundary:false,scope:'正向、架空验证；反向及带载尚未验证',algorithm:M.rules.version,rules:M.rules,...more});
    async function trial(values,stage,extra=false,details={}){
      check(signal);if(!budget.take(extra))throw Object.assign(new Error('试验预算不足'),{code:'budget'});
      const start=budget.now(),config={...options,...details,stage};onProgress({stage,wheel,rpm:config.rpm,budget:budget.progress()});
      let metrics,error;
      try{metrics=await run(values,config);}catch(e){error=e;metrics={version:2,state:'aborted',reason:signal?.aborted?'stopped':'communication'};}
      budget.durations.push(Math.max(0,budget.now()-start));
      const row={wheel,stage,rpm:config.rpm,hold,cap,values:values.slice(),metrics,evaluation:M.evaluate(metrics,config),...details,index:records.length+1,time:new Date().toISOString()};
      if(error)row.error=error.message;
      records.push(row);onTrial(row);onProgress({stage,wheel,rpm:config.rpm,budget:budget.progress(),valid:records.filter(r=>r.evaluation.valid).length,invalid:records.filter(r=>!r.evaluation.valid).length});
      check(signal);if(error)throw error;
      if(metrics.state!=='done'||['feedback','overspeed','can','guard','timeout','stopped','config','stationary'].includes(metrics.reason))throw Object.assign(new Error('试验中止：'+metrics.reason),{code:'aborted'});
      return row;
    }
    async function controls(){
      if(!driftDue)return;driftDue=false;
      const drifted=row=>!row.evaluation.valid||Math.abs(row.evaluation.score-baseline.median)>Math.abs(baseline.median)*M.rules.spread;
      let row=await trial(initial,'control');
      if(drifted(row)){
        row=await trial(initial,'control',true);
        if(drifted(row))throw Object.assign(new Error('工况漂移'),{code:'drift'});
      }
    }
    async function search(values){
      values=point(values);if(tested.has(key(values))||candidateCount>=18)return;
      await controls();tested.add(key(values));++candidateCount;
      const row=await trial(values,'search');
      invalid=row.evaluation.valid?0:invalid+1;
      if(invalid>=3)throw Object.assign(new Error('连续三个候选无效'),{code:'invalid_candidates'});
      if(row.evaluation.valid)candidates.push({values,rows:[row],score:row.evaluation.score,tv:row.metrics.jitter});
      if(candidateCount%6===0)driftDue=true;
    }
    const best=()=>[{values:initial,score:baseline.median,tv:baseline.tv},...candidates].sort(compare)[0].values;
    try{
      for(let group=0;group<2;++group){
        const rows=[];for(let n=0;n<3;++n)rows.push(await trial(initial,'baseline',group===1,{group}));
        if(rows.some(r=>!r.evaluation.valid))return finish('baseline_invalid');
        baseline={...M.stats(rows.map(r=>r.evaluation.score)),tv:M.median(rows.map(r=>r.metrics.jitter)),metrics:rows.map(r=>r.metrics)};
        if(baseline.spread<=M.rules.spread)break;
        if(group===1)return finish('baseline_unstable');
      }
      for(const k of [.5,.75,1,1.5,2])await search([initial[0]*k,initial[1],initial[2]]);
      const p=best()[0];for(const values of integralPoints(p,initial[1],initial[2]))await search(values);
      const center=best();
      for(const kp of [.8,1,1.2])for(const ki of [.8,1,1.2])await search([center[0]*kp,center[1]*ki,center[2]]);
      if(center[1]===0)for(const values of integralPoints(center[0],0,center[2]))await search(values);
      await controls();
      const top=candidates.filter(c=>c.rows[0].evaluation.passes).sort(compare).slice(0,2);
      if(!top.length)return finish('no_candidate');
      for(const candidate of top){for(let i=0;i<2;++i)candidate.rows.push(await trial(candidate.values,'rescreen'));
        candidate.score=candidate.rows.every(r=>r.evaluation.passes)?M.median(candidate.rows.map(r=>r.evaluation.score)):Infinity;
        candidate.tv=M.median(candidate.rows.map(r=>r.metrics.jitter));}
      const winner=top.sort(compare)[0];if(!Number.isFinite(winner.score))return finish('no_candidate');
      const acceptance=[];
      for(const rpm of profiles)for(let pair=0;pair<3;++pair){
        const order=random()<.5?['original','candidate']:['candidate','original'];
        for(const role of order)acceptance.push(await trial(role==='original'?initial:winner.values,'acceptance',false,{rpm,pair,role}));
      }
      const decision=M.acceptance(acceptance,profiles),boundary=winner.values[0]<=Math.fround(.01)||winner.values[0]>=5||winner.values[1]===0||winner.values[1]>=10;
      return finish(decision.reason,{...decision,values:decision.recommended?winner.values:initial,candidate:winner.values,boundary});
    }catch(error){if(signal?.aborted)throw error;if(error.code)return finish(error.code,{message:error.message});throw error;}
  }
  return {searchWheel,speeds,integralPoints,rng};
});
