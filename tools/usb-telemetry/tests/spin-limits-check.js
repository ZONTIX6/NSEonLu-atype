async (page) => {
  const check=(ok,msg)=>{if(!ok) throw new Error(msg);};
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://127.0.0.1:4173');
  await page.evaluate(()=>{
    window.limitRow=(state,version=5,extra={})=>{
      const P=TelemetryProtocol,fields=version===4?P.V4_FIELDS:P.FIELDS;
      const row={...Object.fromEntries(fields.map(k=>[k,0])),version,seq:state+1,tick:1000,
        chassis_valid:1,chassis_tick_ms:1000,spin_limit_state:state,spin_extension_enabled:1,
        spin_soft_wz_milli:8000000,spin_hard_wz_milli:10000000,spin_effective_wz_milli:10000000,
        spin_body_hard_mrps:2000,spin_wheel_hard_rpm:12000,spin_extension_confirm_ms:150,
        spin_power_gap_cw:2500,...extra};
      telemetryApp.parser.push(fields.join(',')+'\n'+fields.map(k=>row[k]).join(',')+'\n');
    };
  });
  for(const [state,label] of [[1,'软上限确认中'],[2,'扩展中'],[3,'跟随受限'],[4,'已达硬上限']]) {
    await page.evaluate(s=>limitRow(s),state);
    await page.waitForFunction(label=>document.querySelector('#spin-limit-status').textContent===label,label);
  }
  await page.evaluate(()=>limitRow(4,5,{spin_block_reasons:4096,spin_capability_latched:1,spin_capability_release_ms:200}));
  await page.waitForFunction(()=>document.querySelector('#spin-limit-detail').textContent.includes('轮速硬上限'));
  check((await page.locator('#spin-latch-detail').innerText()).includes('200 / 500'),'latch release display');
  check(await page.locator('#spin-soft').innerText()==='8000','Wz units');
  await page.setViewportSize({width:375,height:900});
  check(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'mobile horizontal overflow');
  await page.locator('#velocity-panel').screenshot({path:'output/playwright/spin-limits-mobile.png'});
  await page.evaluate(()=>limitRow(0,4));
  await page.waitForFunction(()=>document.querySelector('#spin-limit-status').textContent==='固件未提供上限诊断');
  check(await page.locator('#spin-soft').innerText()==='—','legacy must not fabricate zero');
  await page.getByRole('button',{name:'演示数据',exact:true}).click();
  await page.waitForFunction(()=>document.querySelector('#spin-limit-status').textContent==='扩展中');
  await page.setViewportSize({width:1440,height:1000});
  await page.locator('#velocity-panel').screenshot({path:'output/playwright/spin-limits-desktop.png'});
  check(errors.length===0,errors.join(';'));
  return {passed:true,cases:['states','reason','latch','units','v4 absence','375px','demo']};
}
