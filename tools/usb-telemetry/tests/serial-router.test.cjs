const test=require('node:test'),assert=require('node:assert/strict');
const {createRouter}=require('../serial-router.js');
const {Coordinator}=require('../control-coordinator.js');
const F=require('../force-telemetry.js');
test('force-v1 column names match the actual C firmware formatter',()=>{
  const fs=require('node:fs'),path=require('node:path');
  const source=fs.readFileSync(path.join(__dirname,'../../../modules/telemetry/chassis_force_telemetry.c'),'utf8');
  const literal=source.match(/static const char columns\[\]=([\s\S]*?);/)[1];
  const header=[...literal.matchAll(/"([^"\n]*)"/g)].map(x=>x[1]).join('').replace(/\\r\\n$/,'');
  assert.equal(F.HEADER,header);
});
test('one bounded router separates fragmented mixed protocols without consuming another prefix',()=>{
  const rows={telemetry:[],force:[],tuning:[],autotune:[],calibration:[],errors:[]};
  const router=createRouter(Object.fromEntries(Object.keys(rows).map(key=>['on'+key[0].toUpperCase()+key.slice(1),line=>rows[key].push(line)])));
  const text='version,seq,tick\r\n1,2,3\r\n@T1,1,OK,pid\r\n@FC1,1,ERR,9,busy\r\nforce-v1,tick\r\nforce-v1,200\r\n@A2,1,CAP\r\n';
  for(const byte of new TextEncoder().encode(text))router.push(Uint8Array.of(byte));
  assert.equal(rows.telemetry.length,2);assert.equal(rows.force.length,2);assert.equal(rows.calibration.length,1);assert.equal(rows.tuning.length,1);assert.equal(rows.autotune.length,1);
  router.push(new TextEncoder().encode('x'.repeat(4000)+'@FC1,4,ERR,9,busy\nabc\0@FC1,5,ERR,9,busy\n@FC1,6,ERR,9,busy\n'));
  assert.equal(rows.calibration.length,2);router.push(new TextEncoder().encode('@FC1,7,'));router.reset();router.push(new TextEncoder().encode('ERR,9,busy\n'));
  assert.equal(rows.calibration.length,2);
});
test('coordinator sends STOP before queued queries and keeps unknown ownership locked',async()=>{
  const lines=[];let release;const c=new Coordinator({send:line=>{lines.push(line);return lines.length===1?new Promise(r=>release=r):Promise.resolve();}});c.setConnected(true);
  const first=c.send('tuning','@T1,1,GET,pid\r\n');await new Promise(r=>setImmediate(r));
  const ordinary=c.send('tuning','@T1,2,GET,power\r\n').catch(e=>e);
  const stop=c.send('force','@FC1,3,STOP,9,0\r\n');release();await Promise.all([first,ordinary,stop]);
  assert.equal(lines.length,2);assert.match(lines[1],/,STOP,/);
  c.acquire('force',{generation:9});c.unknown('force');assert.throws(()=>c.acquire('pid'));
  await assert.rejects(c.send('tuning','@T1,4,SET,pid,1\r\n'));
  c.observeForce({generation:8,session:3,state:9,zero_submitted:1,final:[0,0,0,0]});assert.equal(c.owner,'force');
  c.observeForce({generation:9,session:3,state:9,zero_submitted:0,final:[0,0,0,0]});assert.equal(c.owner,'force');
  c.observeForce({generation:9,session:3,state:9,zero_submitted:1,final:[0,0,0,0]});assert.equal(c.owner,'force');
  c.activate('force',{generation:9,session:3});
  c.observeForce({generation:9,session:3,state:9,zero_submitted:1,final:[0,0,0,0]});assert.equal(c.owner,null);
});
test('force-v1 exact schema keeps raw units and masks sources that are absent or stale',()=>{
  const values=Object.fromEntries(F.FIELDS.map(k=>[k,0]));Object.assign(values,{backend:1,tick:100,cycle_seq:1,feedback_valid:1,can_submitted:1,load_cW:1234,model_allow_cW:5000,rpm_rf:-1234,feedback_age_rf:2,feedback_age_lf:2,feedback_age_lb:2,feedback_age_rb:2,omega_mrad_s:1000});
  const line=['force-v1',...F.FIELDS.slice(1).map(k=>values[k])].join(',');
  const sample=F.parseLine(line);assert.equal(sample.rawLine,line);
  const n=F.normalize(sample,0);assert.equal(n.values.rpm_rf,-1234);assert.equal(n.values.p_meter_cw,12.34);assert(Math.abs(n.values.velocity_actual_wz_rpm-60/(2*Math.PI))<1e-9);
  assert.equal(n.values.imu_yaw_mdeg,null);assert.equal(n.values.target_rf_rpm,null);assert.equal(n.valid.remote,false);
  assert.equal(F.normalize(sample,501).values.rpm_rf,null);
  const store=new F.Store();store.add(sample,0);store.add(sample,10000);
  assert.equal(store.latest.normalized.valid.chassis,false);assert.equal(store.latest.normalized.values.rpm_rf,null);
  assert.throws(()=>F.parseLine(line+',0'));assert.throws(()=>F.parseLine(line.replace(',1234,',',NaN,')));
});
