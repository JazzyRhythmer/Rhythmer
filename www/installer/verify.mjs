// SPDX-License-Identifier: GPL-3.0-only
// Only identity/INFO and the bank status query. No transfer/restore commands.
import {DeviceUpdater,inspect,verifyEmbeddedBanks,strictInfo} from './core.mjs?v=3156538adfcd';
const $=s=>document.querySelector(s);
$('#verify').addEventListener('click',async()=>{
 $('#verify').disabled=true;$('#result').textContent='';
 try{
  if(!navigator.requestMIDIAccess)throw Error('Use desktop Chrome or Edge with Web MIDI.');
  await navigator.locks.request('rhythmer-device-operation',{ifAvailable:true},async lock=>{
   if(!lock)throw Error('Another editor/installer owns the device. Finish that operation first.');
   const access=await navigator.requestMIDIAccess({sysex:true});
   const up=new DeviceUpdater(access,()=>{});$('#status').textContent='Reading identity and INFO…';
   const source=await inspect(up);
   if(source.kind!=='rhythmer'||!strictInfo(source.info))throw Error('The device does not match the supported Rhythmer identity and ABI.');
   $('#status').textContent='Reading the firmware’s bank validation flags…';
   await verifyEmbeddedBanks(source);
   const name=String.fromCharCode(...source.info.slice(0,source.info.indexOf(0)));
   $('#result').textContent=JSON.stringify({readOnly:true,device:source.id.text,info:name,editorABI:'127 parameters / 71 globals / 64 steps / engine parameters at 119',factoryBankMask:3,bothAudioSlotsValid:true,fullFirmwareReadback:false,physicalRecoveryTested:false},null,2);
   $('#status').textContent='PASS · Running Rhythmer recognized; both A/B audio banks pass the firmware’s validation. No device writes were performed.';
  });
 }catch(e){$('#status').textContent='CHECK STOPPED · '+e.message;}
 finally{$('#verify').disabled=false;}
});
