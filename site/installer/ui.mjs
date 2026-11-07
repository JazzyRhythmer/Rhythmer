// SPDX-License-Identifier: GPL-3.0-only
import {DeviceUpdater,inspect,backup,authorize,validateStockPackage,validateInstallBundle,transfer,verifyRhythmer,verifyEmbeddedBanks,confirmSource} from './core.mjs';
import {mountProgress} from './progress.mjs';
const $=s=>document.querySelector(s),mode=new URLSearchParams(location.search).get('mode')==='stock'?'stock':'rhythmer';
let manifest,target,recovery,source,up,access,working=false,writing=false,backupReady=false,bundleReady=false,releaseLock,heartbeat,channel;
document.body.dataset.mode=mode;
const transferView=mountProgress($('#transfer-status'));let lastTransferStage=null;
const checkpointKey='rhythmer-install-checkpoint-v1';
const history=[];let reportURL,backupURL,inspectedDevice=null;
const state=text=>{$('#result').textContent=text;log(text);};
function log(text){history.push({time:new Date().toISOString(),text:String(text)});$('#log').textContent=history.map(x=>x.time.slice(11,19)+' '+x.text).join('\n');$('#log').scrollTop=$('#log').scrollHeight;updateReport();}
function updateReport(){if(reportURL)URL.revokeObjectURL(reportURL);reportURL=URL.createObjectURL(new Blob([JSON.stringify({mode,physicalHardwareTested:false,source:inspectedDevice,target:target?{product:target.product,sha256:target.sha256}:null,audioPublicRedistributionCleared:false,progress:transferView.model.snapshot(),events:history},null,2)],{type:'application/json'}));$('#report').href=reportURL;}
function checkpoint(){try{return JSON.parse(sessionStorage.getItem(checkpointKey));}catch{return null;}}
function allowed(preparing=false){try{return bundleReady&&authorize({mode,source,recovery,target,backupSaved:preparing||(backupReady&&$('#backup-saved').checked),ack:preparing||$('#ack').checked,checkpoint:checkpoint()});}catch{return false;}}
function refresh(){
 $('#start').disabled=working||!bundleReady;$('#connect').disabled=working||!!source;$('#disconnect').disabled=working||!source;
 $('#backup').disabled=working||!source||!['custom','rhythmer'].includes(source.kind);
 $('#backup-saved').disabled=working||!backupReady;$('#stock-file').disabled=working;
 $('#review').disabled=working||!allowed(true);
 $('#ack').disabled=working;
}
async function busy(fn){if(working)return false;working=true;refresh();try{await fn();return true;}catch(e){if(['running','waiting'].includes(transferView.model.status))transferView.fail(e.message);state('Stopped: '+e.message+(writing?' Firmware or audio may already be written. Inspect before recovery.':' No firmware or sample write was started by this operation.'));return false;}finally{working=false;writing=false;refresh();}}
async function loadBytes(url){const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw Error('Local asset unavailable: '+url);return new Uint8Array(await r.arrayBuffer());}
async function preload(){
 bundleReady=false;refresh();
 const r=await fetch('manifest.json',{cache:'no-store'});if(!r.ok)throw Error('Installer manifest unavailable.');manifest=await r.json();
 if(typeof manifest.localOnly!=='boolean'||!['local-development','public-beta'].includes(manifest.channel||'local-development')||manifest.physicalHardwareTested!==false)throw Error('Unsupported installer channel.');
 if(mode==='rhythmer'&&manifest.releaseHold){$('#build-status').textContent='ON HOLD / '+manifest.releaseHold;throw Error(manifest.releaseHold);}
 const b=await validateInstallBundle(await loadBytes(manifest.bundle.url),manifest);
 recovery=b.recovery;target=mode==='stock'?recovery:b.firmware;bundleReady=true;
 $('#stock-status').textContent='READY / Official V15: exact size, SHA-256 and package layout verified automatically.';
 $('#build-status').textContent=`READY / ${manifest.version}${manifest.buildRevision?' · build '+manifest.buildRevision:''} · Rhythmer + Amen/Think + official V15 recovery verified. Rhythmer SHA-256: ${b.firmware.sha256}`;
 $('#recovery-download').href=manifest.recoveryArchive.url;$('#bundle-download').href=manifest.archive.url;
 log('Installation bundle verified, including genuine official V15 and both loop CRCs/hashes.');refresh();
}
function reserve(){
 if(!navigator.locks)throw Error('Web Locks unavailable. Use current desktop Chrome or Edge.');
 return new Promise((resolve,reject)=>{navigator.locks.request('rhythmer-device-operation',{ifAvailable:true},async lock=>{
  if(!lock){reject(Error('Another Rhythmer installer owns the device. Disconnect it first.'));return;}
  await new Promise(release=>{releaseLock=release;resolve();});
 }).catch(reject);});
}
function signal(busy){channel?.postMessage({type:'installer',busy});}
function closeConnection(){
 if(heartbeat)clearInterval(heartbeat);heartbeat=null;signal(false);channel?.close();channel=null;
 for(const p of [...(access?.inputs.values()||[]),...(access?.outputs.values()||[])]){p.onmidimessage=null;p.close?.().catch(()=>{});}
 releaseLock?.();releaseLock=null;source=null;access=null;up=null;backupReady=false;$('#backup-saved').checked=false;$('#backup-download').hidden=true;
}
async function connectDevice(){
 if(transferView.model.status==='running')transferView.enter('device','Requesting MIDI access and checking the device identity.');
 if(!navigator.requestMIDIAccess)throw Error('Web MIDI unavailable. Use Chrome or Edge on a computer.');
 if(!isSecureContext)throw Error('Use localhost or HTTPS.');
 await reserve();
 try{
  channel=new BroadcastChannel('jiant-midi');signal(true);heartbeat=setInterval(()=>signal(true),4000);
  access=await navigator.requestMIDIAccess({sysex:true});
  up=new DeviceUpdater(access,log,{refresh:async()=>{access=await navigator.requestMIDIAccess({sysex:true});return access;}});
  source=await inspect(up);inspectedDevice={identity:source.id.text,kind:source.kind,port:source.input.name};$('#device-status').textContent=`${source.id.text} / ${source.kind.toUpperCase()} / ${source.input.name}`;
  if(source.kind==='unsupported')throw Error('This is not a compatible FM-1 identity.');
  $('#backup-saved').checked=false;$('#backup-download').hidden=true;backupReady=['stock','v15','loader'].includes(source.kind);
  $('#backup-label').textContent=backupReady?'This stock firmware or update loader cannot export a musical archive here. I have kept any earlier backups and understand data may be lost.':'I downloaded and saved the CRC-checked archive outside this browser, and kept my original Break audio.';
  $('#backup-status').textContent=backupReady?'Stock/loader has no compatible backup export. This limitation is included in the final review.':'Ready for automatic musical backup before installation.';
  state('Device identified. No firmware or sample writes have started.');
 }catch(e){closeConnection();throw e;}
}
async function captureMusic(){
 backupReady=false;$('#backup-saved').checked=false;$('#backup-download').hidden=true;
 await confirmSource(up,source);
 if(transferView.model.status==='running')transferView.enter('backup','Reading the complete musical archive and checking its CRCs.');
 const archive=await backup(source,(n,total)=>{if(transferView.model.status==='running')transferView.bytes('backup',n,total,'Reading and CRC-checking the musical archive.');$('#backup-status').textContent=`Reading archive: ${n.toLocaleString()} / ${total.toLocaleString()} bytes`;});
 if(backupURL)URL.revokeObjectURL(backupURL);backupURL=URL.createObjectURL(new Blob([JSON.stringify(archive,null,2)],{type:'application/json'}));
 const a=$('#backup-download');a.href=backupURL;a.download=`${source.kind}-before-${mode}-${new Date().toISOString().slice(0,10)}.json`;a.hidden=false;
 backupReady=true;$('#backup-status').textContent=`${archive.objects.length} objects read; all CRCs verified. Save the archive in the final review.`;log('Complete musical archive ready for download; final review requires acknowledgement of saving it.');
}
$('#connect').onclick=()=>busy(connectDevice);
$('#disconnect').onclick=()=>{if(working)return;closeConnection();$('#device-status').textContent='Not connected.';state('Disconnected.');refresh();};
$('#stock-file').onchange=()=>busy(async()=>{
 recovery=null;bundleReady=false;if(mode==='stock')target=null;$('#stock-status').textContent='Checking supplied official V15…';
 try{const f=$('#stock-file').files[0];if(!f)throw Error('Select the official V15 file.');recovery=await validateStockPackage(new Uint8Array(await f.arrayBuffer()));if(mode==='stock')target=recovery;bundleReady=!!recovery&&!!target;$('#stock-status').textContent='Verified official V15 override: FM-1_015.';state('Supplied V15 file verified.');}catch(e){$('#stock-status').textContent=e.message;throw e;}
});
$('#backup').onclick=()=>busy(captureMusic);
for(const id of ['ack','backup-saved'])$('#'+id).onchange=refresh;
const modalReady=()=>$('#confirm-risk').checked&&$('#confirm-backup').checked&&$('#confirm-notice').checked&&($('#confirm-audio-row').hidden||$('#confirm-audio-rights').checked)&&($('#confirm-v15-row').hidden||$('#confirm-v15').checked);
for(const id of ['confirm-risk','confirm-backup','confirm-notice','confirm-audio-rights','confirm-v15'])$('#'+id).onchange=()=>{$('#confirm-write').disabled=!modalReady();};
$('#confirm form').addEventListener('submit',e=>{if(e.submitter?.value==='write'&&!modalReady())e.preventDefault();});
async function review(){
 if(!allowed(true)||!backupReady)return;const expectedSource=source,expectedTarget=target;
 const isResume=source.kind==='loader';$('#confirm-title').textContent=isResume?'Resume this update?':mode==='stock'?'Return this FM-1 to V15?':'Install Rhythmer?';
 $('#confirm-detail').textContent=`Source: ${source.id.text} (${source.input.name}). Target: ${target.product}. SHA-256: ${target.sha256}. ${mode==='stock'?'The unmodified official V15 image will be written. Custom projects are not imported into stock.':'This integrated package includes Amen (A) and the corrected two-bar Think drum edit (B). It replaces factory audio in A/B during the firmware transfer.'} ${isResume?'Explicit update-loader resume.':''}`;
 $('#confirm-risk').checked=false;$('#confirm-backup').checked=false;$('#confirm-notice').checked=false;$('#confirm-audio-rights').checked=false;$('#confirm-audio-row').hidden=mode==='stock';$('#confirm-v15').checked=false;$('#confirm-v15-row').hidden=mode==='stock';$('#confirm-write').disabled=true;
 const archive=$('#confirm-download');archive.hidden=!backupURL||!['custom','rhythmer'].includes(source.kind);if(!archive.hidden){archive.href=backupURL;archive.download=$('#backup-download').download;}
 $('#confirm-backup-label').textContent=archive.hidden?'This stock firmware or loader has no compatible archive export. I have kept earlier backups and understand existing music/data may be lost.':'I downloaded the backup above and kept it outside this browser. I have separately retained original Break audio.';
 transferView.enter('review','Download the backup where available, read the notices, then confirm. No device write has started.');
 const d=$('#confirm');d.returnValue='';d.showModal();await new Promise(resolve=>d.addEventListener('close',resolve,{once:true}));if(d.returnValue!=='write'||!modalReady()){transferView.cancel();log('Installation review cancelled before device writes.');return;}
 $('#ack').checked=true;$('#backup-saved').checked=true;$('#transfer-status').scrollIntoView({behavior:'smooth',block:'start'});
 await busy(async()=>{
  if(source!==expectedSource||target!==expectedTarget||!allowed())throw Error('Review expired. Inspect and review again.');
  transferView.configure({mode,resume:source?.kind==='loader'});transferView.enter(isResume?'loader':'stage','Rechecking the reviewed integrated package before the device write.');
  const reviewedHash=expectedTarget.sha256;await preload();if(target.sha256!==reviewedHash)throw Error('Build changed after review. Review the new package first.');
  writing=true;sessionStorage.setItem(checkpointKey,JSON.stringify({product:target.product,sha256:target.sha256,mode,audioEmbedded:mode==='rhythmer',at:new Date().toISOString()}));
  lastTransferStage=null;
  source=await transfer(up,source,target,onTransferProgress,{resume:isResume});
  if(mode==='stock'){transferView.enter('final','Official V15 identity matched; finishing verification.');transferView.finish('Official V15 verified: the device returned as FM-1_015.');sessionStorage.removeItem(checkpointKey);state('Verified: FM-1_015 returned. Official V15 firmware restored; custom musical data was not imported.');closeConnection();return;}
  transferView.model.done('identity');
  transferView.enter('final','Verifying Rhythmer and its included factory audio.');
  await verifyEmbeddedBanks(source);await verifyRhythmer(up);
  transferView.finish('Rhythmer with included Amen and Think verified.');sessionStorage.removeItem(checkpointKey);state('Verified: Rhythmer INFO matches and both embedded audio banks pass firmware validation. Check sound and controls on the FM-1 next.');closeConnection();
 });
}
function onTransferProgress(stage,value,detail){
 const names={start:['stage','Preparing the update loader.'],verify:['stage','Serving loader-stage requests.'],loader:['loader','Loader staged. Waiting for its USB MIDI port to appear.'],'write-start':['write','Update loader connected. Waiting for its firmware read requests.'],write:['write','Supplying the firmware package requested by the loader.'],reboot:['reboot','Firmware transfer confirmed. Waiting for the FM-1 to reconnect.'],done:['identity','Firmware reconnected. Checking the returned identity and protocol layout.']};
 const [id,message]=names[stage]||['identity','Checking the device.'];
 if(detail&&(stage==='verify'||stage==='write'))transferView.packet(id,detail,value);else transferView.enter(id,message);
 $('#result').textContent=message;
 if(lastTransferStage!==stage){lastTransferStage=stage;log(message+(typeof value==='string'?' / '+value:''));}
}
async function startFlow(){
 if(working||!bundleReady)return;
 transferView.begin({mode,resume:source?.kind==='loader'});lastTransferStage=null;
 const prepared=await busy(async()=>{
  transferView.enter('device','Identifying the connected FM-1.');if(!source)await connectDevice();
  transferView.configure({mode,resume:source?.kind==='loader'});
  authorize({mode,source,recovery,target,backupSaved:true,ack:true,checkpoint:checkpoint()});
  if(['custom','rhythmer'].includes(source.kind))await captureMusic();else{transferView.enter('backup','Stock firmware / loader cannot export a musical archive here.');transferView.model.skip('backup');}
 });
 if(prepared)await review();
}
$('#start').onclick=()=>startFlow();$('#review').onclick=()=>startFlow();
window.addEventListener('beforeunload',e=>{if(writing){e.preventDefault();e.returnValue='A device write is in progress.';}else closeConnection();});
if(mode==='stock'){
 $('#v15-recommendation').hidden=true;
 $('#start-label').textContent='OFFICIAL RECOVERY / FM-1_015';$('#start-title').textContent='Your stock firmware, ready.';$('#start-description').textContent='The unmodified official M-VAVE V15 image and its Apache-2.0 license/notices are included. Restoration does not write Rhythmer or the Break audio banks.';$('#sample-brief').hidden=true;$('#bundle-download').hidden=true;
 document.title='Rhythmer / Official V15 Recovery';$('#title').textContent='RETURN TO V15.';$('#intro').textContent='Official recovery is included and verified automatically. Connect one FM-1, then start below.';$('#start').textContent='RESTORE OFFICIAL V15 →';$('#review-heading').textContent='Restore the original firmware.';$('#review-copy').textContent='The included, unmodified official V15 is transferred through JIANT/Felucca’s two-stage Web MIDI updater. Completion requires the returned FM-1_015 identity. This mode does not install Rhythmer or its Break samples.';$('#review').textContent='RESTORE OFFICIAL V15 →';
}
$('#report').addEventListener('click',updateReport);
transferView.render(true);updateReport();preload().catch(e=>{bundleReady=false;target=null;$('#build-status').textContent=e.message;state('Installation bundle unavailable: '+e.message);refresh();});refresh();
