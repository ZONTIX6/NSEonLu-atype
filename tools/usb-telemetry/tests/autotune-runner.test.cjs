const {test}=require('node:test');const assert=require('node:assert/strict');
test('hardware runner returns terminal failure evidence and keeps polling under the lease',async()=>{
  const R=require('../autotune-runner.js');let time=0,polls=0;
  const result=await R.runTrial({trial:async()=>({state:'running'}),status:async()=>++polls===3?{state:'aborted',reason:'feedback'}:{state:'running'}},7,[.75,0,0],{wheel:0,rpm:500,hold:1000,cap:3000},{now:()=>time,sleep:async ms=>{assert(ms<1000);time+=ms;}});
  assert.equal(result.reason,'feedback');assert.equal(polls,3);
});
