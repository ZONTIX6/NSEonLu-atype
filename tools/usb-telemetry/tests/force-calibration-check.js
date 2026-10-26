// Playwright CLI run-code --filename. All serial I/O uses a synthetic in-page port.
async (page)=>{
  const check=(ok,msg)=>{if(!ok)throw Error(msg);};const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(()=>{
    window.__fcCommands=[];let controller,session=0,state=0,records=[],generation=9;
    const encode=new TextEncoder();
    const status=id=>`@FC1,${id},OK,${generation},${session},${state>=3?4:0},${session},${generation},${state},0,${records.length},1,1,0,0,0,0,0,0,0,28,1000,2000,50000\r\n`;
    const port={async open(){},async setSignals(){},async close(){},
      get readable(){return new ReadableStream({start(c){controller=c;},cancel(){}});},
      get writable(){return new WritableStream({write(bytes){
        const line=new TextDecoder().decode(bytes);window.__fcCommands.push(line);const f=line.trim().split(','),id=f[1],op=f[2];let reply;
        if(op==='CAP'){
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
          }
          setTimeout(()=>{state=8;},250);
        }else if(op==='STOP'){state=9;reply=status(id);}
        else if(op==='CLEAR'){records=[];state=0;reply=status(id);}
        else if(op==='READ'){
          const first=Number(f[5]),count=Number(f[6]);const hex=records.slice(first,first+count).flatMap(b=>[...b]).map(n=>n.toString(16).padStart(2,'0')).join('');
          const prefix=`@FC1,${id},DATA,${generation},${session},4,${first},${count},${records.length},${hex}`;
          let crc=65535;for(const c of prefix){crc^=c.charCodeAt(0);for(let k=0;k<8;k++)crc=crc&1?(crc>>>1)^0xa001:crc>>>1;}
          reply=prefix+','+crc.toString(16).padStart(4,'0')+'\r\n';
        }else reply=status(id);
        queueMicrotask(()=>controller.enqueue(encode.encode(reply)));
      }});}};
    Object.defineProperty(navigator,'serial',{configurable:true,value:location.search.includes('no-serial')?null:{async requestPort(){if(location.search.includes('occupied'))throw Error('串口已被其他页面占用，请先断开');return port;},addEventListener(){}}});
  });
  await page.goto('http://127.0.0.1:4176/force-calibration.html');
  check(await page.locator('#fc-start').isDisabled(),'连接前禁止开始');
  await page.locator('#fc-connect').click();await page.waitForFunction(()=>document.querySelector('#fc-status').textContent.includes('已连接'));
  check(await page.evaluate(()=>!__fcCommands.some(x=>x.includes(',START,')||x.includes(',ARM,'))),'连接不能准备或启动');
  await page.locator('#fc-arm').click();await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('已准备'));
  check(await page.locator('#fc-start').isDisabled(),'夹具未确认禁止开始');await page.locator('#fc-fixture').check();
  await page.locator('#fc-start').click();await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('数据冻结'));
  await page.locator('#fc-read').click();await page.waitForFunction(()=>document.querySelector('#fc-quality').textContent.includes('12 条'));
  check(await page.locator('#fc-window').inputValue()===''&&await page.locator('#fc-margin').inputValue()==='','T 和裕量初始为空');
  const downloadPromise=page.waitForEvent('download');await page.locator('#fc-json').click();const download=await downloadPromise;
  await download.saveAs('output/playwright/force-calibration-capture.json');
  const csvPromise=page.waitForEvent('download');await page.locator('#fc-csv').click();await (await csvPromise).saveAs('output/playwright/force-calibration-capture.csv');
  await page.setViewportSize({width:1440,height:1000});await page.screenshot({path:'output/playwright/force-calibration-desktop.png',fullPage:true});
  await page.setViewportSize({width:390,height:844});check(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'390px 无横向溢出');
  await page.screenshot({path:'output/playwright/force-calibration-mobile.png',fullPage:true});
  await page.locator('#fc-disconnect').click();check(await page.locator('#fc-start').isDisabled(),'断开后禁止开始');
  await page.locator('#fc-connect').click();await page.waitForFunction(()=>document.querySelector('#fc-status').textContent.includes('已连接'));
  check(await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',START,')).length===1),'重新连接不得恢复测试');
  await page.locator('#fc-arm').click();await page.locator('#fc-fixture').check();await page.locator('#fc-start').click();await page.locator('#fc-stop').click();
  await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('异常终止'));
  const starts=await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',START,')).length);
  await page.locator('#fc-import').setInputFiles('output/playwright/force-calibration-capture.json');
  check(await page.evaluate(()=>__fcCommands.filter(x=>x.includes(',START,')).length)===starts,'导入历史不得开始');
  await page.locator('#fc-arm').click();await page.evaluate(()=>{Object.defineProperty(document,'hidden',{configurable:true,value:true});document.dispatchEvent(new Event('visibilitychange'));});
  await page.waitForFunction(()=>document.querySelector('#fc-state').textContent.includes('异常终止'));
  await page.reload();check(await page.locator('#fc-start').isDisabled(),'刷新不得恢复开始');
  check(await page.evaluate(()=>__fcCommands.length===0),'刷新不得自动发送控制命令');
  for(const mode of ['disabled','old-firmware','occupied','no-serial']){
    await page.goto('http://127.0.0.1:4176/force-calibration.html?'+mode);
    if(mode!=='no-serial')await page.locator('#fc-connect').click();
    const text={disabled:'disabled','old-firmware':'未提供 FC1',occupied:'占用','no-serial':'Web Serial'}[mode];
    await page.waitForFunction(text=>document.querySelector('#fc-message').textContent.includes(text),text);
    check(await page.locator('#fc-start').isDisabled(),mode+' 禁止开始');
  }
  check(errors.length===0,errors.join('\n'));return {passed:true,errors,downloads:2};
}
