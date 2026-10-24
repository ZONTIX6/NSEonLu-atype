async (page) => {
  const check=(ok,msg)=>{if(!ok) throw new Error(msg);};
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://127.0.0.1:4173');
  check(await page.locator('#velocity-status').count()===1,'Missing chassis velocity panel');
  await page.evaluate(()=>{
    window.velocityRow=(version=4,extras={})=>{
      const P=TelemetryProtocol,fields=version===3?P.V3_FIELDS:P.FIELDS;
      const row={...Object.fromEntries(fields.map(k=>[k,0])),version,seq:1,tick:1000,
        velocity_actual_valid:1,velocity_actual_tick_ms:995,velocity_actual_vx_mmps:1234,
        velocity_actual_vy_mmps:-567,velocity_actual_wz_mrps:-1250,
        velocity_target_valid:1,velocity_target_tick_ms:1000,velocity_target_vx_mmps:1500,
        velocity_target_vy_mmps:-600,velocity_target_wz_mrps:-1500,...extras};
      telemetryApp.parser.push(fields.join(',')+'\n'+fields.map(k=>row[k]).join(',')+'\n');
    };
    velocityRow();
  });
  await page.waitForFunction(()=>document.querySelector('#velocity-actual-vx').textContent==='1.234');
  check(await page.locator('#velocity-actual-vy').innerText()==='-0.567','Lateral units/sign');
  check(await page.locator('#velocity-actual-wz').innerText()==='-1.250','Spin must be rev/s');
  check(await page.locator('#velocity-target-wz').innerText()==='-1.500','Target units/sign');
  check(await page.locator('[data-chart="velocity-chart"] input').count()===6,'Need six target/actual traces');
  await page.locator('#pause-button').click();
  await page.evaluate(()=>velocityRow(4,{seq:2,velocity_actual_vx_mmps:2500}));
  check(await page.locator('#velocity-actual-vx').innerText()==='1.234','Pause must freeze');
  await page.locator('#pause-button').click();
  await page.waitForFunction(()=>document.querySelector('#velocity-actual-vx').textContent==='2.500');
  await page.evaluate(()=>velocityRow(4,{seq:3,velocity_actual_tick_ms:979}));
  await page.waitForFunction(()=>document.querySelector('#velocity-actual-vx').textContent==='—');
  check(await page.locator('#velocity-target-vx').innerText()==='1.500','Actual stale must not hide target');
  await page.evaluate(()=>velocityRow(3,{seq:4}));
  await page.waitForFunction(()=>document.querySelector('#velocity-status').textContent.includes('未提供'));
  check(await page.locator('#velocity-target-vx').innerText()==='—','Legacy no invented zeros');
  await page.evaluate(()=>velocityRow(4,{seq:5}));
  for(const width of [1440,375,320]) {
    await page.setViewportSize({width,height:900});
    check(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'Overflow '+width);
  }
  await page.locator('[aria-label="放大底盘整体速度"]').click();
  check(await page.locator('#expanded-chart-unit').innerText()==='m/s / 圈/s','Chart axes units');
  await page.locator('#close-chart-button').click();
  await page.setViewportSize({width:1440,height:1000});
  await page.evaluate(()=>velocityRow(4,{seq:6}));
  await page.locator('#velocity-panel').scrollIntoViewIfNeeded();
  await page.locator('#pause-button').click();
  await page.screenshot({path:'build/velocity-ui.png',fullPage:true});
  check(errors.length===0,errors.join('; '));
  return {passed:true,cases:['physical units','target vs actual','six curves','pause','stale','v3','mobile','expanded axes']};
}
