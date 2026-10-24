const test=require('node:test');
const assert=require('node:assert/strict');
const P=require('../protocol.js');
const extra='spin_soft_wz_milli,spin_hard_wz_milli,spin_effective_wz_milli,spin_body_hard_mrps,spin_wheel_hard_rpm,spin_limit_state,spin_extension_enabled,spin_extension_qualified,spin_extension_confirm_ms,spin_power_gap_cw,spin_max_tracking_error_rpm,spin_body_tracking_error_mrps,spin_capability_latched,spin_capability_release_ms,spin_peak_wheel_rpm,spin_peak_body_mrps'.split(',');
test('v5 appends limit diagnostics to the unchanged v4 prefix',()=>{
  assert.deepEqual(P.FIELDS.slice(111),extra);
  assert.equal(P.V4_FIELDS.length,111);
  assert.deepEqual(P.FIELDS.slice(0,111),P.V4_FIELDS);
  const r={...Object.fromEntries(P.FIELDS.map(k=>[k,0])),version:5,tick:1000,
    chassis_valid:1,chassis_tick_ms:1000,spin_limit_state:2,spin_block_reasons:16384,
    spin_extension_enabled:1,spin_extension_qualified:1,spin_soft_wz_milli:8000000,
    spin_hard_wz_milli:10000000,spin_body_hard_mrps:1500,spin_power_gap_cw:2345};
  const rows=[],p=new P.StreamParser({onSample:r=>rows.push(r)});
  p.push(P.HEADER+'\n'+P.FIELDS.map(k=>r[k]).join(',')+'\n');
  assert.equal(rows.length,1);assert.equal(P.displayValue(rows[0],'spin_body_hard_mrps'),1.5);
  assert.equal(P.displayValue(rows[0],'spin_power_gap_cw'),23.45);
  const store=new P.TelemetryStore();store.add(rows[0],0);rows.length=0;
  p.push(store.exportCSV());assert.equal(rows[0].spin_block_reasons,16384);
  for(const [key,value] of [['spin_block_reasons',32768],['spin_limit_state',5],['spin_extension_qualified',2]]) {
    rows.length=0;p.push(P.FIELDS.map(k=>k===key?value:r[k]).join(',')+'\n');assert.equal(rows.length,0);
  }
});
test('old schemas remain absent through display and export, including mixed captures',()=>{
  const store=new P.TelemetryStore();
  for(const version of [1,2,3,4]) {
    const fields=P['V'+version+'_FIELDS'];const rows=[],p=new P.StreamParser({onSample:r=>rows.push(r)});
    p.push(fields.join(',')+'\n'+fields.map(k=>k==='version'?version:0).join(',')+'\n');
    assert.equal(rows.length,1);assert.equal(P.displayValue(rows[0],'spin_limit_state'),null);
    store.add(rows[0],version);
  }
  const rows=[],p=new P.StreamParser({onSample:r=>rows.push(r)});p.push(store.exportCSV());
  assert.deepEqual(rows.map(r=>r.version),[1,2,3,4]);
  assert.ok(rows.every(r=>r.spin_limit_state===undefined));
});
