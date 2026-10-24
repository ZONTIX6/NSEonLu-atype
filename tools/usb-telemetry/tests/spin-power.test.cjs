const test = require('node:test');
const assert = require('node:assert/strict');
const P = require('../protocol.js');
test('v3 spin policy and sources survive parsing, units, staleness and CSV export', () => {
  const fields = [...P.FIELDS.slice(0,85), ...'spin_power_phase,spin_block_reasons,spin_base_power_cw,spin_buffer_request_cw,spin_buffer_grant_cw,spin_base_confirm_ms,spin_filtered_power_cw,spin_power_ceiling_cw,spin_slow_correction_cw,spin_gain_milli,spin_rise_milli,spin_fall_milli,spin_lpf_ms,spin_settle_ms,power_tuning_source,spin_tuning_source'.split(',')];
  const fixture = {...Object.fromEntries(fields.map(k => [k,0])),version:3,tick:1000,
    chassis_valid:1,chassis_tick_ms:1000,spin_power_phase:2,spin_block_reasons:4,
    spin_base_power_cw:7840,spin_buffer_grant_cw:2000,spin_slow_correction_cw:-100,
    spin_gain_milli:80000,power_tuning_source:2,spin_tuning_source:3};
  const rows=[],parser=new P.StreamParser({onSample:r=>rows.push(r)});
  parser.push(fields.join(',')+'\n'+fields.map(k=>fixture[k]).join(',')+'\n');
  assert.equal(rows.length,1);
  assert.equal(P.displayValue(rows[0],'spin_base_power_cw'),78.4);
  assert.equal(P.displayValue(rows[0],'spin_slow_correction_cw'),-1);
  assert.equal(P.displayValue(rows[0],'spin_gain_milli'),80);
  assert.equal(P.displayValue({...rows[0],chassis_valid:0},'spin_buffer_grant_cw'),null);
  assert.equal(rows[0].power_tuning_source,2);
  const store=new P.TelemetryStore();store.add(rows[0],0);
  rows.length=0;parser.push(store.exportCSV());
  assert.equal(rows.length,1);assert.equal(rows[0].spin_buffer_grant_cw,2000);
});
