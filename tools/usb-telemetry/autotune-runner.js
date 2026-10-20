(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.AutotuneRunner=api;})(globalThis,function(){
  'use strict';
  async function runTrial(client,session,values,options,{now=()=>performance.now(),sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms))}={}){
    const check=()=>{if(options.signal?.aborted)throw new Error('已停止自动整定。');};
    check();const ack=await client.trial(session,values,options);check();
    if(ack.state==='aborted')return ack;
    if(ack.state!=='running')throw new Error('板端未开始指定试验。');
    const deadline=now()+options.hold+8500;
    while(now()<deadline){await sleep(150);check();const state=await client.status(session);check();
      if(state.state==='done'||state.state==='aborted')return state;
      if(state.state!=='running')throw new Error('板端会话或试验状态已改变。');
    }
    throw new Error('试验结果超时，结束会话并恢复原参数。');
  }
  return {runTrial};
});
