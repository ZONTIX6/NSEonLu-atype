async (page)=>{
  const check=(ok,message)=>{if(!ok)throw new Error(message);},errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://127.0.0.1:4176');await page.getByRole('tab',{name:/自动整定/}).click();
  await page.locator('#auto-demo').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('演示完成'));
  check(await page.locator('#auto-comparisons tr').count()===3,'three fresh acceptance profiles in demo');
  check(await page.locator('#auto-adopt').isDisabled(),'synthetic results cannot be adopted');
  const jsonDownload=page.waitForEvent('download');await page.locator('#auto-json').click();await (await jsonDownload).saveAs('output/playwright/autotune-v2-report.json');
  const csvDownload=page.waitForEvent('download');await page.locator('#auto-csv').click();await (await csvDownload).saveAs('output/playwright/autotune-v2-trials.csv');
  await page.locator('#auto-wheel').selectOption('all');await page.locator('#auto-demo').click();await page.waitForFunction(()=>document.querySelector('#auto-trials').children.length>2);await page.locator('#auto-stop').click();
  await page.waitForFunction(()=>!document.querySelector('#auto-demo').disabled);check(await page.locator('#auto-adopt').isDisabled(),'cancel never enables synthetic adoption');
  await page.locator('#auto-demo').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('演示完成'));
  check(await page.locator('#auto-results tr').count()===4,'four independent synthetic wheel results');
  await page.setViewportSize({width:390,height:844});check(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'mobile overflow');await page.screenshot({path:'output/playwright/autotune-v2-mobile.png',fullPage:true});
  await page.setViewportSize({width:1440,height:1100});await page.screenshot({path:'output/playwright/autotune-v2-desktop.png',fullPage:true});
  let syncCalls=0;
  await page.route('**/api/tuning',async route=>{++syncCalls;await route.fulfill({status:syncCalls===1?500:200,contentType:'application/json',body:syncCalls===1?' {"error":"disk failure"}':'{"saved":true}'});});
  await page.addInitScript(()=>{
    window.__commands=[];window.__legacy=false;window.__late=false;window.__saveReady=false;
    window.__groups={pid:{revision:1,values:Array.from({length:4},()=>[.75,0,0,15000,3000,0]).flat(),storage:'saved'},power:{revision:1,values:Array(19).fill(1),storage:'saved'},spin:{revision:1,values:Array(16).fill(1),storage:'saved'}};
    window.__trial={session:0,state:'idle',wheel:0,trial:0,values:[0,0,0],cap:0};
    class Port{
      async open(){
        this.readable=new ReadableStream({start:c=>{this.controller=c;}});
        this.writable=new WritableStream({write:bytes=>{
          const line=new TextDecoder().decode(bytes).trim();__commands.push(line);const c=line.split(',');let reply;
          if(c[0]==='@FC1')return;
          if(window.__dropParameterWrite&&c[0]==='@T1'&&['SET','SAVE'].includes(c[2]))return;
          if(c[0]==='@T1'){
            const board=__groups[c[3]];
            if(c[2]==='SET'){board.values=c.slice(5).map(TelemetryTuning.decodeFloat);++board.revision;board.storage='pending';}
            if(c[2]==='SAVE')board.storage='pending';
            if(__saveReady&&board.storage==='pending')board.storage='saved';
            reply=['@T1',c[1],'OK',c[3],board.revision,0,...board.values.map(TelemetryTuning.encodeFloat),board.storage].join(',');
          }else if(c[2]==='CAP'){if(__legacy)return;reply=['@A2',c[1],'CAP',2,31,180000,1000,20].join(',');}
          else if(c[2]==='CONFIG')reply=['@A2',c[1],'CONFIG',c[3],1,...Array(6).fill('00000000'),514].join(',');
          else{
            if(c[2]==='BEGIN'){++__trial.session;__trial.state='ready';__trial.trial=0;__trial.values=[0,0,0];__trial.cap=0;__trial.wheel=0;}
            if(c[2]==='TRIAL'){__trial.state='running';__trial.wheel=+c[4];++__trial.trial;__trial.values=c.slice(8).map(TelemetryTuning.decodeFloat);__trial.hold=+c[6];__trial.cap=+c[7];}
            if(c[2]==='STATUS')__trial.state='done';if(c[2]==='END')__trial.state='aborted';
            const error=12+Math.abs(__trial.values[0]-1.5)*100+Math.abs(__trial.values[1]-1.5)*18,hold=__trial.hold||1000;
            reply=['@A2',c[1],'OK',__trial.session,__trial.state,__trial.wheel,__trial.trial,c[2]==='END'?'stopped':'none',hold,Math.round(error*1000),0,200,3000,100000,0,hold,hold,hold/2,2,1,4,__trial.cap,...__trial.values.map(TelemetryTuning.encodeFloat),2,100].join(',');
            if(__late&&c[2]==='STATUS'){
              const old=reply.replace(',OK,'+__trial.session+',',',OK,'+(__trial.session+1)+',');this.controller.enqueue(new TextEncoder().encode(old+'\r\n'));
            }
          }
          const data=new TextEncoder().encode(reply+'\r\n');for(let i=0;i<data.length;i+=7)this.controller.enqueue(data.slice(i,i+7));
        }});
      }
      async close(){}async setSignals(){}
    }
    Object.defineProperty(navigator,'serial',{configurable:true,value:{requestPort:async()=>new Port(),addEventListener(){}}});
  });
  await page.reload();await page.locator('#connect-button').click();await page.waitForFunction(()=>!document.querySelector('#tuning-apply').disabled);
  await page.getByRole('tab',{name:/自动整定/}).click();await page.locator('#auto-confirm').check();
  await page.evaluate(()=>{__late=true;});await page.locator('#auto-start').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('试调完成'));
  check(!await page.locator('#auto-adopt').isDisabled(),'complete V2 permits explicit adoption');
  check(await page.evaluate(()=>__commands.filter(l=>l.includes(',END,')).length===1),'END before adoption');
  check(await page.evaluate(()=>!__commands.some(l=>l.includes(',SET,'))),'temporary trials never SET');
  await page.evaluate(()=>{++__groups.power.revision;});await page.locator('#auto-adopt').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('禁止覆盖'));
  check(await page.locator('#auto-adopt').isDisabled(),'power configuration invalidates all unadopted results');
  await page.locator('#auto-start').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('试调完成'));await page.locator('#auto-adopt').click();await page.waitForFunction(()=>!document.querySelector('#auto-sync').hidden);
  check(await page.evaluate(()=>__commands.filter(l=>l.includes(',SET,')).length===1),'one confirmed SET');
  await page.locator('#auto-sync').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('文件已同步'));check(await page.evaluate(()=>__commands.filter(l=>l.includes(',SET,')).length===1),'file retry does not SET again');
  await page.evaluate(()=>{__groups.pid.storage='error';});await page.waitForFunction(()=>!document.querySelector('#auto-save').hidden);await page.locator('#auto-save').click();await page.evaluate(()=>{__saveReady=true;});await page.waitForFunction(()=>document.querySelector('#auto-persistence').textContent.includes('已保存到板端'));
  await page.locator('#auto-wheel').selectOption('all');await page.locator('#auto-start').click();
  for(let wheel=1;wheel<4;++wheel){await page.waitForFunction(()=>!document.querySelector('#auto-continue').hidden);const count=await page.evaluate(()=>__commands.filter(l=>l.includes(',BEGIN,')).length);await page.waitForTimeout(350);check(await page.evaluate(()=>__commands.filter(l=>l.includes(',BEGIN,')).length)===count,'no automatic session restart');await page.locator('#auto-continue').click();}
  await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('试调完成'));
  check(await page.locator('#auto-results tr').count()===4,'four actual sessions retained');
  await page.locator('#auto-wheel').selectOption('0');
  await page.evaluate(()=>{__groups.pid.values[0]=.75;__groups.pid.values[1]=0;++__groups.pid.revision;});
  await page.locator('#auto-start').click();
  await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('试调完成'));
  await page.evaluate(()=>{__dropParameterWrite=true;});await page.locator('#auto-adopt').click();
  await page.waitForFunction(()=>telemetryApp.coordinator.phase==='unknown');
  check(await page.locator('#tuning-apply').isDisabled(),'unknown adoption must keep other parameter controls locked');
  await page.locator('#connect-button').click();await page.waitForFunction(()=>telemetryApp.session.state==='idle');
  await page.evaluate(()=>{__dropParameterWrite=false;});await page.locator('#connect-button').click();
  await page.waitForFunction(()=>!document.querySelector('#auto-start').disabled);
  check(await page.evaluate(()=>telemetryApp.coordinator.owner===null),'reconnection clears local unknown owner without restoring a trial');
  await page.evaluate(()=>{__legacy=true;});await page.locator('#auto-start').click();await page.waitForFunction(()=>document.querySelector('#auto-status').textContent.includes('旧固件'));
  check(await page.locator('#auto-adopt').isDisabled(),'legacy cannot claim V2 acceptance');
  await page.evaluate(()=>{__legacy=false;});await page.locator('#auto-start').click();await page.waitForFunction(()=>document.querySelector('#auto-trials').children.length>0);await page.evaluate(()=>telemetryApp.session.disconnect());
  await page.waitForFunction(()=>!document.querySelector('#auto-stop').disabled===false);check(await page.locator('#auto-adopt').isDisabled(),'disconnect invalidates recommendations');
  check(errors.length===0,'browser errors: '+errors.join(';'));
  console.log('PASS: V2 demo/exports/mobile/late replies/config conflict/adoption/save retry/four sessions/legacy/disconnect');
}
