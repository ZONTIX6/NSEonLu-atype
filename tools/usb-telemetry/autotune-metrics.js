(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.AutotuneMetrics=api;})(globalThis,function(){
  'use strict';
  const rules=Object.freeze({version:'pid-autotune-v2.0',weights:Object.freeze([.45,.25,.15,.10,.05]),spread:.15,improvement:.10,regression:.05,tvRegression:.10,scoreFloor:.01,maxCandidates:18,maxTrials:50,maxExtra:4,
    overshoot:.10,steady:.03,steadyFloor:10,settle:.80,stableTail:100,coverage:.95,limited:50,maxFeedback:20});
  const median=values=>{const a=values.slice().sort((a,b)=>a-b),n=a.length;return n?(a[Math.floor(n/2)]+a[Math.floor((n-1)/2)])/2:NaN;};
  function stats(values){const med=median(values),min=Math.min(...values),max=Math.max(...values);return {median:med,min,max,spread:(max-min)/Math.max(med,.05)};}
  function evaluate(m,{rpm,hold,cap}){
    const finite=['rmse','overshoot','steady','jitter','settle','limited','samples','window','coverage','feedbackCount','maxFeedback','maxStat','statLimit','currentLimit'].every(k=>Number.isFinite(m[k])&&m[k]>=0);
    const valid=m.version===2&&m.state==='done'&&m.reason==='none'&&finite&&rpm>0&&hold>0&&cap>0&&m.currentLimit>0&&m.currentLimit<=cap&&m.samples>=20&&m.feedbackCount>=20&&m.feedbackCount<=m.samples+1&&m.window===hold&&m.coverage>=hold*rules.coverage&&m.coverage<=hold&&m.maxFeedback<=20&&m.maxStat<=m.statLimit&&m.statLimit>0&&m.statLimit<=20&&m.limited<=50;
    const failures=[];
    if(!valid)failures.push(m.reason!=='none'?m.reason:'quality');
    if(m.overshoot>rpm*rules.overshoot)failures.push('overshoot');
    if(m.steady>Math.max(rpm*rules.steady,rules.steadyFloor))failures.push('steady');
    if(m.settle===0xffffffff||m.settle>hold*rules.settle||hold-m.settle<rules.stableTail)failures.push('settle');
    const score=valid ? .45*m.rmse/rpm+.25*m.overshoot/rpm+.15*m.steady/rpm+.10*(m.settle===0xffffffff?1:m.settle/hold)+.05*m.jitter/m.currentLimit : Infinity;
    return {valid,passes:valid&&!failures.length,score,failures};
  }
  function acceptance(records,speeds){
    const comparisons=speeds.map(rpm=>{
      const rows=records.filter(r=>r.rpm===rpm),base=rows.filter(r=>r.role==='original'),candidate=rows.filter(r=>r.role==='candidate');
      const original=stats(base.map(r=>r.evaluation.score)),proposed=stats(candidate.map(r=>r.evaluation.score));
      const originalTV=stats(base.map(r=>r.metrics.jitter)),candidateTV=stats(candidate.map(r=>r.metrics.jitter));
      const pairs=[0,1,2].filter(pair=>candidate.find(r=>r.pair===pair)?.evaluation.score<base.find(r=>r.pair===pair)?.evaluation.score).length;
      const failures=[];
      if(base.length!==3||candidate.length!==3||base.some(r=>!r.evaluation.valid)||candidate.some(r=>!r.evaluation.passes))failures.push('hard_gates');
      if(original.spread>rules.spread||proposed.spread>rules.spread)failures.push('repeatability');
      if(proposed.median>original.median*(1+rules.regression))failures.push('speed_regression');
      if(pairs<2)failures.push('paired_wins');
      if(candidateTV.median>originalTV.median*(1+rules.tvRegression))failures.push('output_variation');
      const gates={};for(const [name,threshold] of [['overshoot',rpm*rules.overshoot],['steady',Math.max(rpm*rules.steady,rules.steadyFloor)],['settle',(candidate[0]?.hold||0)*rules.settle],['limited',rules.limited]])
        gates[name]={threshold,original:stats(base.map(r=>r.metrics[name])),candidate:stats(candidate.map(r=>r.metrics[name]))};
      return {rpm,original,candidate:proposed,originalTV,candidateTV,pairedWins:pairs,gates,failures};
    });
    const base=comparisons.reduce((s,c)=>s+c.original.median,0)/3,proposed=comparisons.reduce((s,c)=>s+c.candidate.median,0)/3;
    const improvement=base>=rules.scoreFloor?(base-proposed)/base:0;
    const recommended=comparisons.length===3&&comparisons.every(c=>!c.failures.length)&&base>=rules.scoreFloor&&improvement>=rules.improvement;
    return {comparisons,improvement,recommended,reason:base<rules.scoreFloor?'score_floor':recommended?'accepted':'no_improvement'};
  }
  return {rules,median,stats,evaluate,acceptance};
});
