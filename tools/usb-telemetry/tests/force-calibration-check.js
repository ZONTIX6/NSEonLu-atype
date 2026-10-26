// Playwright CLI run-code --filename. All serial I/O uses a synthetic in-page port.
async (page)=>{
  const check=(ok,msg)=>{if(!ok)throw Error(msg);};const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(()=>{
    window.__fcCommands=[];window.__portCounts={choose:0,open:0,reader:0,dtr:0};let controller,session=0,state=0,records=[],generation=9;
    const encode=new TextEncoder();
    const status=id=>`@FC1,${id},OK,${generation},${session},${state>=3?4:0},${session},${generation},${state},0,${records.length},1,${window.__holdStop?0:1},0,0,0,0,0,0,0,28,1000,2000,50000\r\n`;
    const port={async open(){++__portCounts.open;},async setSignals(){++__portCounts.dtr;},async close(){},
      get readable(){return new ReadableStream({start(c){controller=c;},cancel(){}});},
      get writable(){return new WritableStream({write(bytes){
        const line=new TextDecoder().decode(bytes);window.__fcCommands.push(line);const f=line.trim().split(','),id=f[1],op=f[2];let reply;
        if(f[0]==='@T1')reply=`@T1,${id},ERR,${f[3]},unavailable\r\n`;
        else if(f[0]==='@A2'||f[0]==='@A1')reply=`${f[0]},${id},ERR,disabled\r\n`;
        else if(op==='CAP'){
          if(location.search.includes('old-firmware'))return;
          reply=`@FC1,${id},CAP,${generation},${location.search.includes('disabled')?0:1},1,48,1280,1500,500,1000,50,20,100,150,50,100,0,3,1,0,1,4,2,3,850,900\r\n`;
        }
        else if(op==='ARM'){++session;state=2;reply=status(id);}
        else if(op==='START'){
          state=5;reply=status(id);records=[];
          for(let k=0;k<12;k++){
            const bytes=new Uint8Array(48),v=new DataView(bytes.buffer);bytes[0]=k%2?2:1;bytes[1]=Number(f[5]);bytes[2]=5;
            v.setUint32(4,Math.floor(k/2)+1,true);v.setUint32(8,Math.floor(k/2)*2000,true);v.setInt16(16,k*10,true);
            v.setFloat32(20,k*5,true);v.setInt16(24,300,true);v.setInt16(28,300,true);bytes[34]=28;bytes[35]=15;v.setUint32(36,2000,true);bytes[46]=2;bytes[47]=2;records.push(bytes);
            if(k===11){v.setInt16(24,0,true);v.setInt16(28,0,true);}
          }
          setTimeout(()=>{state=8;},250);
        }else if(op==='STOP'){state=9;reply=status(id);}
        else if(op==='CLEAR'){records=[];state=0;reply=status(id);}
        else if(op==='META')reply=`@FC1,${id},META,${generation},4,${session},${generation},1,300,50,100,50,1500,500,1000,50,20,100,150,50,100,850,900\r\n`;
        else if(op==='READ'){
          const first=Number(f[5]),count=Number(f[6]);const hex=records.slice(first,first+count).flatMap(b=>[...b]).map(n=>n.toString(16).padStart(2,'0')).join('');
          const prefix=`@FC1,${id},DATA,${generation},${session},4,${first},${count},${records.length},${hex}`;
          let crc=65535;for(const c of prefix){crc^=c.charCodeAt(0);for(let k=0;k<8;k++)crc=crc&1?(crc>>>1)^0xa001:crc>>>1;}
          reply=prefix+','+crc.toString(16).padStart(4,'0')+'\r\n';
        }else reply=status(id);
        queueMicrotask(()=>controller.enqueue(encode.encode(reply)));
      }});}};
    window.__serialFeed=line=>controller.enqueue(encode.encode(line));
    Object.defineProperty(navigator,'serial',{configurable:true,value:location.search.includes('no-serial')?null:{async requestPort(){++__portCounts.choose;if(location.search.includes('occupied'))throw Error('串口已被其他页面占用，请先断开');return port;},addEventListener(){}}});
  });
  await page.goto('about:blank');await page.goto('http://127.0.0.1:4176/#force-calibration');
  check(await page.locator('.workspace-tabs [role=tab]').count()===4,'仪表盘应有四个一级标签');
  await page.locator('#force-calibration-tab').focus();await page.keyboard.press('Home');
  await page.waitForFunction(()=>document.activeElement.id==='monitor-tab');await page.keyboard.press('End');
  await page.waitForFunction(()=>document.activeElement.id==='force-calibration-tab');
  check(await page.locator('#fc-connect').count()===0,'标定面板不能创建第二套串口连接入口');
  check(await page.locator('#fc-start').isDisabled(),'连接前禁止开始');
  await page.locator('#connect-button').click();await page.waitForFunction(()=>document.querySelector('#fc-status').textContent.includes('已连接'));
  check(await page.evaluate(()=>!__fcCommands.some(x=>x.includes(',START,')||x.includes(',ARM,'))),'连接不能准备或启动');
  await page.locator('#fc-arm').click();await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('已准备'));
  check(await page.locator('#auto-start').isDisabled()&&await page.locator('#tuning-apply').isDisabled(),'标定会话须锁定整定和调参');
  check(await page.locator('#control-status-bar').isVisible()&&await page.locator('#fc-stop').isEnabled(),'全局停止须可见可用');
  check(await page.locator('#fc-start').isDisabled(),'夹具未确认禁止开始');await page.locator('#fc-fixture').check();
  await page.locator('#fc-start').click();await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('数据冻结'));
  await page.locator('#fc-read').click();await page.waitForFunction(()=>document.querySelector('#fc-quality').textContent.includes('12 条'));
  check(await page.locator('#fc-window').inputValue()===''&&await page.locator('#fc-margin').inputValue()==='','T 和裕量初始为空');
  await page.locator('#fc-window').fill('10');await page.locator('#fc-margin').fill('20');await page.locator('#fc-gap').fill('2500');
  await page.locator('#fc-voltage').fill('24.3');await page.locator('#fc-commit').fill('abc123');await page.locator('#fc-conditions').fill('模拟夹具条件');
  const downloadPromise=page.waitForEvent('download');await page.locator('#fc-json').click();const download=await downloadPromise;
  await download.saveAs('output/playwright/force-calibration-capture.json');
  const csvPromise=page.waitForEvent('download');await page.locator('#fc-csv').click();await (await csvPromise).saveAs('output/playwright/force-calibration-capture.csv');
  await page.setViewportSize({width:1440,height:1000});await page.screenshot({path:'output/playwright/force-calibration-desktop.png',fullPage:true});
  await page.setViewportSize({width:390,height:844});check(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'390px 无横向溢出');
  await page.screenshot({path:'output/playwright/force-calibration-mobile.png',fullPage:true});
  const beforeSwitch=await page.evaluate(()=>({...__portCounts}));
  await page.locator('#fc-arm').click();
  await page.evaluate(()=>{document.querySelector('#tuning-tab').click();document.querySelector('#monitor-tab').click();});
  await page.waitForFunction(()=>document.querySelector('#monitor-tab').getAttribute('aria-selected')==='true');
  check(await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',STOP,')).length)===1,'多次切换只执行一次停止');
  check(await page.evaluate(count=>JSON.stringify(__portCounts)===JSON.stringify(count),beforeSwitch),'切换不重新开端口或改变 DTR');
  await page.locator('#force-calibration-tab').click();await page.locator('#fc-arm').click();
  await page.evaluate(()=>{window.__holdStop=true;});await page.locator('#monitor-tab').click();
  await page.waitForFunction(()=>telemetryApp.coordinator.phase==='unknown');
  check(await page.locator('#force-calibration-tab').getAttribute('aria-selected')==='true','停止未确认不能切走');
  check(await page.locator('#fc-stop').isEnabled()&&await page.locator('#auto-start').isDisabled(),'未确认时停止仍可用、控制仍锁定');
  await page.evaluate(()=>{window.__holdStop=false;});await page.locator('#fc-stop').click();
  await page.waitForFunction(()=>telemetryApp.coordinator.owner===null);await page.locator('#monitor-tab').click();
  await page.evaluate(()=>{
    const F=ForceTelemetry;const values=Object.fromEntries(F.FIELDS.map(k=>[k,0]));Object.assign(values,{backend:1,tick:100,cycle_seq:1,feedback_valid:1,can_submitted:1,load_cW:1234,model_allow_cW:5000,rpm_rf:-1234,feedback_age_rf:2,feedback_age_lf:2,feedback_age_lb:2,feedback_age_rb:2,omega_mrad_s:1000});
    __serialFeed(F.HEADER+'\r\n'+F.FIELDS.map((k,i)=>i?values[k]:'force-v1').join(',')+'\r\n');
  });
  await page.waitForFunction(()=>document.querySelector('#metric-meter').textContent==='12.34');
  check(await page.locator('#imu-yaw').innerText()==='—'&&await page.locator('#rc-lx').innerText()==='—','force-v1 不提供的来源须为空');
  await page.locator('#pause-button').click();
  await page.locator('#force-calibration-tab').click();await page.locator('#fc-arm').click();
  await page.waitForFunction(()=>__fcCommands.filter(x=>x.includes(',PING,')).length>0);
  await page.locator('#fc-stop').click();await page.waitForFunction(()=>telemetryApp.coordinator.owner===null);
  await page.locator('#connect-button').click();check(await page.locator('#fc-start').isDisabled(),'断开后禁止开始');
  await page.locator('#connect-button').click();await page.waitForFunction(()=>document.querySelector('#fc-status').textContent.includes('已连接'));
  check(await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',START,')).length===1),'重新连接不得恢复测试');
  await page.locator('#fc-arm').click();await page.locator('#fc-fixture').check();await page.locator('#fc-start').click();await page.locator('#fc-stop').click();
  await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('异常终止'));
  const starts=await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',START,')).length);
  await page.locator('#fc-voltage').fill('');await page.locator('#fc-commit').fill('');await page.locator('#fc-window').fill('');
  await page.locator('#fc-import').setInputFiles('output/playwright/force-calibration-capture.json');
  await page.waitForFunction(()=>document.querySelector('#fc-voltage').value==='24.3');
  check(await page.locator('#fc-commit').inputValue()==='abc123'&&await page.locator('#fc-window').inputValue()==='10'&&await page.locator('#fc-gap').inputValue()==='2500','导入必须保留原测量条件');
  await page.locator('#fc-analyze').click();await page.locator('#fc-margin').fill('999');
  const changedPromise=page.waitForEvent('download');await page.locator('#fc-json').click();await (await changedPromise).saveAs('output/playwright/force-calibration-changed.json');
  check(await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',START,')).length)===starts,'导入历史不得开始');
  await page.locator('#fc-arm').click();await page.evaluate(()=>{Object.defineProperty(document,'hidden',{configurable:true,value:true});document.dispatchEvent(new Event('visibilitychange'));});
  await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('异常终止'));
  await page.reload();check(await page.locator('#fc-start').isDisabled(),'刷新不得恢复开始');
  check(await page.evaluate(()=>__fcCommands.length===0),'刷新不得自动发送控制命令');
  for(const mode of ['disabled','old-firmware','occupied','no-serial']){
    await page.goto('http://127.0.0.1:4176/?'+mode+'#force-calibration');
    if(mode!=='no-serial')await page.locator('#connect-button').click();
    const text={disabled:'disabled','old-firmware':'未提供 FC1',occupied:'占用','no-serial':'Web Serial'}[mode];
    await page.waitForFunction(text=>document.querySelector('#fc-message').textContent.includes(text),text);
    check(await page.locator('#fc-start').isDisabled(),mode+' 禁止开始');
  }
  await page.goto('http://127.0.0.1:4176/force-calibration.html');
  await page.waitForURL('http://127.0.0.1:4176/#force-calibration');
  check(await page.evaluate(()=>__fcCommands.length===0),'兼容入口跳转不得打开或控制串口');
  check(errors.length===0,errors.join('\n'));return {passed:true,errors,downloads:3};
}
