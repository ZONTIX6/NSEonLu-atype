const test=require('node:test');
const assert=require('node:assert/strict');
const P=require('../force-calibration-protocol.js');
const R=require('../force-calibration-report.js');
const {Client}=require('../force-calibration-client.js');
const encoder=new TextEncoder();
const caps='@FC1,1,CAP,9,1,1,48,1280,1500,500,1000,50,20,100,150,50,100,0,3,1,0,1,4,2,3,850,900\r\n';
function status(id=2,session=3,state=2,records=0) {return `@FC1,${id},OK,9,${session},4,3,9,${state},0,${records},1,1,0,0,0,0,0,-123,12500,30,1000,2000,50000\r\n`;}
test('independent little endian vector and known CRC16',()=>{
  assert.equal(P.crc16('123456789'),0x4b37);
  const bytes=Uint8Array.from([2,1,3,0,4,3,2,1,4,3,2,1,24,252,255,255,133,255,212,254,0,0,72,65,44,1,1,0,2,0,253,255,252,255,30,15,208,7,0,0,1,0,2,0,0,0,2,2]);
  const r=P.decodeRecord(bytes);assert.equal(r.raw_rpm,-123);assert.equal(r.raw_current,-300);
  assert.equal(r.used_rpm,12.5);assert.equal(r.time_us,0x01020304);assert.equal(r.feedback_time_us,-1000);
  assert.deepEqual(r.final,[1,2,-3,-4]);assert.equal(r.cycle_us,2000);
  assert.throws(()=>P.decodeRecord(bytes.subarray(1)));
  bytes[0]=9;assert.throws(()=>P.decodeRecord(bytes));
});
test('fragmented strict replies reject overflow, extra fields and checksum damage',()=>{
  const replies=[],errors=[];const parser=P.createParser({onReply:x=>replies.push(x),onError:e=>errors.push(e)});
  for(const byte of encoder.encode(caps+status())) parser.push(Uint8Array.of(byte));
  assert.equal(replies.length,2);assert.equal(replies[0].rf_id,4);assert.equal(replies[1].used_rpm,12.5);
  parser.push(encoder.encode(caps.replace(',1280,',',4294967296,')+caps.replace('\r\n',',0\r\n')));
  assert.equal(replies.length,2);assert.equal(errors.length,2);
  parser.push(encoder.encode('@FC1,7,DATA,9,3,4,0,1,1,00,0000\r\n'));assert.equal(replies.length,2);
});
test('fixed DATA CRC validates without generating its expectation from the parser',()=>{
  const line='@FC1,7,DATA,9,3,4,0,1,1,'+'00'.repeat(48)+',f45b';
  assert.equal(P.parseLine(line).bytes.length,48);
  assert.throws(()=>P.parseLine(line.replace(',0,1,1,',',1,1,1,')),/CRC/);
});
const records=[{type:'CONTROL',time_us:0,used_rpm:100,cycle_seq:1,wheel:1},
  {type:'RX',time_us:0,raw_rpm:100,feedback_seq:1,wheel:1},
  {type:'RX',time_us:5000,raw_rpm:140,feedback_seq:2,wheel:1},
  {type:'RX',time_us:10000,raw_rpm:180,feedback_seq:3,wheel:1}];
test('growth uses board time, absolute speed and complete gap-free windows',()=>{
  let r=R.analyzeGrowth({records,window_us:10000,margin_rpm:20,max_gap_us:5000});
  assert.equal(r.valid,true);assert.equal(r.max_growth_rpm,80);assert.equal(r.candidate_rpm,100);
  const negative=records.map(r=>({...r,raw_rpm:-r.raw_rpm,used_rpm:-r.used_rpm}));
  assert.equal(R.analyzeGrowth({records:negative,window_us:10000,margin_rpm:20,max_gap_us:5000}).candidate_rpm,100);
  for(const broken of [records.slice(0,3),records.filter((_,i)=>i!==2),[records[0],records[2],records[1],records[3]],
    records.map((r,i)=>i===3?{...r,feedback_seq:2}:r)])
    assert.equal(R.analyzeGrowth({records:broken,window_us:10000,margin_rpm:20,max_gap_us:5000}).valid,false);
});
test('torque, equivalent command current and report retain unknown values',()=>{
  const [row]=R.torqueRows({measurements:[{mass_kg:.5,arm_m:.1,current:8192,direction:'正向'}]});
  assert(Math.abs(row.torque_nm-.4905)<1e-12);assert.equal(row.command_a,10);
  const report={records,caps:{format:1},quality:{complete:false},pulse:null,source_commit:null,voltage_v:null};
  const json=JSON.parse(R.toJson(report));assert.equal(json.voltage_v,null);assert.equal(json.quality.complete,false);
  assert.match(R.toCsv(report),/time_us/);assert.match(R.toCsv(report),/CONTROL/);
});
class SerialFake {
  constructor(){this.state='connected';this.lines=[];this.generation=9;this.auto=true;}
  async send(line) {
    this.lines.push(line);if(!this.auto)return;
    const [,id,op]=line.trim().split(',');
    let reply=op==='CAP'?caps.replace(',1,CAP,',`,${id},CAP,`):status(Number(id),3,op==='STOP'?9:2);
    queueMicrotask(()=>this.onChunk(encoder.encode(reply)));
  }
  async disconnect(){this.state='idle';this.onState?.('idle');}
}
test('connecting only queries, explicit arm leases, STOP preempts and disconnect cancels',async()=>{
  const serial=new SerialFake(),errors=[];const c=new Client({serialSession:serial,onError:e=>errors.push(e),timeout_ms:35});
  await c.capabilities();assert.equal(serial.lines.length,1);assert.match(serial.lines[0],/,CAP/);
  await c.arm();await new Promise(r=>setTimeout(r,62));assert(serial.lines.some(x=>x.includes(',PING,')));
  serial.auto=false;const one=c.status().catch(e=>e),two=c.status().catch(e=>e);
  await new Promise(r=>setTimeout(r,1));const stop=c.stop().catch(e=>e);
  await Promise.all([one,two,stop]);assert.match(serial.lines.at(-1),/,STOP,/);
  assert.equal(serial.lines.filter(x=>x.includes(',STATUS,')).length,1);
  await c.disconnect();const count=serial.lines.length;await new Promise(r=>setTimeout(r,60));assert.equal(serial.lines.length,count);
});
test('START timeout never retries or accepts an old session',async()=>{
  const serial=new SerialFake();const c=new Client({serialSession:serial,timeout_ms:20});
  await c.capabilities();await c.arm();serial.auto=false;
  const start=c.start({wheel:1,current:300,rise_ms:50,hold_ms:100,fall_ms:50});
  await assert.rejects(start,/未确认/);assert.equal(serial.lines.filter(x=>x.includes(',START,')).length,1);
  await c.disconnect();
});
