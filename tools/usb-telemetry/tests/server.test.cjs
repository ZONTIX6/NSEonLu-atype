'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { createServer } = require('../serve.cjs');

test('serves dashboard assets on loopback and rejects unrelated files and write methods', async () => {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'telemetry-web-'));
  await fs.writeFile(path.join(root, 'index.html'), '<h1>仪表盘</h1>');
  await fs.writeFile(path.join(root, 'protocol.js'), 'window.version = 1;');
  await fs.writeFile(path.join(root, 'private.txt'), 'not a dashboard asset');
  const server = createServer(root);
  try {
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const base = `http://127.0.0.1:${server.address().port}`;
    const index = await fetch(base + '/');
    assert.equal(index.status, 200);
    assert.match(index.headers.get('content-type'), /text\/html.*utf-8/);
    assert.equal(await index.text(), '<h1>仪表盘</h1>');
    const script = await fetch(base + '/protocol.js');
    assert.match(script.headers.get('content-type'), /javascript/);
    assert.equal((await fetch(base + '/private.txt')).status, 404);
    assert.equal((await fetch(base + '/%2e%2e/AGENTS.md')).status, 404);
    assert.equal((await fetch(base + '/index.html', { method: 'POST' })).status, 405);
    const head = await fetch(base + '/index.html', { method: 'HEAD' });
    assert.equal(head.status, 200);
    assert.equal(await head.text(), '');
    assert.equal(index.headers.get('permissions-policy'), 'serial=(self)');
  } finally {
    await new Promise(resolve => server.close(resolve));
    await fs.rm(root, { recursive: true, force: true });
  }
});
