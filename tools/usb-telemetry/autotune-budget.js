(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.AutotuneBudget=api;})(globalThis,function(){
  'use strict';
  class TrialBudget{
    constructor({hold,now=()=>performance.now(),elapsed=0}){this.hold=hold;this.now=now;this.start=now()-elapsed;this.used=0;this.extra=0;this.durations=[];}
    get remainingMs(){return Math.max(0,180000-(this.now()-this.start));}
    get trialMs(){return Math.max(this.hold+6800,...this.durations.map(d=>d*1.25));}
    canRun(){return this.used<50&&this.remainingMs>=this.trialMs+2500;}
    take(extra=false){if(!this.canRun()||(extra&&this.extra>=4))return false;++this.used;if(extra)++this.extra;return true;}
    progress(){const remaining=50-this.used,mean=this.durations.length?this.durations.reduce((a,b)=>a+b,0)/this.durations.length:this.hold+1000;return {used:this.used,remaining,extra:this.extra,remainingMs:this.remainingMs,estimatedMs:remaining*mean};}
  }
  return {TrialBudget};
});
