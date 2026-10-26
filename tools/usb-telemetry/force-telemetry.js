(function(root,factory){const api=factory(typeof module==='object'&&module.exports?require('./protocol.js'):root.TelemetryProtocol);if(typeof module==='object'&&module.exports)module.exports=api;else root.ForceTelemetry=api;})(globalThis,function(P){
  'use strict';
  const FIELDS=('force-v1,tick,command_seq,cycle_seq,config_version,safety_epoch,policy_version,backend,exit,spin,source,calibrated,calibration_checksum,state,faults,limits,can_submitted,dt_us,fg_x_mN,fg_y_mN,fx_request_mN,fy_request_mN,mz_request_mNm,fx_sent_model_mN,fy_sent_model_mN,mz_sent_model_mNm,vx_mm_s,vy_mm_s,omega_mrad_s,residual_mm_s,alpha_pm,beta_pm,predicted_cW,track_cW,load_cW,torque_mNm,growth_allowed,offer_mNm,offer_cW,operating_cW,braking,brake_timed_out,zero_over_budget,command_age_ms,source_age_ms,yaw_age_ms,raw_lf,raw_rf,raw_lb,raw_rb,allocated_lf,allocated_rf,allocated_lb,allocated_rb,sent_lf,sent_rf,sent_lb,sent_rb,rpm_lf,rpm_rf,rpm_lb,rpm_rb,feedback_current_lf,feedback_current_rf,feedback_current_lb,feedback_current_rb,temperature_lf,temperature_rf,temperature_lb,temperature_rb,feedback_age_lf,feedback_age_rf,feedback_age_lb,feedback_age_rb,nominal_cW,model_allow_cW,source_ceiling_cW,referee_W,buffer_lower_cJ,buffer_recovering,cap_energy_pm,cap_available_W,input_actual_cW,input_request_W,input_confirmed_W,input_confirmed,input_confirmed_age_ms,cap051_age_ms,cap052_age_ms,cap061_failures,confirmation_timeouts,follow_translation_only,feedback_valid,input_us,estimate_us,command_us,allocation_us,guard_us,can_us').split(',');
  const HEADER=FIELDS.join(','),wheels=['lf','rf','lb','rb'];
  const booleans=['calibrated','can_submitted','growth_allowed','braking','brake_timed_out','zero_over_budget','buffer_recovering','input_confirmed','follow_translation_only','feedback_valid'];
  function parseLine(line){
    if(line===HEADER)return null;
    const cells=line.split(',');if(cells[0]!=='force-v1'||cells.length!==FIELDS.length)throw Error('force-v1 字段数量不匹配');
    const r={force:true,rawLine:line};for(let k=1;k<cells.length;k++){if(!/^-?\d+$/.test(cells[k]))throw Error('force-v1 整数字段无效');const value=Number(cells[k]);if(!Number.isInteger(value)||value < -2147483648||value>4294967295)throw Error('force-v1 数值越界');r[FIELDS[k]]=value;}
    if(booleans.some(k=>r[k]!==0&&r[k]!==1)||r.backend!==1||r.source>1||r.source<0)throw Error('force-v1 模式或布尔字段无效');return r;
  }
  function normalize(raw,lag=0){
    const values=Object.fromEntries(P.FIELDS.map(k=>[k,null])),fresh=lag<=500&&lag>=0&&lag+(raw.sourceLagMs??0)<=100;
    const feedback=fresh&&raw.cycle_seq>0&&raw.feedback_valid===1&&wheels.every(w=>raw['feedback_age_'+w]>=0&&raw['feedback_age_'+w]<=5);
    const valid={chassis:feedback,velocityActual:feedback,velocityTarget:false,model:feedback,remote:false,imu:false,meter:feedback,referee:false,bufferEstimate:false,cap:fresh&&raw.source===1&&raw.cap051_age_ms<=20,capInput:fresh&&raw.source===1&&raw.cap052_age_ms<=20};
    const ages={chassis:feedback?Math.max(...wheels.map(w=>raw['feedback_age_'+w])):P.UINT_MAX,model:P.UINT_MAX,meter:P.UINT_MAX,remote:P.UINT_MAX,imu:P.UINT_MAX,buffer:P.UINT_MAX,cap:raw.cap051_age_ms,capInput:raw.cap052_age_ms};
    const motors=wheels.map(key=>({key,online:feedback,enabled:feedback&&raw.can_submitted===1,rpm:feedback?raw['rpm_'+key]:null,target:null,currentCommand:feedback&&raw.can_submitted?raw['sent_'+key]:null,currentFeedback:feedback?raw['feedback_current_'+key]:null}));
    for(const m of motors){values['rpm_'+m.key]=m.rpm;values['i_'+m.key+'_cmd']=m.currentCommand;values['i_'+m.key+'_fb']=m.currentFeedback;}
    if(feedback)Object.assign(values,{p_meter_cw:raw.load_cW/100,effective_limit_cw:raw.model_allow_cW/100,nominal_budget_cw:raw.nominal_cW/100,target_power_cw:raw.track_cW/100,p_unlimited_cw:raw.predicted_cW/100,p_limited_cw:raw.operating_cW/100,
      velocity_actual_vx_mmps:raw.vx_mm_s/1000,velocity_actual_vy_mmps:raw.vy_mm_s/1000,velocity_actual_wz_rpm:raw.omega_mrad_s/1000*60/(2*Math.PI)});
    if(valid.cap)Object.assign(values,{cap_energy_pm:raw.cap_energy_pm/10,cap_limit_cw:raw.cap_available_W});
    if(valid.capInput)values.cap_input_cw=raw.input_actual_cW/100;
    if(fresh)values.power_source=raw.source;
    return {raw,values,valid,ages,motors};
  }
  class Store{
    constructor(){this.clear();}
    clear(){this.samples=[];this.latest=null;this.stats={missing:0,received:0};this.progressMs=0;this.previousCycle=null;}
    add(raw,arrivalMs){if(this.previousCycle===null||raw.cycle_seq!==this.previousCycle){this.progressMs=arrivalMs;this.previousCycle=raw.cycle_seq;}
      const data={...raw,sourceLagMs:Math.max(0,arrivalMs-this.progressMs)};
      const entry={...data,arrivalMs,version:0,segment:0,normalized:normalize(data)};this.samples.push(entry);this.latest=entry;++this.stats.received;
      while(this.samples.length>30000||(this.samples.length>1&&arrivalMs-this.samples[0].arrivalMs>600000))this.samples.shift();return entry;}
    isStale(now){return !this.latest||now-this.latest.arrivalMs>500;}
    exportCSV(){return [HEADER,...this.samples.map(x=>x.rawLine)].join('\r\n')+'\r\n';}
  }
  return {FIELDS,HEADER,parseLine,normalize,Store};
});
