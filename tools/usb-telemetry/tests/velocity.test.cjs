const test = require('node:test');
const assert = require('node:assert/strict');
const P = require('../protocol.js');
const extra = 'velocity_actual_valid,velocity_actual_tick_ms,velocity_actual_vx_mmps,velocity_actual_vy_mmps,velocity_actual_wz_mrps,velocity_target_valid,velocity_target_tick_ms,velocity_target_vx_mmps,velocity_target_vy_mmps,velocity_target_wz_mrps'.split(',');
const fields = [...P.FIELDS.slice(0,101), ...extra];
const fixture = () => ({...Object.fromEntries(fields.map(k=>[k,0])),version:4,tick:1000,
  velocity_actual_valid:1,velocity_actual_tick_ms:995,velocity_actual_vx_mmps:1234,velocity_actual_vy_mmps:-567,velocity_actual_wz_mrps:-1250,velocity_target_valid:1,velocity_target_tick_ms:1000,velocity_target_vx_mmps:1500,velocity_target_vy_mmps:-600,velocity_target_wz_mrps:-1500});
test('physical chassis velocity parses with independent validity, signed units and export',()=>{
  const rows=[],errors=[],parser=new P.StreamParser({onSample:r=>rows.push(r),onError:e=>errors.push(e)});
  const csv=fields.join(',')+'\r\n'+fields.map(k=>fixture()[k]).join(',')+'\r\n';
  for(let i=0;i<csv.length;i+=7) parser.push(csv.slice(i,i+7));
  assert.deepEqual(errors,[]);assert.equal(rows.length,1);
  const n=P.normalizeSample(rows[0]);assert.equal(n.valid.velocityActual,true);
  assert.equal(n.values.velocity_actual_vx_mmps,1.234);
  assert.equal(n.values.velocity_actual_vy_mmps,-.567);
  assert.equal(n.values.velocity_actual_wz_mrps,-1.25);
  assert.equal(n.values.velocity_target_wz_mrps,-1.5);
  assert.equal(P.displayValue({...rows[0],velocity_actual_valid:0},'velocity_target_vx_mmps'),1.5);
  assert.equal(P.displayValue({...rows[0],velocity_target_tick_ms:979},'velocity_target_vx_mmps'),null);
  const store=new P.TelemetryStore();store.add(rows[0],0);rows.length=0;
  parser.push(store.exportCSV());assert.equal(rows.length,1);assert.equal(rows[0].velocity_actual_wz_mrps,-1250);
});
test('invalid or stale wheel estimate is blank; age handles wraparound',()=>{
  for(const patch of [{velocity_actual_valid:0},{velocity_actual_tick_ms:979},{velocity_actual_tick_ms:1001}])
    assert.equal(P.displayValue({...fixture(),...patch},'velocity_actual_vx_mmps'),null);
  assert.equal(P.displayValue({...fixture(),velocity_actual_tick_ms:980},'velocity_actual_vx_mmps'),1.234);
  assert.equal(P.displayValue({...fixture(),tick:3,velocity_actual_tick_ms:4294967294},'velocity_actual_wz_mrps'),-1.25);
});
test('v1-v3 preserve schema on export without inventing velocity',()=>{
  for(const version of [1,2,3]) {
    const old=fields.slice(0,{1:72,2:85,3:101}[version]);
    const rows=[],parser=new P.StreamParser({onSample:r=>rows.push(r)});
    parser.push(old.join(',')+'\n'+old.map(k=>k==='version'?version:0).join(',')+'\n');
    assert.equal(rows.length,1);assert.equal(P.displayValue(rows[0],'velocity_actual_vx_mmps'),null);
    const store=new P.TelemetryStore();store.add(rows[0],0);rows.length=0;
    parser.push(store.exportCSV());assert.equal(rows.length,1);assert.equal(rows[0].version,version);
    assert.equal(rows[0].velocity_actual_valid,undefined);assert.equal(P.displayValue(rows[0],'velocity_actual_wz_mrps'),null);
  }
});

test('body speed and body limit diagnostics expose rpm while raw CSV keeps legacy units',()=>{
  const raw={...fixture(),chassis_valid:1,chassis_tick_ms:1000,
    spin_body_hard_mrps:2000,spin_body_tracking_error_mrps:50,spin_peak_body_mrps:2100};
  const n=P.normalizeSample(raw);
  assert.equal(n.values.velocity_actual_wz_rpm,-75);
  assert.equal(n.values.velocity_target_wz_rpm,-90);
  assert.equal(n.values.spin_body_hard_rpm,120);
  assert.equal(n.values.spin_body_tracking_error_rpm,3);
  assert.equal(n.values.spin_peak_body_rpm,126);
  assert.equal(raw.velocity_actual_wz_mrps,-1250);
  assert.equal(n.values.velocity_actual_wz_mrps,-1.25);
  assert.equal(P.displayValue({...raw,velocity_actual_valid:0},'velocity_actual_wz_rpm'),null);
  assert.equal(P.displayValue({...raw,velocity_actual_tick_ms:979},'velocity_actual_wz_rpm'),null);
  assert.equal(P.displayValue({...raw,chassis_valid:0},'spin_body_hard_rpm'),null);
  assert.equal(P.displayValue({version:3,tick:1000},'velocity_actual_wz_rpm'),null);
});
