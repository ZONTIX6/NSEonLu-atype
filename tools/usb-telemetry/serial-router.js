(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.TelemetrySerialRouter=api;})(globalThis,function(){
  'use strict';
  function createRouter({onTelemetry=()=>{},onForce=()=>{},onTuning=()=>{},onAutotune=()=>{},onCalibration=()=>{},onErrors=()=>{},maxLine=3072}={}){
    let line='',dropping=false;
    function deliver(value){
      if(value.startsWith('@FC1,'))onCalibration(value);
      else if(value.startsWith('@T1,'))onTuning(value);
      else if(/^@A[12],/.test(value))onAutotune(value);
      else if(value.startsWith('force-v1,'))onForce(value);
      else if(value.startsWith('version,')||/^\d+,/.test(value))onTelemetry(value);
    }
    return {reset(){line='';dropping=false;},push(bytes){
      for(const b of typeof bytes==='string'?new TextEncoder().encode(bytes):bytes){
        if(b===10){if(!dropping)deliver(line.replace(/\r$/,''));line='';dropping=false;}
        else if(!dropping&&(b===0||b>127||line.length>=maxLine-1)){onErrors('损坏或超长串口行');line='';dropping=true;}
        else if(!dropping)line+=String.fromCharCode(b);
      }
    }};
  }
  return {createRouter};
});
