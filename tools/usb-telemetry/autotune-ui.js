(function(){
  'use strict';
  const A=window.TelemetryAutotune,$=id=>document.getElementById(id),names=['左前 LF','右前 RF','左后 LB','右后 RB'],keys=['lf','rf','lb','rb'];
  const reason={none:'有效',limited:'限流，已剔除',no_response:'响应不足',guard:'台架条件改变',feedback:'反馈过期',timeout:'超时',overspeed:'转速越界',can:'CAN 发送失败',stopped:'已停止',quality:'覆盖或采样质量不足',stationary:'未在 3 秒内停稳',config:'配置改变',overshoot:'超调超限',steady:'稳态误差超限',settle:'未按时持续稳定',hard_gates:'硬门槛未通过',repeatability:'重复性不足',speed_regression:'该转速评分退化',paired_wins:'成对改善不足',output_variation:'输出波动增加超过 10%'};
  const stages={baseline:'重复基线',search:'分阶段搜索',control:'原参数对照',rescreen:'前两名复筛',acceptance:'独立验收'};
  const conclusions={accepted:'多转速台架验收通过',no_improvement:'未发现稳定改善',score_floor:'基线已接近评分下限，无充分改善证据',no_candidate:'无通过硬门槛的候选',baseline_invalid:'基线无效，请调整试验条件',baseline_unstable:'基线不稳定',drift:'工况漂移，已停止',invalid_candidates:'连续三个候选无效',budget:'预算不足，仅保留探索报告',aborted:'试验中止',end_unconfirmed:'结束未确认，不可采用'};
  const fmt=n=>Number.isFinite(n)?Number(n.toPrecision(5)).toString():'—';
  const wait=ms=>new Promise(resolve=>setTimeout(resolve,ms));
  window.createAutotuneUI=function({client,tuningClient,lockManual,getSamples}){
    let connected=false,busy=false,controller=null,original=null,results=[],rows=[],demo=false,session=0,selected=0,epoch=0,pollTimer=null,adopted=null,syncPending=null,demoSamples=[];
    let snapshot=null,report=null,continueNext=null;
    const status=(text,error=false)=>{$('auto-status').textContent=text;$('auto-status').classList.toggle('error',error);};
    function controls(){
      $('auto-start').disabled=!connected||busy;$('auto-demo').disabled=connected||busy;$('auto-stop').disabled=!busy||!controller;
      $('auto-adopt').disabled=!connected||busy||demo||!results.some(r=>r.recommended)||!!adopted;
      $('auto-sync').hidden=!syncPending;$('auto-sync').disabled=busy;
      $('auto-save').hidden=!adopted||!['error','volatile'].includes(adopted.storage);$('auto-save').disabled=busy;
      $('auto-json').disabled=!report||busy;$('auto-csv').disabled=!rows.length||busy;
      $('auto-continue').hidden=!continueNext;
      document.querySelectorAll('#auto-settings input,#auto-settings select').forEach(el=>{el.disabled=busy;});
      $('auto-mode').textContent=demo?'演示 · 合成台架':busy?'台架试验进行中':connected?'真实设备':'尚未连接';
    }
    function appendCells(parent,values){const tr=document.createElement('tr');for(const value of values){const td=document.createElement('td');td.textContent=value;tr.append(td);}parent.append(tr);}
    function addRow(row){
      rows.push(row);selected=row.wheel;
      appendCells($('auto-trials'),[rows.length,names[row.wheel],stages[row.stage]+' / '+row.rpm,...row.values.map(fmt),fmt(row.metrics.rmse),fmt(row.metrics.overshoot),fmt(row.metrics.steady),row.metrics.settle===0xffffffff?'未稳定':Number.isFinite(row.metrics.settle)?row.metrics.settle+' ms':'—',row.evaluation.failures.length?row.evaluation.failures.map(f=>reason[f]||f).join('、'):'有效，门槛通过']);
      $('auto-latest').textContent=fmt(row.metrics.rmse)+' rpm';
    }
    function progress(p){$('auto-progress').value=p.budget.used;
      $('auto-progress-text').textContent=names[p.wheel]+' · '+stages[p.stage]+' · '+p.rpm+' rpm · 有效 '+(p.valid??0)+' / 无效 '+(p.invalid??0)+' · 剩余 '+p.budget.remaining+' 次 / '+Math.floor(p.budget.remainingMs/1000)+' 秒；按历史耗时估计剩余预算约 '+Math.ceil(p.budget.estimatedMs/1000)+' 秒';}
    function compare(){
      $('auto-results').replaceChildren();$('auto-comparisons').replaceChildren();
      const range=s=>fmt(s.median)+' ['+fmt(s.min)+'～'+fmt(s.max)+']';
      for(const r of results){appendCells($('auto-results'),[names[r.wheel],r.original.map(fmt).join(' / '),(r.candidate||r.values).map(fmt).join(' / '),Number.isFinite(r.improvement)?fmt(r.improvement*100)+'%':'—',r.boundary?'搜索触及边界':'未触及',r.invalidated||conclusions[r.reason]||r.reason]);
        for(const c of r.comparisons||[])appendCells($('auto-comparisons'),[names[r.wheel]+' / '+c.rpm,range(c.original),range(c.candidate),fmt(c.originalTV.median)+' / '+fmt(c.candidateTV.median),Object.entries(c.gates).map(([k,g])=>({overshoot:'超调 rpm',steady:'稳态 rpm',settle:'稳定 ms',limited:'限流 ‰'}[k])+': '+range(g.candidate)+' ≤ '+fmt(g.threshold)).join('；'),c.pairedWins+'/3',c.failures.length?c.failures.map(f=>reason[f]||f).join('、'):'全部通过']);}
    }
    function settings(){
      const rpm=Number($('auto-rpm').value),hold=Number($('auto-hold').value),cap=Number($('auto-cap').value);
      if(![rpm,hold,cap].every(Number.isInteger)||rpm<100||rpm>1500||hold<500||hold>2000||cap<500||cap>5000)throw new Error('请检查转速、保持时间和电流上限范围。');
      return {rpm,hold,cap};
    }
    async function hardwareTrial(values,options){
      const metrics=await window.AutotuneRunner.runTrial(client,session,values,options);
      const sample=getSamples().at(-1);return {...metrics,environment:sample&&performance.now()-sample.arrivalMs<500?sample.normalized.raw:{observed:false},supplyVoltage:'未观测'};
    }
    async function demoTrial(values,options){
      await wait(35);A.check(options.signal);
      const error=12+Math.abs(values[0]-1.5)*100+Math.abs(values[1]-1.5)*18,now=performance.now();demoSamples=[];
      for(let i=0;i<100;++i){const t=i/99*1.6,rpm=t<.3?0:t<1.3?options.rpm*(1-Math.exp(-(t-.3)*12))*(1-error/700):options.rpm*Math.exp(-(t-1.3)*18);
        demoSamples.push({arrivalMs:now-1600+i/99*1600,segment:1,normalized:{values:{['rpm_'+keys[options.wheel]]:rpm,['target_'+keys[options.wheel]+'_rpm']:t>=.3&&t<1.3?options.rpm:0}}});}
      return {version:2,state:'done',reason:'none',samples:options.hold,feedbackCount:options.hold/2,window:options.hold,coverage:options.hold,maxFeedback:2,maxStat:1,statLimit:4,currentLimit:options.cap,rmse:error,overshoot:0,steady:3,settle:200,jitter:100,limited:0,synthetic:true};
    }
    async function readSnapshot(){
      const clean=r=>({revision:r.revision,mode:r.mode,values:r.values});
      const current={pid:clean(await tuningClient.get('pid'))};
      for(const group of ['power','spin'])try{current[group]=clean(await tuningClient.get(group));}catch(e){if(!/未启用|不支持/.test(e.message))throw e;current[group]={observed:false,reason:e.message};}
      current.controllers=[];for(let wheel=0;wheel<4;++wheel)current.controllers.push(await client.configuration(wheel));
      return current;
    }
    function invalidate(message){for(const r of results){r.recommended=false;r.invalidated=message;}if(report)report.invalidated=message;compare();}
    function ensureSnapshot(current){if(JSON.stringify(current)!==JSON.stringify(snapshot)){invalidate('参数或控制配置改变，结果失效');throw new Error('原参数或控制配置已发生变化，禁止覆盖；请重新整定。');}}
    async function nextWheel(wheel,signal){
      status('本轮已结束并恢复原参数。请先进入零力模式，再重新准备遥控器，点击继续测试 '+names[wheel]+'。');
      await new Promise(resolve=>{continueNext=resolve;signal.addEventListener('abort',resolve,{once:true});controls();});
      continueNext=null;controls();A.check(signal);
    }
    async function start(isDemo){
      if(busy)return;let options;
      try{options=settings();if(!isDemo&&!$('auto-confirm').checked)throw new Error('请先确认底盘已架空固定、四轮离地。');}catch(e){status(e.message,true);return;}
      const token=epoch,aborter=new AbortController();busy=true;demo=isDemo;controller=aborter;results=[];rows=[];adopted=null;syncPending=null;session=0;demoSamples=[];snapshot=null;clearTimeout(pollTimer);
      report={version:2,algorithm:A.rules.version,rules:A.rules,started:new Date().toISOString(),synthetic:isDemo,settings:options,supplyVoltage:'未观测',scope:'正向、架空台架；反向及带载尚未验证',results,records:rows};
      $('auto-trials').replaceChildren();$('auto-results').replaceChildren();$('auto-comparisons').replaceChildren();$('auto-persistence').textContent='候选参数仅用于试验，尚未采用。';$('auto-progress').max=50;$('auto-progress').value=0;
      lockManual(true);controls();status(isDemo?'演示整定中，不连接设备、不保存参数。':'正在读取原参数并建立台架会话…');
      let completed=false;
      try{
        while(tuningClient.pending){A.check(aborter.signal);await wait(30);}
        if(isDemo)original={revision:0,values:Array.from({length:4},()=>[.75,0,0,15000,3000,0]).flat()};
        else{status('正在检查 V2 固件能力与控制配置…');report.capabilities=await client.capabilities();snapshot=await readSnapshot();original=snapshot.pid;}
        report.configuration=snapshot;report.original=original;
        const wheels=$('auto-wheel').value==='all'?[0,1,2,3]:[Number($('auto-wheel').value)];
        for(const wheel of wheels){
          A.check(aborter.signal);selected=wheel;
          if(wheel!==wheels[0]&&!isDemo)await nextWheel(wheel,aborter.signal);
          if(!isDemo){ensureSnapshot(await readSnapshot());A.check(aborter.signal);const begun=await client.begin(original.revision);session=begun.session;}
          status((isDemo?'演示 · 合成数据 · ':'')+'正在整定 '+names[wheel]+'，Kd 保持原值。');
          const result=await A.searchWheelV2(isDemo?demoTrial:hardwareTrial,original.values,{...options,wheel,elapsed:0,signal:aborter.signal,onTrial:addRow,onProgress:progress});
          if(session){try{await client.end(session);session=0;result.endConfirmed=true;}catch(e){result.recommended=false;result.reason='end_unconfirmed';results.push(result);compare();throw e;}}
          results.push(result);compare();
          if(['aborted','drift','invalid_candidates','baseline_invalid','baseline_unstable','budget'].includes(result.reason)){status(conclusions[result.reason]+'。已结束本轮，保留探索报告。',true);break;}
        }
        completed=true;
      }catch(e){report.error=e.message;if(token===epoch)status(e.message+' 请进入零力模式后重新准备。',true);}
      finally{
        if(session&&connected)try{while(client.pending)await wait(25);await client.end(session);}catch(e){report.endError=e.message;if(token===epoch)status('结束未确认：'+e.message+' 请进入零力模式。',true);}
        session=0;controller=null;continueNext=null;busy=false;lockManual(false);report.finished=new Date().toISOString();
        if(token!==epoch){controls();return;}
        if(completed){const improved=results.filter(r=>r.recommended).length,last=results.at(-1);
          status((isDemo?'演示完成（合成数据），仅展示判定，不可采用。':'试调完成，原参数已恢复；请进入零力模式。')+(improved?' '+improved+' 个轮子通过多转速台架验收。':' '+(conclusions[last?.reason]||'未发现稳定改善')+'；保留原参数。'));
          $('auto-progress').max=Math.max(1,$('auto-progress').value);}
        controls();
      }
    }
    function savedText(){const state=adopted?.storage;
      $('auto-persistence').textContent=state==='saved'?'已保存到板端，重启后恢复。':state==='pending'?'等待板端保存：请进入零力模式，保持电机停止。':state==='error'?'板端保存失败，可停机后重试。':state==='unsupported'?'旧固件未提供保存状态，请升级。':'尚未确认板端保存。';controls();}
    function schedulePoll(){clearTimeout(pollTimer);if(connected&&adopted?.storage==='pending')pollTimer=setTimeout(pollSave,1000);}
    async function pollSave(){
      if(busy||tuningClient.pending){schedulePoll();return;}const token=epoch,record=adopted;
      try{const state=await tuningClient.get('pid');if(token!==epoch)return;
        if(adopted!==record)return;
        if(state.revision!==record.revision){adopted=null;syncPending=null;status('板端参数已改变，请重新整定或读取。',true);controls();return;}
        adopted=state;savedText();}catch(e){if(token===epoch)$('auto-persistence').textContent='保存状态未确认：'+e.message;}
      schedulePoll();
    }
    async function synchronize(token=epoch){
      const record=syncPending;if(!record)return;
      const response=await fetch('/api/tuning',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({group:'pid',...record})});
      const data=await response.json();if(!response.ok||!data.saved)throw new Error(data.error||'本地文件同步失败。');
      if(token!==epoch)return;
      if(syncPending===record)syncPending=null;status('候选参数已采用，项目参数文件已同步。');
    }
    async function adopt(){
      if(busy||demo||!original)return;busy=true;lockManual(true);controls();const token=epoch;
      try{while(tuningClient.pending)await wait(30);const currentSnapshot=await readSnapshot(),current=currentSnapshot.pid;
        if(token!==epoch)throw new Error('连接已改变，请重新整定。');
        ensureSnapshot(currentSnapshot);
        adopted=await tuningClient.set('pid',current.revision,A.mergeResults(original.values,results));if(token!==epoch)return;
        syncPending=adopted;savedText();await synchronize(token);
      }catch(e){if(token===epoch)status((adopted?'板端已生效，文件尚未同步：':'')+e.message,true);}
      finally{busy=false;lockManual(false);controls();if(token===epoch)schedulePoll();}
    }
    $('auto-start').addEventListener('click',()=>start(false));$('auto-demo').addEventListener('click',()=>start(true));
    $('auto-continue').addEventListener('click',()=>continueNext?.());
    function download(content,type,extension){const url=URL.createObjectURL(new Blob([content],{type})),link=document.createElement('a');link.href=url;link.download='pid-autotune-v2-'+new Date().toISOString().replaceAll(':','-')+'.'+extension;link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
    $('auto-json').addEventListener('click',()=>download(JSON.stringify(report,null,2),'application/json','json'));
    $('auto-csv').addEventListener('click',()=>download(A.reportCSV(rows),'text/csv;charset=utf-8','csv'));
    document.querySelectorAll('#auto-settings input,#auto-settings select').forEach(el=>el.addEventListener('change',()=>{if(!busy&&results.length){invalidate('测试配置改变，结果失效');controls();}}));
    $('auto-stop').addEventListener('click',()=>{controller?.abort();status('正在结束试调并恢复原参数…');});$('auto-adopt').addEventListener('click',adopt);
    $('auto-sync').addEventListener('click',async()=>{busy=true;lockManual(true);controls();const token=epoch,record=syncPending;
      try{const current=await tuningClient.get('pid');if(token!==epoch)return;
        if(current.revision!==record.revision||current.values.some((v,i)=>v!==record.values[i]))throw new Error('参数已改变，请重新读取。');await synchronize(token);
      }catch(e){if(token===epoch)status(e.message,true);}finally{busy=false;lockManual(false);controls();}});
    $('auto-save').addEventListener('click',async()=>{busy=true;lockManual(true);controls();const token=epoch;
      try{const state=await tuningClient.save('pid',adopted.revision);if(token!==epoch)return;adopted=state;savedText();}catch(e){if(token===epoch)status(e.message,true);}finally{busy=false;lockManual(false);controls();if(token===epoch)schedulePoll();}});
    controls();
    return {
      setConnected(value){if(connected===value)return;connected=value;++epoch;clearTimeout(pollTimer);
        if(!value){controller?.abort();client.disconnect();invalidate('连接改变，未采用结果失效');results=[];adopted=null;syncPending=null;status('串口已断开；板端将停止试调。请进入零力模式，重连后重新读取。',true);}
        else{demo=false;results=[];original=null;status('先架空固定底盘，再按说明准备遥控器。');}controls();},
      renderChart(){if($('autotune-panel').hidden)return;const samples=demo?demoSamples:getSamples(),key=keys[selected],now=demoSamples.length&&demo?demoSamples.at(-1).arrivalMs:performance.now();
        window.TelemetryCharts.drawChart($('auto-chart'),samples,[{key:'target_'+key+'_rpm',name:'目标',color:'#8a65d6',dash:true},{key:'rpm_'+key,name:'实际',color:'#3478f6'}],{now,unit:'rpm',range:[-5000,0]});},
      configurationChanged(){if(!busy&&results.length){invalidate('控制配置改变，结果失效');controls();}},
      stop(){controller?.abort();},get busy(){return busy;}
    };
  };
})();
