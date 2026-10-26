(function(root,factory){const api=factory(typeof module==='object'&&module.exports?require('./force-calibration-protocol.js'):root.ForceCalibrationProtocol);if(typeof module==='object'&&module.exports)module.exports=api;else root.ForceCalibrationClient=api;})(globalThis,function(P){
  'use strict';
  class Client{
    constructor({send,isConnected=()=>false,onStatus=()=>{},onError=()=>{},timeout_ms=600}){
      this.send=send;this.isConnected=isConnected;this.connected=isConnected();this.onStatus=onStatus;this.onError=onError;this.timeout_ms=timeout_ms;
      this.nextId=0;this.queue=[];this.pending=null;this.caps=null;this.latest=null;this.pulse=null;this.lease=null;this.heartbeat=null;
      this.parser=P.createParser({onReply:r=>this.receive(r),onError:e=>this.onError(e)});
    }
    setConnected(value){this.connected=value;this.endLease();this.cancelAll(Error('连接世代已改变'));this.caps=null;this.latest=null;this.pulse=null;this.pulseCaptureId=null;this.parser.reset();}
    receiveLine(line){try{const r=P.parseLine(line);if(r)this.receive(r);}catch(e){this.onError(e);}}
    push(bytes){this.parser.push(bytes);}
    cancelAll(error){if(this.pending){clearTimeout(this.pending.timer);this.pending.reject(error);this.pending=null;}for(const q of this.queue)q.reject(error);this.queue=[];}
    endLease(){clearTimeout(this.heartbeat);this.heartbeat=null;this.lease=null;}
    renewLoop(){
      if(!this.lease)return;
      clearTimeout(this.heartbeat);this.heartbeat=setTimeout(async()=>{
        const lease=this.lease;
        try{if(lease)await this.request('PING',[lease],{session:lease,priority:true});}
        catch(e){this.endLease();this.onError(e);this.stop().catch(()=>{});return;}
        this.renewLoop();
      },50);
    }
    receive(r){
      const q=this.pending;if(!q||r.id!==q.id)return;
      if(q.op!=='CAP'&&r.generation!==q.generation)return;
      if(r.type==='OK'&&q.session&&q.op!=='STOP'&&r.session!==q.session)return;
      clearTimeout(q.timer);this.pending=null;
      if(r.type==='ERR'){const error=Error(`固件拒绝：${r.error}`);error.confirmed=true;error.wireError=r.error;q.reject(error);}
      else if((q.op==='CAP'&&r.type!=='CAP')||(q.op==='READ'&&r.type!=='DATA')||(q.op==='META'&&r.type!=='META')||(!['CAP','READ','META'].includes(q.op)&&r.type!=='OK'))q.reject(Error('回执类型错误'));
      else{
        if(r.type==='OK'){this.latest=r;if(r.state>=8)this.endLease();this.onStatus(r,q.op);}
        q.resolve(r);
      }
      this.pump();
    }
    request(op,args=[],options={}){
      if(!this.connected||!this.isConnected())return Promise.reject(Error('串口未连接'));
      if(op!=='CAP'&&!this.caps)return Promise.reject(Error('请先读取固件能力'));
      return new Promise((resolve,reject)=>{
        const q={id:++this.nextId,op,args,resolve,reject,generation:this.caps?.generation,...options};
        if(options.priority)this.queue.unshift(q);else this.queue.push(q);this.pump();
      });
    }
    pump(){
      if(this.pending||!this.queue.length)return;const q=this.queue.shift();this.pending=q;
      const values=['@FC1',q.id,q.op,...(q.op==='CAP'?[]:[q.generation]),...q.args];
      q.timer=setTimeout(()=>{
        if(this.pending!==q)return;this.pending=null;
        q.reject(Error(q.op==='START'?'START 结果未确认；不会自动重试，请停止并检查板端状态':`${q.op} 回执超时`));this.pump();
      },this.timeout_ms);
      this.send(values.join(',')+'\r\n').catch(e=>{if(this.pending===q){clearTimeout(q.timer);this.pending=null;q.reject(e);this.pump();}});
    }
    async capabilities(){
      const caps=await this.request('CAP');
      if(caps.format!==1||caps.record_bytes!==48||[caps.lf_id,caps.rf_id,caps.lb_id,caps.rb_id].join(',')!=='1,4,2,3')throw Error('固件记录格式或轮序不兼容');
      if(caps.enabled&&(caps.max_current<1||caps.max_current>1500||caps.max_on_ms<1||caps.max_on_ms>500||caps.max_rpm<1||caps.max_rpm>1000||caps.record_capacity<8||caps.record_capacity>1280||caps.power_stage!==3||caps.bench_mode!==1||caps.heartbeat_ms>150))throw Error('固件限制或构建权限不兼容');
      this.caps=caps;return caps;
    }
    status(){return this.request('STATUS');}
    async arm(){
      if(!this.caps?.enabled)throw Error('固件未启用标定工具');
      const s=await this.request('ARM');if(!s.session)throw Error('会话编号无效');
      this.lease=s.session;this.renewLoop();return s;
    }
    async start(pulse){
      if(!this.lease||this.latest?.state!==2)throw Error('请显式准备并等待静止确认');
      const {wheel,current,rise_ms,hold_ms,fall_ms}=pulse;
      if(![wheel,current,rise_ms,hold_ms,fall_ms].every(Number.isInteger)||wheel<0||wheel>3||!current||Math.abs(current)>this.caps.max_current||Math.min(rise_ms,hold_ms,fall_ms)<1||rise_ms+hold_ms+fall_ms>this.caps.max_on_ms)throw Error('脉冲超出固件限制');
      try{const s=await this.request('START',[this.lease,wheel,current,rise_ms,hold_ms,fall_ms],{session:this.lease});this.pulse={...pulse};this.pulseCaptureId=s.capture_id;return s;}
      catch(e){this.endLease();this.stop().catch(()=>{});throw e;}
    }
    stop(){
      this.endLease();this.cancelAll(Error('请求已由停止取消'));
      if(!this.caps||!this.connected||!this.isConnected())return Promise.reject(Error('串口未连接，板端依靠许可与心跳归零'));
      return this.request('STOP',[0],{priority:true});
    }
    clear(){if(!this.latest)throw Error('请读取状态');return this.request('CLEAR',[this.latest.capture_id]);}
    async readCapture(){
      const s=await this.status();if((s.state>2&&s.state<8)||!s.zero_submitted||!s.capture_id)throw Error('需先冻结记录并成功提交零电流');
      const report={caps:{...this.caps},status:s,pulse:s.capture_id===this.pulseCaptureId?this.pulse:null,records:[],quality:{complete:false,issues:[]},source_commit:null,voltage_v:null};
      try{
        const meta=await this.request('META',[s.capture_id]);
        if(meta.capture_id!==s.capture_id||meta.capture_session!==s.capture_session||meta.capture_generation!==s.capture_generation)throw Error('冻结元数据身份不匹配');
        report.capture_meta=meta;report.pulse=Object.fromEntries(['wheel','current','rise_ms','hold_ms','fall_ms'].map(key=>[key,meta[key]]));
        for(let first=0;first<s.records;){
          let block;for(let attempt=0;attempt<2;attempt++){
            try{block=await this.request('READ',[s.capture_id,first,Math.min(2,s.records-first)]);break;}
            catch(e){if(attempt||!e.message.includes('超时'))throw e;}
          }
          if(block.capture_id!==s.capture_id||block.session!==s.capture_session||block.first!==first||block.total!==s.records||block.count!==Math.min(2,s.records-first))throw Error('记录块身份、数量或索引不匹配');
          for(let k=0;k<block.count;k++)report.records.push(P.decodeRecord(block.bytes.subarray(k*48,(k+1)*48)));
          first+=block.count;
        }
        report.quality.complete=!!s.complete&&report.records.length===s.records;
        if(!s.complete)report.quality.issues.push('板端记录不完整或异常终止');
      }catch(e){report.quality.issues.push(e.message);this.onError(e);}
      if(!report.pulse)report.quality.issues.push('冻结脉冲条件未知；仅保存原始证据');
      return report;
    }
    async disconnect(){
      this.setConnected(false);
    }
  }
  return {Client};
});
