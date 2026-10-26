(function(){
  'use strict';
  window.createForceCalibrationUI=function({client,coordinator,isConnected,getOtherBusy=()=>false}){
  const P=ForceCalibrationProtocol,R=ForceCalibrationReport,$=id=>document.getElementById('fc-'+id);
  const wheelNames=['LF 左前 · CAN ID 1','RF 右前 · CAN ID 4','LB 左后 · CAN ID 2','RB 右后 · CAN ID 3'];
  let caps=null,latest=null,report=null,reports=[],measurements=[],polling=false,reading=false,visible=false,stopPromise=null,capPromise=null,epoch=0;
  function showError(error){const message=error?.message||String(error||'串口操作已取消，请重新连接');$('message').textContent=message.includes('CAP 回执超时')?'固件未提供 FC1 能力，请使用本工具对应的标定固件。':message;$('message').classList.add('error');}
  function number(id){return $(id).value.trim()===''?null:Number($(id).value);}
  function pulse(){return {wheel:number('wheel'),current:number('current'),rise_ms:number('rise'),hold_ms:number('hold'),fall_ms:number('fall')};}
  function active(){return latest&&latest.state>=1&&latest.state<=7;}
  function validPulse(){const p=pulse();return caps&&Object.values(p).every(Number.isInteger)&&p.wheel>=0&&p.wheel<4&&p.current!==0&&Math.abs(p.current)<=caps.max_current&&Math.min(p.rise_ms,p.hold_ms,p.fall_ms)>0&&p.rise_ms+p.hold_ms+p.fall_ms<=caps.max_on_ms;}
  function buttons(){
    const connected=isConnected(),frozen=connected&&latest&&(latest.state<=2||latest.state>=8)&&latest.zero_submitted;
    $('arm').disabled=!connected||!caps?.enabled||active()||reading||!!coordinator.owner||getOtherBusy();
    $('start').disabled=!connected||latest?.state!==2||coordinator.owner!=='force'||coordinator.phase!=='active'||!$('fixture').checked||!validPulse()||reading;
    $('read').disabled=!frozen||reading||!latest.capture_id;
    $('clear').disabled=!frozen||active()||reading||!latest.capture_id;
    $('pulse-fields').disabled=!!active();$('fixture').disabled=!!latest&&latest.state>=3&&latest.state<=7;
    $('json').disabled=!report;$('csv').disabled=!report;$('analyze').disabled=!report?.quality?.complete||!report.pulse||report.synthetic===true;
    $('import').disabled=!!active();$('example').disabled=!!active();
  }
  function preview(){const p=pulse();$('preview').textContent=`${wheelNames[p.wheel]} · ${p.current??'—'} CAN 单位 ≈ ${p.current===null?'—':(p.current*20/16384).toFixed(3)} A 指令等效 · 通电 ${p.rise_ms+p.hold_ms+p.fall_ms} ms（含上升与下降）`;buttons();}
  function showStatus(s,operation){
    latest=s;$('state').textContent=P.states[s.state];$('reason').textContent=P.reasons[s.reason];
    $('output').textContent=`${s.requested} / ${s.final.join(', ')}（LF/RF/LB/RB）`;
    $('rpm').textContent=`${s.raw_rpm} / ${s.used_rpm??'未知'}`;$('temperature').textContent=`${s.temperature} ℃ / ${s.feedback_age_us} μs`;
    $('power').textContent=`${s.load_w??'未知'} / ${s.budget_w??'未知'}`;
    $('zero').textContent=s.zero_submitted?'已成功提交零电流至 CAN 发送邮箱；此证明不代表电调已执行。':'等待最终零电流成功提交，暂不可导出。';
    if(!stopPromise&&operation==='STATUS')coordinator.observeForce(s);
    if(s.state>=8){$('fixture').checked=false;$('message').textContent=stopPromise?'等待后续状态确认零电流提交。':s.reason?`已停止：${P.reasons[s.reason]}。保留原始记录供检查。`:'本次脉冲已结束，记录冻结后可手动读取。';}buttons();
  }
  async function poll(){
    if(polling||!isConnected()||!caps||(!visible&&coordinator.owner!=='force')||stopPromise)return;
    polling=true;try{await client.status();}catch(e){if(isConnected())showError(e);}finally{polling=false;}
  }
  function action(id,fn){$(id).addEventListener('click',async()=>{try{await fn();}catch(e){showError(e);}finally{buttons();}});}
  async function capabilities(){
    $('message').classList.remove('error');
    caps=await client.capabilities();$('status').textContent=caps.enabled?'已连接 · 标定固件':'已连接 · 标定功能 disabled';
    $('limits').textContent=caps.enabled?`真实边界：|电流| ≤ ${caps.max_current} CAN 单位，通电 ≤ ${caps.max_on_ms} ms，转速 < ${caps.max_rpm} rpm，温度 < ${caps.stop_temperature_c} ℃；静止 ${caps.stationary_rpm} rpm / ${caps.stationary_ms} ms；心跳 ${caps.heartbeat_ms} ms；缓存 ${caps.record_capacity} × ${caps.record_bytes} B。记录时长上界由实际 RX 与控制事件频率决定。`:'当前固件未启用标定；请构建独立台架版本。';
    $('message').textContent=caps.enabled?'连接成功。检查准备条件后点击“手动准备”。':'固件能力 disabled，无法准备或加载电机。';
    $('current').min=-caps.max_current;$('current').max=caps.max_current;await poll();
  }
  function ensureCapabilities(){if(!isConnected())return Promise.resolve();if(caps)return poll();if(!capPromise)capPromise=capabilities().catch(showError).finally(()=>{capPromise=null;buttons();});return capPromise;}
  action('arm',async()=>{
    if(getOtherBusy())throw Error('请等待其他控制操作结束');const token=epoch;coordinator.acquire('force',{generation:caps.generation});
    try{const s=await client.arm();coordinator.activate('force',{generation:s.generation,session:s.session});$('message').textContent='正在确认连续静止；板端检查遥控器、辅助电机、反馈与能源许可。';}
    catch(e){if(token===epoch){if(e.confirmed)coordinator.release('force',true);else coordinator.unknown('force');}throw e;}
  });
  action('start',async()=>{if(coordinator.owner!=='force'||coordinator.phase!=='active')throw Error('标定会话尚未确认');const p=pulse();try{await client.start(p);$('message').textContent=`正在执行 ${wheelNames[p.wheel]} 的一次脉冲；需要时立即停止。`;}catch(e){if(!e.confirmed)coordinator.unknown('force');throw e;}});
  function stopAndConfirm(){
    if(stopPromise)return stopPromise;
    const token=epoch;
    coordinator.stopping('force');
    stopPromise=(async()=>{
      const stopped=await client.stop(),deadline=Date.now()+1800;
      do{
        const s=await client.status();
        if(token!==epoch)throw Error('连接已改变，停止结果不属于当前连接');
        if(s.generation===stopped.generation&&s.session===stopped.session&&(s.state>=8||s.state===0)&&s.zero_submitted===1&&s.final.length===4&&s.final.every(n=>n===0)){
          coordinator.release('force',true);$('message').textContent='后续板端状态已确认成功提交零电流。';return s;
        }
        await new Promise(resolve=>setTimeout(resolve,50));
      }while(Date.now()<deadline);
      throw Error('停止尚未确认，请保持停机条件并使用公共停止入口重试');
    })().catch(e=>{if(token===epoch){coordinator.unknown('force');showError(Error('停止尚未确认：'+e.message));}throw e;}).finally(()=>{if(token===epoch){stopPromise=null;buttons();}});return stopPromise;
  }
  action('read',async()=>{reading=true;buttons();try{setReport(await client.readCapture());}finally{reading=false;}});
  action('clear',async()=>{await client.clear();$('message').textContent='板端记录已清除；电脑端已读取的报告仍可保存。';});
  for(const id of ['wheel','current','rise','hold','fall','fixture'])$(id).addEventListener('input',preview);
  function setReport(next){
    if(!Array.isArray(next.records)||next.records.length>1280||next.records.some(r=>!['RX','CONTROL','MARKER'].includes(r.type)||!Number.isFinite(r.time_us)||!Number.isInteger(r.wheel)||r.wheel<0||r.wheel>3))throw Error('记录文件格式或容量错误');
    report=next;report.quality=R.validateReport(next);
    for(const [id,value] of Object.entries({window:next.window_us==null?'':next.window_us/1000,margin:next.margin_rpm??'',gap:next.max_gap_us??5000,voltage:next.voltage_v??'',commit:next.source_commit??'',conditions:next.conditions?.notes??''}))$(id).value=value;
    measurements=Array.isArray(next.measurements)?next.measurements.map(x=>({...x})):[];measurementTable();
    if(report.analysis){const computed=R.analyzeGrowth(report);report.analysis=report.quality.complete&&computed.valid?computed:null;}
    $('analysis').textContent='请确认本报告的 T、额外裕量和观察条件后重新计算。';
    $('quality').textContent=`${next.synthetic?'synthetic 离线示例 · ':''}${next.records.length} 条原始事件 · ${report.quality.complete?'记录完整':'记录不完整，仅可保存证据'}。${report.quality.issues.join('；')}`;
    $('point').max=Math.max(0,next.records.length-1);$('point').value=0;$('point').disabled=!next.records.length;
    fit();point();buttons();
  }
  function fit(){if(report?.records.length){$('view-from').value=0;$('view-to').value=Math.max(1,Math.ceil(report.records.at(-1).time_us/1000));}draw();}
  function point(){const r=report?.records[number('point')];$('point-detail').textContent=r?JSON.stringify(r,null,2):'尚无原始事件。';}
  $('point').addEventListener('input',point);$('fit').addEventListener('click',fit);
  for(const id of ['view-from','view-to'])$(id).addEventListener('input',draw);
  function chart(id,series,unit){
    const canvas=$(id),rect=canvas.getBoundingClientRect(),dpr=devicePixelRatio||1;
    canvas.width=Math.max(1,rect.width*dpr);canvas.height=rect.height*dpr;const c=canvas.getContext('2d');c.scale(dpr,dpr);
    const width=rect.width,height=rect.height,left=58,right=15,top=18,bottom=34;
    const from=(number('view-from')??0)*1000,to=(number('view-to')??350)*1000;
    if(to<=from)return;
    const visible=(report?.records||[]).filter(r=>r.time_us>=from&&r.time_us<=to);
    const values=series.flatMap(s=>visible.filter(s.when).map(s.value)).filter(Number.isFinite);
    const min=Math.min(0,...values),max=Math.max(1,...values),span=max-min;
    const x=t=>left+(t-from)/(to-from)*(width-left-right),y=v=>height-bottom-(v-min)/span*(height-top-bottom);
    c.font='11px Segoe UI';c.fillStyle='#536780';c.strokeStyle='#e7ecf3';c.lineWidth=1;
    for(let k=0;k<=4;k++){const v=min+span*k/4;c.beginPath();c.moveTo(left,y(v));c.lineTo(width-right,y(v));c.stroke();c.fillText(v.toFixed(0),5,y(v)+4);}
    c.fillText(`${(from/1000).toFixed(1)} ms`,left,height-8);c.fillText(`${(to/1000).toFixed(1)} ms`,Math.max(left,width-82),height-8);
    for(const s of series){c.strokeStyle=s.color;c.lineWidth=1.7;c.setLineDash(s.dashed?[5,4]:[]);c.beginPath();let previous=null;
      for(const r of visible.filter(s.when)){const v=s.value(r);if(!Number.isFinite(v)){previous=null;continue;}
        if(!previous||r.time_us-previous.time_us>5000)c.moveTo(x(r.time_us),y(v));else c.lineTo(x(r.time_us),y(v));previous=r;}
      c.stroke();c.setLineDash([]);
    }
    if(!visible.length)c.fillText('读取冻结记录后显示原始事件',left+10,height/2);
    for(const r of visible.filter(r=>r.type==='CONTROL'&&r.attempted!==r.successful)){c.strokeStyle='#ac3024';const px=x(r.time_us);c.beginPath();c.moveTo(px-4,top);c.lineTo(px+4,top+8);c.moveTo(px+4,top);c.lineTo(px-4,top+8);c.stroke();}
    canvas.setAttribute('aria-label',`${unit}；${visible.length} 条板端事件；时间 ${(from/1000).toFixed(1)} 至 ${(to/1000).toFixed(1)} ms`);
  }
  function draw(){if(!visible)return;chart('speed-chart',[{when:r=>r.type==='RX',value:r=>r.raw_rpm,color:'#2763bd'},{when:r=>r.type==='CONTROL',value:r=>r.used_rpm,color:'#007d72',dashed:true}],'原始/控制使用转速 rpm');
    chart('current-chart',[{when:r=>r.type==='CONTROL',value:r=>r.requested,color:'#2763bd'},{when:r=>r.type==='CONTROL',value:r=>r.final?.[r.wheel],color:'#a04f0a',dashed:true}],'请求/最终电流 CAN 单位');}
  function enrich(){return {...report,conditions:{...(report.conditions||{}),notes:$('conditions').value},voltage_v:number('voltage'),source_commit:$('commit').value.trim()||null,
    window_us:number('window')===null?null:number('window')*1000,margin_rpm:number('margin'),max_gap_us:number('gap'),measurements:R.torqueRows({measurements})};}
  action('analyze',()=>{
    if(!report?.quality.complete||!report.pulse||report.synthetic)throw Error('当前记录不能给出标定候选');
    const analysis=R.analyzeGrowth({...enrich(),window_us:number('window')===null?null:number('window')*1000,margin_rpm:number('margin'),max_gap_us:number('gap')});
    report.analysis=analysis;Object.assign(report,{window_us:number('window')*1000,margin_rpm:number('margin'),max_gap_us:number('gap')});$('analysis').textContent=analysis.valid?`待复核候选 ${analysis.candidate_rpm.toFixed(1)} rpm（最大增长 ${analysis.max_growth_rpm.toFixed(1)} rpm，${analysis.windows.length} 个窗口，排除 ${analysis.excluded.length} 个）。仅覆盖 ${wheelNames[report.pulse.wheel]} / ${report.pulse.current} CAN 单位，电压 ${number('voltage')??'未观测'} V。`:analysis.issues.join('；');
  });
  action('add-measurement',()=>{
    const mass=number('mass'),force=number('force'),arm=number('arm-length'),lower=number('lower'),upper=number('upper');
    if((mass===null&&force===null)||arm===null||arm<=0||mass<0||force<0||lower===null||upper===null)throw Error('请填写总质量或拉力、正力臂，以及上下阈值');
    measurements.push({mass_kg:mass,force_n:force,arm_m:arm,current:(lower+upper)/2,lower_current:lower,upper_current:upper,direction:$('direction').value});
    measurementTable();
  });
  function measurementTable(){$('measurements').replaceChildren(...R.torqueRows({measurements}).map(row=>{const tr=document.createElement('tr');for(const value of [row.direction,row.mass_kg,row.force_n,row.arm_m,row.torque_nm,row.command_a,row.issues.join('；')||'待复核']){const td=document.createElement('td');td.textContent=typeof value==='number'?value.toFixed(4):value??'未知';tr.append(td);}return tr;}));}
  for(const id of ['window','margin','gap','voltage','conditions','commit'])$(id).addEventListener('input',()=>{if(report){delete report.analysis;$('analysis').textContent='条件已改变，请重新计算候选。';}});
  function download(kind){if(!report)return;const full=enrich(),blob=new Blob([kind==='json'?R.toJson(full):R.toCsv(full)],{type:kind==='json'?'application/json':'text/csv;charset=utf-8'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=`force-calibration-${report.status?.capture_id??'offline'}.${kind}`;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
  action('json',()=>download('json'));action('csv',()=>download('csv'));
  $('import').addEventListener('change',async()=>{try{
    if(active())throw Error('先停止测试，再导入历史文件');reports=[];
    for(const file of $('import').files){if(file.size>10000000)throw Error('历史文件超过 10 MB');const data=JSON.parse(await file.text());setReport(data);reports.push(report);}
    const envelope=R.envelope(reports);$('envelope').textContent=envelope.four_wheels_complete?`四轮均有完整记录；共同包络候选 ${envelope.candidate_rpm} rpm，仍需复核一致的测量范围。`:`已导入 ${envelope.wheels.length}/4 轮有效候选；四轮尚未完成。`;
  }catch(e){showError(e);}finally{$('import').value='';}});
  action('example',()=>setReport({synthetic:true,pulse:null,records:[{type:'RX',wheel:1,time_us:0,raw_rpm:0,feedback_seq:1},{type:'RX',wheel:1,time_us:2000,raw_rpm:40,feedback_seq:2}],quality:{complete:false,issues:['人工示例，不是硬件采样；不能用于采用参数']}}));
  window.addEventListener('resize',draw);
  setInterval(poll,100);preview();draw();if(!navigator.serial)showError(Error('此浏览器未提供 Web Serial，请用 Edge / Chrome 打开 localhost 页面。'));
  return {showStatus,showError,ensureCapabilities,stopAndConfirm,refresh:buttons,
    setVisible(value){visible=value;if(value){draw();ensureCapabilities();}},
    setConnected(value){++epoch;caps=null;latest=null;capPromise=null;stopPromise=null;$('status').textContent=value?'已连接 · 正在读取标定能力':'未连接';$('state').textContent='未准备';buttons();if(value)ensureCapabilities();},
    get report(){return report;},get active(){return active()||coordinator.owner==='force';}};
  };
})();
