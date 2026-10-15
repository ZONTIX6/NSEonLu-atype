'use strict';
const http = require('node:http');
const fs = require('node:fs/promises');
const path = require('node:path');

const types = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.svg': 'image/svg+xml' };

function createServer(root = __dirname) {
  return http.createServer(async (request, response) => {
    response.setHeader('Cache-Control', 'no-store');
    response.setHeader('X-Content-Type-Options', 'nosniff');
    response.setHeader('Permissions-Policy', 'serial=(self)');
    if (request.method !== 'GET' && request.method !== 'HEAD') {
      response.writeHead(405, { Allow: 'GET, HEAD' });
      response.end('Method not allowed');
      return;
    }
    try {
      const url = new URL(request.url, 'http://localhost');
      const asset = url.pathname === '/' ? '/index.html' : url.pathname;
      if (!/^\/(?:index\.html|[a-z][a-z0-9-]*\.(?:css|js|svg))$/.test(asset)) {
        response.writeHead(404); response.end('Not found'); return;
      }
      const content = await fs.readFile(path.join(root, asset.slice(1)));
      response.writeHead(200, { 'Content-Type': `${types[path.extname(asset)]}; charset=utf-8` });
      response.end(request.method === 'HEAD' ? undefined : content);
    } catch (error) {
      response.writeHead(error.code === 'ENOENT' ? 404 : 400);
      response.end(error.code === 'ENOENT' ? 'Not found' : 'Bad request');
    }
  });
}

if (require.main === module) {
  const port = Number(process.argv[2] || 4173);
  if (!Number.isInteger(port) || port < 1 || port > 65535) {
    console.error('端口必须是 1 到 65535 的整数。'); process.exitCode = 1;
  } else {
    const server = createServer();
    server.on('error', error => {
      console.error(error.code === 'EADDRINUSE' ? `端口 ${port} 已被占用，请使用 node serve.cjs 4174 选择其他端口。` : error.message);
      process.exitCode = 1;
    });
    server.listen(port, '127.0.0.1', () => {
      console.log(`USB 遥测仪表盘：http://127.0.0.1:${port}`);
      console.log('请用电脑端 Edge / Chrome 打开上面的地址。按 Ctrl+C 停止。');
    });
  }
}

module.exports = { createServer };
