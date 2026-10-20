(function(root,factory){const api=factory(typeof module==='object'&&module.exports ? require('./tuning.js') : root.TelemetryTuning);if(typeof module==='object'&&module.exports) module.exports=api;else root.TelemetryAutotune=api;})(typeof globalThis!=='undefined'?globalThis:this,function(T){
  'use strict';
  function check(signal){if(signal?.aborted) throw new Error('已停止自动整定。');}
  function candidates(values,wheel){
    const [kp,ki,kd]=values.slice(wheel*6,wheel*6+3),rows=[];
    for(const p of [kp*.75,kp,kp*1.25]) for(const i of [ki,0,.5,1,2]) {
      const row=[...([Math.min(5,Math.max(.01,p)),Math.min(10,Math.max(0,i))].map(v=>T.decodeFloat(T.encodeFloat(v)))),kd];
      if(!rows.some(r=>r.every((v,j)=>v===row[j]))) rows.push(row);
    }
    return rows;
  }
  function score(m,rpm,hold,cap){
    if(m.state!=='done'||m.reason!=='none'||m.samples<20||m.limited>50||![m.rmse,m.overshoot,m.steady,m.jitter].every(Number.isFinite)) return Infinity;
    return m.rmse/rpm+2*m.overshoot/rpm+m.steady/rpm+.1*(m.settle===0xffffffff?1:m.settle/hold)+.1*m.jitter/cap;
  }
  async function searchWheel(run,original,options){
    const {wheel,rpm,hold,cap,signal,onTrial=()=>{}}=options;
    check(signal);const initial=original.slice(wheel*6,wheel*6+3),baseline=await run(initial,options);
    check(signal);onTrial({wheel,stage:'baseline',values:initial,metrics:baseline});
    const baseScore=score(baseline,rpm,hold,cap);
    if(!Number.isFinite(baseScore)) throw new Error('基线试验无效（'+baseline.reason+'），请降低测试转速或检查电机反馈。');
    let best={values:initial,metrics:baseline,score:baseScore};
    for(const values of candidates(original,wheel)) {
      check(signal);if(values.every((v,i)=>v===initial[i])) continue;
      const metrics=await run(values,options);check(signal);const value=score(metrics,rpm,hold,cap);
      onTrial({wheel,stage:'search',values,metrics});if(value<best.score) best={values,metrics,score:value};
    }
    check(signal);
    if(best.values===initial) return {wheel,baseline,validation:null,values:initial,recommended:false};
    const validation=await run(best.values,options);check(signal);onTrial({wheel,stage:'verify',values:best.values,metrics:validation});
    const recommended=score(validation,rpm,hold,cap)<baseScore*.95;
    return {wheel,baseline,validation,values:recommended?best.values:initial,recommended};
  }
  function mergeResults(original,results){const values=original.slice();for(const r of results) if(r.recommended) values.splice(r.wheel*6,3,...r.values);return values;}
  const messages={format:'自动整定协议格式错误。',guard:'台架条件未满足，请检查遥控器模式、摇杆居中和电机静止状态。',conflict:'参数版本改变或手动操作尚未结束，请重新开始。',busy:'板端已有试调会话。',session:'试调会话已失效。',expired:'试调已停止，请先进入零力模式再重新准备。',config:'本轮参数无效或电机尚未停止。'};
  class AutotuneClient {
    constructor({send,timeoutMs=2000}){this.send=send;this.timeoutMs=timeoutMs;this.sequence=0;this.pending=null;this.version=1;this.session=0;this.trialNo=0;this.expected=null;}
    request(operation,...fields){
      if(this.pending) return Promise.reject(new Error('自动整定请求尚未完成。'));
      const id=++this.sequence;
      return new Promise((resolve,reject)=>{
        const finish=(error,result)=>{if(this.pending?.id!==id)return;clearTimeout(this.pending.timer);this.pending=null;error?reject(error):resolve(result);};
        const timer=setTimeout(()=>finish(new Error('自动整定响应超时，板端会按心跳期限停止；旧固件需升级。')),this.timeoutMs);
        const version=['CAP','CONFIG'].includes(operation)?2:this.version;
        this.pending={id,finish,timer,version,operation,fields};
        try{Promise.resolve(this.send(['@A'+version,id,operation,...fields].join(',')+'\r\n')).catch(e=>finish(e));}catch(e){finish(e);}
      });
    }
    begin(revision){return this.request('BEGIN',revision);}
    trial(session,values,{wheel,rpm,hold,cap}){return this.request('TRIAL',session,wheel,rpm,hold,cap,...values.map(T.encodeFloat));}
    status(session){return this.request('STATUS',session);}
    end(session){return this.request('END',session);}
    async capabilities(){const cap=await this.request('CAP');if(cap.version!==2||(cap.flags&31)!==31||cap.sessionMs!==180000||cap.leaseMs!==1000||cap.maxFeedback!==20)throw new Error('固件未提供完整 V2 能力，请升级；旧版结果不能用于 V2 验收。');this.version=2;this.session=0;this.trialNo=0;this.expected=null;return cap;}
    configuration(wheel){return this.request('CONFIG',wheel);}
    receive(line){
      if(!line.startsWith('@A1,')&&!line.startsWith('@A2,'))return false;
      const c=line.split(','),p=this.pending;if(!p||c[1]!==String(p.id))return true;
      if(c[0]!=='@A'+p.version)return true;
      if(c[2]==='ERR'){p.finish(new Error(messages[c[3]]||'试调被拒绝：'+c[3]));return true;}
      if(p.version===2){this.receiveV2(c,p);return true;}
      const indexes=[3,5,6,8,9,10,11,12,13,14];
      if(c.length!==15||c[2]!=='OK'||!['idle','ready','running','done','aborted'].includes(c[4])||
        !['none','stopped','timeout','guard','feedback','overspeed','limited','no_response','config','can'].includes(c[7])||
        indexes.some(i=>!/^\d+$/.test(c[i])||Number(c[i])>0xffffffff)||Number(c[5])>3||Number(c[14])>1000) {
        p.finish(new Error('自动整定回复格式错误。'));return true;
      }
      p.finish(null,{session:+c[3],state:c[4],wheel:+c[5],trial:+c[6],reason:c[7],samples:+c[8],rmse:+c[9]/1000,overshoot:+c[10]/1000,settle:+c[11],steady:+c[12]/1000,jitter:+c[13]/1000,limited:+c[14]});return true;
    }
    receiveV2(c,p){
      const unsigned=v=>/^\d+$/.test(v)&&Number(v)<=0xffffffff;
      const fail=()=>p.finish(new Error('自动整定 V2 回复格式或参数快照错误。'));
      if(p.operation==='CAP'){
        if(c.length!==8||c[2]!=='CAP'||c.slice(3).some(v=>!unsigned(v)))return fail();
        p.finish(null,{version:+c[3],flags:+c[4],sessionMs:+c[5],leaseMs:+c[6],maxFeedback:+c[7]});return;
      }
      if(p.operation==='CONFIG'){
        if(c.length!==12||c[2]!=='CONFIG'||!unsigned(c[3])||+c[3]!==p.fields[0]||!unsigned(c[4])||!unsigned(c[11]))return fail();
        try{const values=c.slice(5,11).map(T.decodeFloat);p.finish(null,{wheel:+c[3],improve:+c[4],coefA:values[0],coefB:values[1],outputRC:values[2],derivativeRC:values[3],speedSmooth:values[4],currentSmooth:values[5],motorFlags:+c[11]});}catch(_){fail();}return;
      }
      const indexes=[3,5,6,8,9,10,11,12,13,14,15,16,17,18,19,20,21,25,26];
      if(c.length!==27||c[2]!=='OK'||indexes.some(i=>!unsigned(c[i]))||+c[5]>3||+c[14]>1000||+c[25]>2||+c[20]>20||!['idle','ready','running','done','aborted'].includes(c[4])||!['none','stopped','timeout','guard','feedback','overspeed','limited','no_response','config','can','quality','stationary'].includes(c[7]))return fail();
      if(p.operation!=='BEGIN'&&(+c[3]!==p.fields[0]||+c[6]!==this.trialNo+(p.operation==='TRIAL'?1:0)))return;
      let values;try{values=c.slice(22,25).map(T.decodeFloat);}catch(_){return fail();}
      const expected=p.operation==='BEGIN'?null:p.operation==='TRIAL'?{wheel:p.fields[1],values:p.fields.slice(5).map(T.decodeFloat)}:this.expected;
      if(expected&&(+c[5]!==expected.wheel||values.some((v,i)=>v!==expected.values[i])))return fail();
      if(p.operation==='BEGIN'){
        if(+c[3]===0||+c[6]!==0||c[4]!=='ready')return fail();this.session=+c[3];this.trialNo=0;this.expected=null;
      }
      if(p.operation==='TRIAL'){this.trialNo=+c[6];this.expected=expected;}
      p.finish(null,{version:2,session:+c[3],state:c[4],wheel:+c[5],trial:+c[6],reason:c[7],samples:+c[8],rmse:+c[9]/1000,overshoot:+c[10]/1000,settle:+c[11],steady:+c[12]/1000,jitter:+c[13]/1000,limited:+c[14],window:+c[15],coverage:+c[16],feedbackCount:+c[17],maxFeedback:+c[18],maxStat:+c[19],statLimit:+c[20],currentLimit:+c[21],values,phase:+c[25],elapsed:+c[26]});
    }
    disconnect(){this.pending?.finish(new Error('串口已断开，板端将停止试调；请重连读取实际参数。'));this.version=1;this.session=0;this.trialNo=0;this.expected=null;}
  }
  const node=typeof module==='object'&&module.exports,M=node?require('./autotune-metrics.js'):globalThis.AutotuneMetrics,S=node?require('./autotune-strategy.js'):globalThis.AutotuneStrategy,R=node?require('./autotune-report.js'):globalThis.AutotuneReport;
  return {candidates,score,searchWheel,mergeResults,AutotuneClient,check,evaluate:M.evaluate,rules:M.rules,searchWheelV2:S.searchWheel,reportCSV:R.reportCSV};
});
