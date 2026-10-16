const test = require('node:test');
const assert = require('node:assert/strict');
const {encodeFloat, decodeFloat, TuningClient, groups} = require('../tuning.js');

test('wire floats preserve float32 and reject nonfinite or malformed values', () => {
  assert.equal(encodeFloat(.75), '3f400000');
  assert.equal(decodeFloat('bf800000'), -1);
  for (const value of [NaN, Infinity, 1e50]) assert.throws(() => encodeFloat(value));
  for (const value of ['7fc00000', 'zzzzzzzz', '3f80']) assert.throws(() => decodeFloat(value));
  assert.equal(groups.pid.fields.length, 24);
  assert.equal(groups.power.fields.length, 19);
  assert.equal(groups.spin.fields.length, 16);
});
test('only matching complete responses resolve, and SET uses acknowledged revision', async () => {
  const writes = [], client = new TuningClient({send: async line => writes.push(line), timeoutMs: 100});
  const get = client.get('spin');
  assert.match(writes[0], /^@T1,1,GET,spin\r\n$/);
  const values = Array(16).fill(1).map(encodeFloat).join(',');
  client.receive('@T1,99,OK,spin,1,2,' + values);
  client.receive('@T1,1,OK,pid,1,2,' + values);
  client.receive('@T1,1,OK,spin,1,2,' + values);
  const result = await get;
  assert.equal(result.mode, 2);
  const set = client.set('spin', result.revision, result.values);
  assert.match(writes[1], /^@T1,2,SET,spin,1,/);
  client.receive('@T1,2,ERR,spin,conflict');
  await assert.rejects(set, /版本|重新读取/);
});
test('timeout and disconnect reject pending requests and allow retry', async () => {
  const client = new TuningClient({send: async () => {}, timeoutMs: 10});
  await assert.rejects(client.get('pid'), /超时/);
  const pending = client.get('pid');
  client.disconnect();
  await assert.rejects(pending, /断开/);
});
