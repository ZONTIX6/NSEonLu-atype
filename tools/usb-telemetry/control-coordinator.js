(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.TelemetryControlCoordinator=api;})(globalThis,function(){
  'use strict';
  const cancelled=()=>Error('未发送请求已由停止或连接变化取消');
  class Coordinator{
    constructor({send,onChange=()=>{}}){this.writer=send;this.onChange=onChange;this.owner=null;this.phase='idle';this.identity=null;this.connected=false;this.queue=[];this.writing=false;this.epoch=0;}
    notify(){this.onChange({owner:this.owner,phase:this.phase,identity:this.identity});}
    setConnected(value){this.connected=value;++this.epoch;this.cancelQueued();this.owner=null;this.phase='idle';this.identity=null;this.notify();}
    cancelQueued(){for(const q of this.queue)q.reject(cancelled());this.queue=[];}
    acquire(owner,identity=null){if(!this.connected||this.owner)throw Error(this.owner?'其他控制会话或停止结果尚未确认':'串口未连接');this.cancelQueued();this.owner=owner;this.phase='requesting';this.identity=identity;this.notify();}
    activate(owner,identity){if(this.owner===owner){this.phase='active';this.identity=identity;this.notify();}}
    unknown(owner){if(this.owner===owner){this.phase='unknown';this.notify();}}
    stopping(owner){if(this.owner===owner){this.phase='stopping';this.notify();}}
    release(owner,confirmed=false){if(this.owner===owner&&confirmed){this.owner=null;this.phase='idle';this.identity=null;this.notify();}}
    observeForce(s){
      if(this.owner!=='force'||!this.identity?.session||this.phase==='requesting'||(this.identity?.generation!==undefined&&s.generation!==this.identity.generation)||
        (this.identity?.session&&s.session!==this.identity.session))return;
      if(s.state>=8&&s.zero_submitted===1&&Array.isArray(s.final)&&s.final.length===4&&s.final.every(x=>x===0))this.release('force',true);
    }
    canWrite(owner){return !this.owner||this.owner===owner&&this.phase==='active';}
    send(owner,line){
      const op=line.trim().split(',')[2],stop=['STOP','END','CANCEL'].includes(op),heartbeat=['PING','STATUS'].includes(op)&&owner==='pid'||op==='PING';
      if(!this.connected)return Promise.reject(Error('串口未连接'));
      const requiredRead=owner==='tuning'&&this.owner==='pid'&&op==='GET'||owner==='pid'&&this.owner==='tuning'&&['CONFIG','CAP'].includes(op);
      if(!stop&&this.owner&&this.owner!==owner&&!requiredRead)return Promise.reject(Error('当前控制会话已锁定其他功能'));
      if(this.owner&&['unknown','stopping'].includes(this.phase)&&!stop&&op!=='STATUS'&&op!=='PING')return Promise.reject(Error('停止或操作结果尚未确认'));
      if(stop)this.cancelQueued();
      return new Promise((resolve,reject)=>{this.queue.push({owner,line,resolve,reject,priority:stop?0:heartbeat?1:['CAP','GET','STATUS','READ','META','CONFIG'].includes(op)?3:2,epoch:this.epoch});this.queue.sort((a,b)=>a.priority-b.priority);this.pump();});
    }
    async pump(){
      if(this.writing||!this.queue.length)return;this.writing=true;const q=this.queue.shift();
      try{if(q.epoch!==this.epoch||!this.connected)throw cancelled();await this.writer(q.line);q.resolve();}catch(e){q.reject(e);}finally{this.writing=false;this.pump();}
    }
  }
  return {Coordinator};
});
