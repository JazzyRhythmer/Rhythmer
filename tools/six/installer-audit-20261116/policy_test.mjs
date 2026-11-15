import assert from 'node:assert/strict';import {readFileSync}from'node:fs';
import {authorize,sourceKind,DeviceUpdater}from'./stage/Rhythmer/installer/core.mjs';
import {validateOfficial,recoveryPolicy}from'./stage/Rhythmer/installer/install-safety.mjs';
import {isSupportedSource}from'./stage/Jazzy/installer/package.mjs';
let n=0;const check=(v,s)=>{assert.ok(v,s);n++;};const throws=(f)=>{assert.throws(f);n++;};
const prev='/Users/dave/Documents/Codex/FM1-Installer-V15-V16-20261112/';
const f15=new Uint8Array(readFileSync(prev+'official-v15.fwsc')),f16=new Uint8Array(readFileSync(prev+'official-v16.fwsc'));const s15=await validateOfficial(f15),s16=await validateOfficial(f16);
check(s15.product==='FM-1_015'&&s16.product==='FM-1_016','pinned official packages');
for(const [id,rec]of[['FM-1_015',s15],['FM-1_016',s16]]){
 const src={id:{text:id,model:'FM-1',version:Number(id.split('_')[1])}};src.kind=sourceKind(src.id,[]);
 check(authorize({mode:'rhythmer',source:src,recovery:rec,target:{product:'FM-1_748023'},backupSaved:true,ack:true}),'official allowed');check(isSupportedSource(id,'','jazzy'),'Jazzy official allowed');
 throws(()=>authorize({mode:'rhythmer',source:src,recovery:rec,target:{product:'FM-1_748023'},backupSaved:false,ack:true}));
}
throws(()=>recoveryPolicy({id:'FM-1_016'},s15,'jazzy'));throws(()=>recoveryPolicy({id:'FM-1_015'},s16,'stock'));
throws(()=>authorize({mode:'rhythmer',source:{kind:'loader'},recovery:s15,target:{product:'FM-1_748023',sha256:'x'},backupSaved:true,ack:true}));
throws(()=>authorize({mode:'rhythmer',source:{kind:'custom'},recovery:s15,target:{product:'FM-1_748023'},backupSaved:true,ack:true}));
const bad=f16.slice();bad[2000]^=1;await assert.rejects(()=>validateOfficial(bad));n++;
const midi={inputs:new Map([['a',{name:'JAZZY',state:'connected'}],['b',{name:'FM-1',state:'connected'}]]),outputs:new Map()};await assert.rejects(()=>new DeviceUpdater(midi).find(()=>true),e=>e.code==='ambiguous');n++;
console.log('PASS',n,'package/source/backup/multiple-device safeguards');
