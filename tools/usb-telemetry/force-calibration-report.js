(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.ForceCalibrationReport=api;})(globalThis,function(){
  'use strict';
  function quality(records){
    const issues=[];let lastTime=-1,lastRx=null,lastControl=null;const rx_intervals_us=[],control_intervals_us=[];
    for(const r of records){
      if(!Number.isFinite(r.time_us)||r.time_us<lastTime)issues.push('事件时间无序');lastTime=r.time_us;
      if(r.flags||r.quality_error)issues.push('时间溢出或记录质量失败');
      if(r.type==='RX'){
        if(lastRx){const delta=((r.feedback_seq??r.sequence)-(lastRx.feedback_seq??lastRx.sequence))>>>0;
          if(delta!==1)issues.push(delta===0?'重复反馈序号':`反馈序号缺口 ${delta-1}`);
          rx_intervals_us.push(r.time_us-lastRx.time_us);}
        lastRx=r;
      }else if(r.type==='CONTROL'){
        if(lastControl)control_intervals_us.push(r.time_us-lastControl.time_us);lastControl=r;
        if(r.attempted!==undefined&&(r.attempted===0||r.successful!==r.attempted))issues.push('CAN 命令未成功提交');
        if(r.reason)issues.push('记录含停止原因');if(r.cycle_us>5000)issues.push('控制周期超过 5 ms');
      }
    }
    return {issues:[...new Set(issues)],rx_intervals_us,control_intervals_us,valid:!issues.length};
  }
  function analyzeGrowth({records,window_us,margin_rpm,max_gap_us}){
    const result={valid:false,max_growth_rpm:null,candidate_rpm:null,windows:[],excluded:[],issues:[]};
    if(!Array.isArray(records)||!Number.isFinite(window_us)||window_us<=0||!Number.isFinite(margin_rpm)||margin_rpm<0||!Number.isFinite(max_gap_us)||max_gap_us<=0){result.issues.push('请填写窗口、裕量与最大间隔');return result;}
    const q=quality(records);if(!q.valid){result.issues=q.issues;return result;}
    const rx=records.filter(r=>r.type==='RX'),controls=records.filter(r=>r.type==='CONTROL');
    for(const c of controls){
      const end=c.time_us+window_us;
      if(!Number.isFinite(c.used_rpm)||!rx.length||rx.at(-1).time_us<end){result.excluded.push({time_us:c.time_us,reason:'窗口尾部不足或控制转速缺失'});continue;}
      const before=rx.filter(r=>r.time_us<=c.time_us).at(-1),points=rx.filter(r=>r.time_us>c.time_us&&r.time_us<=end);
      const chain=[before,...points].filter(Boolean);
      let valid=!!before&&c.time_us-before.time_us<=max_gap_us&&end-chain.at(-1)?.time_us<=max_gap_us;
      for(let k=0;k<chain.length;k++)if(!Number.isFinite(chain[k].raw_rpm)||(k&&chain[k].time_us-chain[k-1].time_us>max_gap_us))valid=false;
      if(!valid){result.excluded.push({time_us:c.time_us,reason:'窗口存在反馈缺口'});continue;}
      const inside=rx.filter(r=>r.time_us>=c.time_us&&r.time_us<=end);
      if(!inside.length){result.excluded.push({time_us:c.time_us,reason:'窗口内没有样本'});continue;}
      const peak=Math.max(...inside.map(r=>Math.abs(r.raw_rpm))),growth=Math.max(0,peak-Math.abs(c.used_rpm));
      result.windows.push({time_us:c.time_us,end_us:end,growth_rpm:growth});
    }
    if(result.windows.length){result.valid=true;result.max_growth_rpm=Math.max(...result.windows.map(x=>x.growth_rpm));result.candidate_rpm=result.max_growth_rpm+margin_rpm;}
    else result.issues.push('没有足够的完整窗口');return result;
  }
  function envelope(reports){
    const wheels=new Set(),candidates=[];
    for(const r of reports)if(r.quality?.complete&&r.analysis?.valid&&r.pulse&&Number.isInteger(r.pulse.wheel)){
      wheels.add(r.pulse.wheel);candidates.push(r.analysis.candidate_rpm);
    }
    return {four_wheels_complete:wheels.size===4,candidate_rpm:candidates.length?Math.max(...candidates):null,wheels:[...wheels]};
  }
  function torqueRows({measurements=[]}){return measurements.map(m=>{
    const force=Number.isFinite(m.force_n)?m.force_n:Number.isFinite(m.mass_kg)?m.mass_kg*9.81:null;
    const torque=force!==null&&Number.isFinite(m.arm_m)?force*m.arm_m:null;
    const command_a=Number.isFinite(m.current)?m.current*20/16384:null;
    const span=Number.isFinite(m.upper_current)&&Number.isFinite(m.lower_current)?Math.abs(m.upper_current-m.lower_current):null;
    return {...m,force_n:force,torque_nm:torque,command_a,k_out_candidate_nm_a:command_a?torque/Math.abs(command_a):null,
      threshold_span_command:span,needs_review:true,issues:[...(torque===null?['测量条件缺失']:[]),...(span>Math.abs(m.current)*.2?['阈值差较大，复核摩擦和误差']:[])]};});}
  function toJson(report){return JSON.stringify({format:1,source_commit:null,voltage_v:null,pulse:null,conditions:{},...report},null,2);}
  function toCsv(report){
    const keys=['type','wheel','state','flags','sequence','time_us','feedback_time_us','raw_rpm','raw_current','used_rpm','requested','final_lf','final_rf','final_lb','final_rb','temperature','enabled_mask','cycle_us','prepare_us','submit_us','reason','attempted','successful'];
    const cell=v=>v===null||v===undefined?'':`"${String(v).replaceAll('"','""')}"`;
    return ['# force calibration; CAN current units; rotor rpm; board relative us',`# metadata=${JSON.stringify({...report,records:undefined})}`,keys.join(','),
      ...(report.records||[]).map(r=>keys.map(k=>cell(k.startsWith('final_')?r.final?.[['final_lf','final_rf','final_lb','final_rb'].indexOf(k)]:r[k])).join(','))].join('\r\n')+'\r\n';
  }
  return {analyzeGrowth,torqueRows,toCsv,toJson,quality,envelope};
});
