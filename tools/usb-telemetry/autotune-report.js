(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.AutotuneReport=api;})(globalThis,function(){
  'use strict';
  function reportCSV(records){
    const fields=['wheel','stage','rpm','role','pair','kp','ki','kd','session','trial','reason','valid','passes','score','rmse_rpm','overshoot_rpm','steady_rpm','settle_ms','current_tv_raw_per_s','current_limit_raw','limited_pm','window_ms','coverage_ms','control_count','feedback_count','max_feedback_ms','max_stat_ms','failures','time'];
    const escape=v=>'"'+String(v??'').replaceAll('"','""')+'"';
    return '\ufeff'+[fields,...records.map(r=>{const m=r.metrics,e=r.evaluation;return [r.wheel,r.stage,r.rpm,r.role,r.pair,...r.values,m.session,m.trial,m.reason,e.valid,e.passes,Number.isFinite(e.score)?e.score:'',m.rmse,m.overshoot,m.steady,m.settle,m.jitter,m.currentLimit,m.limited,m.window,m.coverage,m.samples,m.feedbackCount,m.maxFeedback,m.maxStat,e.failures.join('|'),r.time];})].map(row=>row.map(escape).join(',')).join('\r\n');
  }
  return {reportCSV};
});
