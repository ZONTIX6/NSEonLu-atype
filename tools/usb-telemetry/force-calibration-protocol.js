(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.ForceCalibrationProtocol=api;})(globalThis,function(){
  'use strict';
  const states=['未准备','静止确认中','已准备','预记录','上升','保持','下降','尾段记录','数据冻结','异常终止'];
  const reasons=['无','手动停止','USB 断开或世代变化','心跳超时','安全/台架许可失效','反馈失效','超速','超温','功率/预算失效','控制周期超时','CAN 提交失败','记录容量不足','协议错误'];
  const capKeys=['generation','enabled','format','record_bytes','record_capacity','max_current','max_on_ms','max_rpm','stop_temperature_c','stationary_rpm','stationary_ms','heartbeat_ms','pre_ms','tail_ms','source_mode','power_stage','bench_mode','normal_calibrated','lf_id','rf_id','lb_id','rb_id','speed_lpf_pm','current_lpf_pm'];
  const statusKeys=['generation','session','capture_id','capture_session','capture_generation','state','reason','records','complete','zero_submitted','requested','final_lf','final_rf','final_lb','final_rb','raw_rpm','used_rpm_milli','temperature','feedback_age_us','load_mw','budget_mw'];
  const metaKeys=['generation','capture_id','capture_session','capture_generation','wheel','current','rise_ms','hold_ms','fall_ms','max_current','max_on_ms','max_rpm','stop_temperature_c','stationary_rpm','stationary_ms','heartbeat_ms','pre_ms','tail_ms','speed_lpf_pm','current_lpf_pm'];
  function integer(s,signed=false){
    if(!(signed?/^-?\d+$/:/^\d+$/).test(s))throw Error('协议整数字段格式错误');
    const n=Number(s);if(!Number.isSafeInteger(n)||n<(signed?-2147483648:0)||n>(signed?2147483647:4294967295))throw Error('协议整数越界');return n;
  }
  function crc16(text){let crc=0xffff;for(const c of text){crc^=c.charCodeAt(0);for(let k=0;k<8;k++)crc=crc&1?(crc>>>1)^0xa001:crc>>>1;}return crc;}
  function decodeRecord(bytes){
    if(!(bytes instanceof Uint8Array)||bytes.length!==48)throw Error('记录必须为 48 字节');
    const v=new DataView(bytes.buffer,bytes.byteOffset,48),type=['','RX','CONTROL','MARKER'][bytes[0]];
    if(!type||bytes[1]>3||bytes[2]>9||v.getUint16(44,true)>12)throw Error('记录类型或状态错误');
    const r={type,wheel:bytes[1],state:bytes[2],flags:bytes[3],sequence:v.getUint32(4,true),time_us:v.getUint32(8,true),
      feedback_time_us:v.getInt32(12,true),raw_rpm:v.getInt16(16,true),raw_current:v.getInt16(18,true),
      used_rpm:type==='RX'?null:v.getFloat32(20,true),requested:type==='RX'?null:v.getInt16(24,true),
      final:type==='RX'?null:[26,28,30,32].map(i=>v.getInt16(i,true)),temperature:bytes[34],enabled_mask:bytes[35],
      cycle_us:type==='RX'?null:v.getUint32(36,true),prepare_us:type==='RX'?null:v.getUint16(40,true),
      submit_us:type==='RX'?null:v.getUint16(42,true),reason:v.getUint16(44,true),attempted:bytes[46],successful:bytes[47]};
    r[type==='RX'?'feedback_seq':'cycle_seq']=r.sequence;
    if(r.used_rpm!==null&&!Number.isFinite(r.used_rpm)){r.used_rpm=null;r.quality_error='滤波转速非有限数';}
    return r;
  }
  function parseLine(line){
    const f=line.split(',');if(f[0]!=='@FC1')return null;
    const id=integer(f[1]),type=f[2];
    if(type==='ERR'){
      if(f.length!==5||!['disabled','format','busy','generation','session','guard','range','not_frozen','not_zero','capture','index','ownership'].includes(f[4]))throw Error('错误回执格式错误');
      return {id,type,generation:integer(f[3]),error:f[4]};
    }
    if(type==='DATA'){
      if(f.length!==11||!/^[0-9a-fA-F]{4}$/.test(f[10])||crc16(line.slice(0,line.lastIndexOf(',')))!==parseInt(f[10],16))throw Error('记录块 CRC 错误');
      const [generation,session,capture_id,first,count,total]=f.slice(3,9).map(s=>integer(s));
      if(count<1||count>2||first+count>total||!/^[0-9a-fA-F]+$/.test(f[9])||f[9].length!==count*96)throw Error('记录块长度或索引错误');
      const bytes=Uint8Array.from(f[9].match(/../g),x=>parseInt(x,16));
      return {id,type,generation,session,capture_id,first,count,total,bytes};
    }
    if(type==='META'){
      if(f.length!==metaKeys.length+3)throw Error('元数据字段数量错误');
      const r={id,type};metaKeys.forEach((key,i)=>{r[key]=integer(f[i+3],key==='current');});
      if(r.wheel>3||!r.current||Math.abs(r.current)>r.max_current||r.max_current>1500||r.rise_ms<1||r.hold_ms<1||r.fall_ms<1||r.rise_ms+r.hold_ms+r.fall_ms>r.max_on_ms||r.max_on_ms>500)throw Error('冻结脉冲元数据越界');
      return r;
    }
    const keys=type==='CAP'?capKeys:type==='OK'?statusKeys:null;
    if(!keys||f.length!==keys.length+3)throw Error('回执字段数量错误');
    const r={id,type};keys.forEach((key,i)=>{r[key]=integer(f[i+3],type==='OK'&&['requested','final_lf','final_rf','final_lb','final_rb','raw_rpm','used_rpm_milli','load_mw','budget_mw'].includes(key));});
    for(const key of type==='CAP'?['enabled','bench_mode','normal_calibrated']:['complete','zero_submitted'])if(r[key]!==0&&r[key]!==1)throw Error('布尔字段错误');
    if(type==='CAP'){
      if(r.record_capacity>65535||r.max_current>32767||r.max_on_ms>65535||r.speed_lpf_pm>1000||r.current_lpf_pm>1000)throw Error('能力字段越界');
    }else{
      if(r.state>9||r.reason>12||r.records>65535||r.temperature>255||[r.requested,r.raw_rpm,r.final_lf,r.final_rf,r.final_lb,r.final_rb].some(x=>x < -32768||x>32767))throw Error('状态字段越界');
      r.final=[r.final_lf,r.final_rf,r.final_lb,r.final_rb];r.used_rpm=r.used_rpm_milli===-2147483648?null:r.used_rpm_milli/1000;
      r.load_w=r.load_mw===-2147483648?null:r.load_mw/1000;r.budget_w=r.budget_mw===-2147483648?null:r.budget_mw/1000;
    }
    return r;
  }
  function createParser({onReply=()=>{},onError=()=>{}}={}){
    let line='',dropping=false;
    return {reset(){line='';dropping=false;},push(bytes){for(const b of bytes){
      if(b===10){if(!dropping){try{const reply=parseLine(line.replace(/\r$/,''));if(reply)onReply(reply);}catch(e){onError(e);}}line='';dropping=false;}
      else if(b===0||b>127||line.length>=511){if(!dropping&&line.startsWith('@FC1'))onError(Error('协议行损坏或过长'));dropping=true;line='';}
      else if(!dropping)line+=String.fromCharCode(b);
    }}};
  }
  return {createParser,decodeRecord,parseLine,crc16,states,reasons};
});
