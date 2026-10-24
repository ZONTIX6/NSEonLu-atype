const test = require('node:test');
const assert = require('node:assert/strict');
const P = require('../protocol.js');
const legacyFields = P.FIELDS.slice(0, 72);
const imuFields = 'imu_valid,imu_age_ms,imu_yaw_mdeg,imu_pitch_mdeg,imu_roll_mdeg,imu_yaw_total_mdeg,imu_gx_mdps,imu_gy_mdps,imu_gz_mdps,imu_ax_mmps2,imu_ay_mmps2,imu_az_mmps2,chassis_offset_mdeg'.split(',');
const fields = [...legacyFields, ...imuFields];
const fixture = (overrides = {}) => ({...Object.fromEntries(fields.map(key => [key, 0])), version: 2, seq: 1, tick: 1000, chassis_valid: 1, chassis_tick_ms: 1000, imu_valid: 1, imu_age_ms: 3, imu_yaw_mdeg: -12345, imu_pitch_mdeg: 1250, imu_roll_mdeg: -2500, imu_yaw_total_mdeg: 721234, imu_gx_mdps: -180500, imu_gy_mdps: 250, imu_gz_mdps: 90000, imu_ax_mmps2: -1500, imu_ay_mmps2: 2000, imu_az_mmps2: 9807, chassis_offset_mdeg: -30500, ...overrides});
const csv = (keys, row) => keys.map(key => row[key]).join(',');

test('v2 IMU survives byte fragments and displays signed physical units', () => {
  const rows = [], parser = new P.StreamParser({onSample: row => rows.push(row)});
  const text = fields.join(',') + '\r\n' + csv(fields, fixture()) + '\r\n';
  for (let i = 0; i < text.length; i += 7) parser.push(new TextEncoder().encode(text.slice(i, i + 7)));
  assert.equal(rows.length, 1);
  const display = P.normalizeSample(rows[0]);
  assert.equal(display.valid.imu, true);
  for (const [key, expected] of Object.entries({imu_yaw_mdeg: -12.345, imu_pitch_mdeg: 1.25, imu_roll_mdeg: -2.5, imu_yaw_total_mdeg: 721.234, imu_gx_mdps: -180.5, imu_gy_mdps: .25, imu_gz_mdps: 90, imu_ax_mmps2: -1.5, imu_ay_mmps2: 2, imu_az_mmps2: 9.807, chassis_offset_mdeg: -30.5})) assert.equal(display.values[key], expected, key);
});

test('IMU invalid and older than 10 ms is masked independently of chassis', () => {
  assert.equal(P.normalizeSample(fixture({imu_age_ms: 10})).valid.imu, true);
  for (const overrides of [{imu_valid: 0}, {imu_age_ms: 11}, {imu_age_ms: 4294967295}]) {
    const display = P.normalizeSample(fixture(overrides));
    assert.equal(display.valid.imu, false);
    assert.equal(display.values.imu_yaw_mdeg, null);
    assert.equal(display.values.imu_az_mmps2, null);
    assert.equal(display.values.chassis_offset_mdeg, -30.5);
  }
  assert.equal(P.normalizeSample(fixture({chassis_valid: 0})).values.chassis_offset_mdeg, null);
});

test('v1 remains readable and unavailable IMU does not become zero measurements', () => {
  const rows = [], parser = new P.StreamParser({onSample: row => rows.push(row)});
  parser.push(legacyFields.join(',') + '\n' + csv(legacyFields, fixture({version: 1, vx_cmd_milli: -1250})) + '\n');
  assert.equal(rows.length, 1);
  const display = P.normalizeSample(rows[0]);
  assert.equal(display.valid.imu, false);
  assert.equal(display.values.imu_yaw_mdeg, null);
  assert.equal(display.values.chassis_offset_mdeg, null);
  assert.equal(display.values.vx_cmd_milli, -1.25);
});

test('rejects version/header mismatch and malformed IMU flag without breaking control replies', () => {
  const rows = [], controls = [], parser = new P.StreamParser({onSample: row => rows.push(row), onControl: line => controls.push(line)});
  parser.push(fields.join(',') + '\n' + csv(fields, fixture({version: 1})) + '\n' + csv(fields, fixture({imu_valid: 2})) + '\n@T1,1,ERR,pid,busy\n' + csv(fields, fixture()) + '\n');
  assert.equal(rows.length, 1);
  assert.deepEqual(controls, ['@T1,1,ERR,pid,busy']);
});

test('mixed legacy and IMU records export as consistent v3 CSV and reparse', () => {
  const store = new P.TelemetryStore();
  const legacy = Object.fromEntries(legacyFields.map(key => [key, fixture({version: 1})[key]]));
  store.add({...legacy, rawLine: csv(legacyFields, legacy)}, 0);
  store.add(fixture({seq: 2, tick: 1020}), 20);
  const exported = store.exportCSV(), rows = [], errors = [];
  const parser = new P.StreamParser({onSample: row => rows.push(row), onError: error => errors.push(error)});
  parser.push(exported);
  assert.deepEqual(errors, []);
  assert.equal(rows.length, 2);
  assert.equal(rows[0].version, 4);
  assert.equal(rows[0].imu_valid, 0);
  assert.equal(rows[0].imu_age_ms, 4294967295);
  assert.equal(P.displayValue(rows[0], 'chassis_offset_mdeg'), null);
  assert.equal(P.displayValue(rows[1], 'imu_yaw_mdeg'), -12.345);
});
