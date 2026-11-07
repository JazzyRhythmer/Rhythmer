// SPDX-License-Identifier: GPL-3.0-only
import {DeviceUpdater,inspect,backup,authorize,validateRhythmer,validateStockPackage,validateLoop,validateInstallBundle,transfer,verifyRhythmer,verifyEmbeddedBanks,sha256hex,strictInfo} from './core.mjs';
import {uploadBreakBank,packBreak} from '../editor/break-audio.mjs';
import {FakeFM1,infoOf} from './fake.mjs';
const $=s=>document.querySelector(s),FAST={hs:2200,hsTries:2,findTries:1,backoff:5,settle1:10,idle1:5000,upgradeTries:1,loaderSettle:10,loader:15000,settle2:10,idle2:5000,back:15000,poll:50};
let checks=[];const ok=(value,label)=>{if(!value)throw Error(label);checks.push(label);$('#tests').textContent=checks.map((x,i)=>`${i+1}. PASS ${x}`).join('\n');};
const rejects=async(fn,label)=>{let error;try{await fn();}catch(e){error=e;}ok(!!error,label);};
const get=async url=>new Uint8Array(await(await fetch(url)).arrayBuffer());
const make=(target,opts={})=>{const dev=new FakeFM1(target.image,{after:target.product,embeddedBanks:target.embeddedBanks,...opts});return {dev,up:new DeviceUpdater(dev.access,()=>{},{times:FAST})};};
// Editor-only audio regression. The installer core has no audio-write routine.
async function uploadEditorBanks(source,loops){
 const {BackupConnection}=await import('./fm1backup.js');const c=new BackupConnection(source.input,source.output);
 try{for(let i=0;i<2;i++)await uploadBreakBank((r,o)=>c.request(r,o),i,loops[i]);}finally{c.close();}
}
$('#status').textContent='Ready. Test modules loaded.';
async function runTests(){
 $('#run').disabled=true;checks=[];$('#status').textContent='Running fake MIDI checks. Real MIDI is never requested.';
 try{
  const m=await(await fetch('manifest.json')).json(),raw=await get(m.firmware.url),target=await validateRhythmer(raw,m.firmware),loops=await Promise.all(m.loops.map(async x=>validateLoop(await get(x.url),x)));
  ok(target.product===m.firmware.product,'Canonical Rhythmer: SHA-256 + actual UFW/JLFS/flash parser');
  const bundleBytes=await get(m.bundle.url),complete=await validateInstallBundle(bundleBytes,m);
  ok(complete.recovery.product==='FM-1_015','Genuine bundled official V15: exact size/hash + actual package/app/loader layout');
  ok(complete.loops.length===2&&complete.firmware.product===m.firmware.product,'One installation bundle includes firmware, both loops and recovery');
  const damagedBundle=bundleBytes.slice();damagedBundle[150]^=1;await rejects(()=>validateInstallBundle(damagedBundle,m),'Corrupt combined installation bundle rejected before MIDI access');
  const wrongAudio=JSON.parse(new TextDecoder().decode(bundleBytes));wrongAudio.loops[0].data=wrongAudio.loops[1].data;
  const wrongBytes=new TextEncoder().encode(JSON.stringify(wrongAudio)),wrongMeta={...m,bundle:{...m.bundle,bytes:wrongBytes.length,sha256:await sha256hex(wrongBytes)}};
  await rejects(()=>validateInstallBundle(wrongBytes,wrongMeta),'Wrong audio in a valid-looking bundle rejected by slot hash');
  ok(complete.audioRights.publicRedistributionCleared===false,'Bundled audio notice does not claim redistribution clearance');

  ok(loops.length===2,'Both 12-bit two-bar factory loops: header + CRC32 + SHA-256');
  const bad=raw.slice();bad[bad.length-4]^=1;await rejects(()=>validateRhythmer(bad,m.firmware),'Reject corrupt Rhythmer before transfer');
  await rejects(()=>validateStockPackage(raw),'Reject Rhythmer masquerading as V15');
  await rejects(()=>validateStockPackage(new Uint8Array(699956)),'Reject correct-size counterfeit V15 by SHA-256');
  const badLoop=loops[0].slice();badLoop[550]^=1;await rejects(()=>validateLoop(badLoop,m.loops[0]),'Reject corrupt sample asset');
  ok(strictInfo(infoOf())?.engines===22,'Current Rhythmer INFO includes EMPTY');
  ok(strictInfo(infoOf('Rhythmer v0.1.1-beta3',21))?.isRhythmer,'Previous 21-engine Rhythmer remains recognized as an upgrade source');
  const badInfo=infoOf();badInfo[badInfo.indexOf(0)+3]=70;ok(!strictInfo(badInfo),'Reject matching name with incompatible INFO layout');
  const recovery={product:'FM-1_015'},base={mode:'rhythmer',source:{kind:'v15'},recovery,target,backupSaved:true,ack:true};
  ok(authorize(base),'Explicitly acknowledged V15 source can install');
  for(const [key,value,label] of [['recovery',null,'Missing V15 file blocks install'],['backupSaved',false,'Missing backup acknowledgement blocks install'],['ack',false,'Missing development acknowledgement blocks install'],['target',null,'Missing verified target blocks install'],['source',{kind:'custom'},'Foreign firmware blocks direct Rhythmer install']])await rejects(()=>authorize({...base,[key]:value}),label);
  await rejects(()=>authorize({...base,source:{kind:'loader'}}),'Unknown loader blocks Rhythmer resume');
  ok(authorize({...base,source:{kind:'loader'},checkpoint:{product:target.product,sha256:target.sha256}}),'Matching saved checkpoint permits explicit Rhythmer resume');
  await rejects(()=>authorize({...base,source:{kind:'loader'},checkpoint:{product:target.product,sha256:'wrong'}}),'Wrong checkpoint hash blocks resume');
  ok(authorize({...base,mode:'stock',source:{kind:'loader'},target:recovery}),'Explicit official V15 recovery permits unknown responding loader');
  let {dev,up}=make(target);let source=await inspect(up);ok(dev.upgrades===0&&source.kind==='v15','Connect/inspect sends no UPGRADE writes');
  const wrongPort={...dev.input,id:'another',name:'FM-1 2'};dev.access.inputs.set('another',wrongPort);await rejects(()=>inspect(up),'Multiple MIDI endpoints block device selection');ok(dev.upgrades===0,'Multiple devices cause no firmware write');dev.access.inputs.delete('another');
  dev.boot('FM-1_015','FM-1');await rejects(()=>transfer(up,source,target,()=>{}),'Device/port changed after review blocks first write');ok(dev.upgrades===0,'Changed device remains untouched');
  ({dev,up}=make(target));source=await inspect(up);let finds=0;const originalFind=up.find.bind(up);up.find=async(...args)=>{if(++finds===2)dev.boot('FM-1_015','FM-1');return originalFind(...args);};await rejects(()=>transfer(up,source,target,()=>{}),'Port swap at actual UPGRADE entry is blocked');ok(dev.upgrades===0,'Upgrade-entry race sends no UPGRADE');
  ({dev,up}=make(target,{info:infoOf()}));source=await inspect(up);const stages=[];await transfer(up,source,target,(s)=>stages.push(s));ok(dev.upgrades===2&&dev.bad===0&&stages.includes('write')&&stages.includes('reboot'),'Real two-stage updater serves exact logical-image bytes and reconnects');ok((await verifyRhythmer(up)).kind==='rhythmer','Post-install exact Rhythmer INFO verification');
  ok(await verifyEmbeddedBanks(await inspect(up))===3&&!(dev.sampleWrites||0),'Integrated firmware install verifies banks without separate sample writes');
  for(const mask of [0,1,2]){
   dev.mask=mask;let error;try{await verifyEmbeddedBanks(await inspect(up));}catch(e){error=e;}
   ok(error?.message.includes(`(mask ${mask})`)&&error.message.includes(mask&1?'Amen ready':'Amen not readable')&&error.message.includes(mask&2?'Think ready':'Think not readable'),`Unreadable factory banks fail closed with the actual A/B status (mask ${mask})`);
  }
  dev.mask=3;ok(!(dev.sampleWrites||0),'Failed bank checks never erase or repair samples automatically');
  dev.sampleOps=0;
  const music=Uint8Array.from({length:600},(_,i)=>i&255);dev.o.objs=[[0,music],[1,music.slice(0,64)],[40,new Uint8Array(0)]];
  const archive=await backup(await inspect(up));ok(archive.objects.length===3&&archive.objects[0].size===600,'Complete manifest backup and per-object CRCs');
  const originalEditor=dev.editorOriginal.bind(dev);let changed=false;dev.editorOriginal=(cmd,a)=>{if(cmd===66&&!changed){changed=true;music[0]^=1;}return originalEditor(cmd,a);};await rejects(async()=>backup(await inspect(up)),'Music changing during backup fails the object CRC');dev.editorOriginal=originalEditor;
  await uploadEditorBanks(await inspect(up),loops);ok(dev.mask===3&&dev.sampleOps===572,'Separate editor upload API: chunks, commit-last CRC and mask verification for A/B');
  const beforeB=dev.banks[1].slice(),custom=packBreak(Float32Array.from({length:48000},(_,i)=>.5*Math.sin(2*Math.PI*333*i/16000)),'USERA').bytes;
  const {BackupConnection}=await import('./fm1backup.js'),current=await inspect(up),channel=new BackupConnection(current.input,current.output);let writeRetries=0;
  await uploadBreakBank((r,o)=>{if(r[0]===80&&r[1].length)writeRetries+=o.retries;return channel.request(r,o);},0,custom);channel.close();
  ok(writeRetries===0&&dev.banks[0].every((v,i)=>v===custom[512+i]),'User sample uploader: actual sequential PCM12 payload, no automatic write retries');
  ok(dev.banks[1].every((v,i)=>v===beforeB[i])&&dev.mask===3,'Writing user slot A preserves bank B');
  dev.o.failSample=true;await rejects(async()=>uploadEditorBanks(await inspect(up),loops),'Sample failure is rejected, not reported as full install success');
  ({dev,up}=make(target,{unplugAfter:2}));source=await inspect(up);await rejects(()=>transfer(up,source,target,()=>{}),'Disconnect during staging stops updater');
  ({dev,up}=make(target,{after:'FM-1_016'}));source=await inspect(up);await rejects(()=>transfer(up,source,target,()=>{}),'Unexpected returned firmware identity fails completion');
  ({dev,up}=make(target));source=await inspect(up);await rejects(()=>transfer(up,source,target,()=>{}),'Right product but absent Rhythmer INFO fails completion');
  ({dev,up}=make(target,{info:infoOf()}));dev.boot('ota-'+target.product,'JIANT Update');source=await inspect(up);await transfer(up,source,target,()=>{},{resume:true});ok(dev.upgrades===1&&dev.bad===0,'Explicit loader resume uses stage two and verifies returned firmware');
  // Real official V15 bytes, simulated device only; this is not a physical restore.
  const synthetic=complete.recovery;
  ({dev,up}=make(synthetic,{identity:m.firmware.product,info:infoOf(),after:'FM-1_015'}));source=await inspect(up);await transfer(up,source,synthetic,()=>{});ok(dev.identity==='FM-1_015'&&dev.bad===0,'Official V15 logical-image transfer/reconnect on simulated MIDI (not physical restore)');
  $('#status').textContent=`PASS: ${checks.length} simulated checks. No physical FM-1 was accessed.`;
  const report={passed:checks.length,checks,physicalHardwareTested:false,officialV15PositiveFileTested:true,firmwareSHA256:await sha256hex(raw)};
  const a=$('#download');a.href=URL.createObjectURL(new Blob([JSON.stringify(report,null,2)],{type:'application/json'}));a.hidden=false;
 }catch(e){$('#status').textContent='FAIL: '+e.message;$('#tests').textContent+='\n'+e.stack;}finally{$('#run').disabled=false;}
}
$('#run').onclick=runTests;
if(new URLSearchParams(location.search).has('autorun'))runTests();
