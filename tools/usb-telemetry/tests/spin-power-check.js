async (page) => {
  const check=(ok,text)=>{if(!ok) throw new Error(text);};
  const errors=[];page.on('pageerror',error=>errors.push(error.message));
  await page.goto('http://127.0.0.1:4173');
  await page.evaluate(()=>{
    const P=TelemetryProtocol,fields=P.FIELDS;
    const row={...Object.fromEntries(fields.map(key=>[key,0])),version:3,seq:1,tick:1000,
      chassis_valid:1,chassis_tick_ms:1000,spin_power_phase:1,spin_block_reasons:4,
      spin_base_power_cw:7840,spin_buffer_request_cw:6000,spin_buffer_grant_cw:2000,
      spin_power_ceiling_cw:10000,power_tuning_source:2,spin_tuning_source:3,
      buffer_control_enabled:1,buffer_estimate_valid:1,referee_online:1,buffer_j:50};
    telemetryApp.parser.push(P.HEADER+'\n'+fields.map(key=>row[key]).join(',')+'\n');
  });
  await page.waitForFunction(()=>document.querySelector('#buffer-state').textContent==='基础功率跟踪');
  const title=await page.locator('#buffer-state').getAttribute('title');
  check(title.includes('模型限流')&&title.includes('板端 Flash')&&title.includes('运行时调参'),'Policy reasons/source missing');
  const picker=page.locator('[data-chart="power-chart"]');
  check(await picker.getByRole('checkbox').count()===9,'Missing spin power traces');
  check(!await picker.locator('[data-trace-key="spin_buffer_grant_cw"]').isChecked(),'New traces should be opt-in');
  await picker.locator('[data-trace-key="spin_buffer_grant_cw"]').check();
  check(await picker.locator('[data-trace-key="spin_buffer_grant_cw"]').isChecked(),'Cannot select buffer grant');
  await page.setViewportSize({width:375,height:812});
  check(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'Mobile overflow');
  check(errors.length===0,errors.join('; '));
  return {passed:true,cases:['v3 stage','growth reasons','parameter sources','new power traces','375px']};
}
