const test=require('node:test'),assert=require('node:assert/strict');
const {createServer}=require('../serve.cjs');
test('only the explicit calibration HTML route is added and local write origin stays enforced',async()=>{
  const server=createServer();await new Promise(r=>server.listen(0,'127.0.0.1',r));
  const base=`http://127.0.0.1:${server.address().port}`;
  try{
    for(const path of ['force-calibration.html','force-calibration.css','force-calibration-ui.js','force-calibration-client.js','force-calibration-protocol.js','force-calibration-report.js'])
      assert.equal((await fetch(base+'/'+path)).status,200,path);
    assert.equal((await fetch(base+'/other.html')).status,404);
    const legacy=await (await fetch(base+'/force-calibration.html')).text();
    assert.match(legacy,/#force-calibration/);assert.doesNotMatch(legacy,/serial\.js|new .*SerialSession|force-calibration-ui\.js/);
    assert.equal((await fetch(base+'/%2e%2e/AGENTS.md')).status,404);
    assert.equal((await fetch(base+'/api/tuning',{method:'POST',headers:{Origin:'https://example.com','Content-Type':'application/json'},body:'{}'})).status,403);
  }finally{await new Promise(r=>server.close(r));}
});
