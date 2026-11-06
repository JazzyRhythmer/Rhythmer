// SPDX-License-Identifier: GPL-3.0-only
// Browser transport for the real Rhythmer C data engine. No audio or device I/O.
import {readBackup,restoreBackup} from './fm1backup.js';
import {validateBreakSlot} from './break-audio.mjs';
// Web MIDI delivers one complete SysEx message per event. The C USB drain can
// contain a RELOAD push followed by a reply: preserve that same boundary here.
export function splitEditorFrames(bytes){
 const frames=[];let start=-1;
 for(let i=0;i<bytes.length;i++){if(bytes[i]===240)start=i;else if(bytes[i]===247&&start>=0){frames.push(bytes.slice(start,i+1));start=-1;}}
 return frames;
}
const KEY='rhythmer-workspace-v2';
export async function createWorkspace(onStatus=()=>{}) {
  if(!navigator.locks)throw Error('This browser needs Web Locks support to protect project saves. Use Chrome or Edge.');
  let releaseLease;
  await new Promise((resolve,reject)=>{navigator.locks.request('rhythmer-workspace-v2',{ifAvailable:true},async lock=>{
    if(!lock){reject(Error('The workspace is open in another editor. Close that editor, then Connect here.'));return;}
    const held=new Promise(done=>{releaseLease=done;});resolve();await held;
  }).catch(reject);});
  try{return await openWorkspace(onStatus,releaseLease);}catch(error){releaseLease();throw error;}
}
async function openWorkspace(onStatus,releaseLease) {
  let api, dirty=false, timer, restoring=false, closed=false, transferBusy=false, archiveReadAt=-10000;
  const mod=await WebAssembly.compileStreaming(fetch('./workspace.wasm',{cache:'no-store'}));
  const wasi={};
  for(const i of WebAssembly.Module.imports(mod)) {
    if(i.module!=='wasi_snapshot_preview1')throw Error('Unexpected workspace import: '+i.name);
    wasi[i.name]=(...args)=>{
      const v=new DataView(api.memory.buffer);
      switch(i.name){
        case 'clock_time_get':v.setBigUint64(args[2],BigInt(Date.now())*1000000n,true);return 0;
        case 'random_get':crypto.getRandomValues(new Uint8Array(api.memory.buffer,args[0],args[1]));return 0;
        case 'fd_write':v.setUint32(args[3],0,true);return 0;
        case 'environ_sizes_get':case 'args_sizes_get':v.setUint32(args[0],0,true);v.setUint32(args[1],0,true);return 0;
        case 'environ_get':case 'args_get':return 0;
        case 'proc_exit':throw Error('Workspace stopped: '+args[0]);
        default:return 52; // ENOSYS; there are no filesystem or network imports.
      }
    };
  }
  api=(await WebAssembly.instantiate(mod,{wasi_snapshot_preview1:wasi})).exports;
  api._initialize?.();api.ws_init();
  const release=await (await fetch('../release.json',{cache:'no-store'})).json();
  const seed=await (await fetch('../downloads/Rhythmer-six-track-test.fm1save',{cache:'no-store'})).json();
  if(seed.format!=='fm1-emulator-save'||seed.hash!==release.sha256)throw Error('Workspace seed does not match firmware ('+seed.hash?.slice(0,8)+' / '+release.sha256?.slice(0,8)+')');
  const flash=new Uint8Array(api.memory.buffer,api.ws_flash(),1048576), seen=new Set();
  for(const s of seed.sectors){
    const b=Uint8Array.from(atob(s.data),c=>c.charCodeAt(0));
    if(!Number.isInteger(s.at)||s.at<0||s.at%4096||s.at+4096>flash.length||b.length!==4096||seen.has(s.at))throw Error('Invalid seed sector');
    seen.add(s.at);flash.set(b,s.at);
  }
  // Audio banks are global, like the physical device, and remain separate from
  // musical archives. Persist before touching RAM flash so quota failure is atomic.
  const factoryAudio=[flash.slice(0xA0000,0xB4000),flash.slice(0xB4000,0xC8000)];
  const audioKey='rhythmer-break-audio-v1';let audioState={version:1,slots:[null,null]};
  const encodeAudio=bytes=>{let text='';for(const b of bytes)text+=String.fromCharCode(b);return btoa(text);};
  const decodeAudio=record=>{
    if(!record||typeof record.data!=='string'||record.data.length!==4*Math.ceil(81920/3))throw Error('Invalid saved audio record');
    return validateBreakSlot(Uint8Array.from(atob(record.data),c=>c.charCodeAt(0)));
  };
  const previousAudio=localStorage.getItem(audioKey);
  if(previousAudio){
    const value=JSON.parse(previousAudio);if(value.version!==1||!Array.isArray(value.slots)||value.slots.length!==2)throw Error('Saved audio library is invalid; original stored data is retained.');
    const decoded=value.slots.map(r=>r?decodeAudio(r):null);
    audioState=value;decoded.forEach((bytes,slot)=>{if(bytes)flash.set(bytes,0xA0000+slot*0x14000);});
  }
  api.ws_boot();
  const raw=bytes=>{
    if(closed)throw Error('Workspace is closed');
    if(bytes.length>1024)throw Error('Editor request too large');
    new Uint8Array(api.memory.buffer,api.ws_input(),bytes.length).set(bytes);
    const n=api.ws_request(bytes.length);
    if(!n)throw Error('Unsupported workspace request');
    return new Uint8Array(api.memory.buffer,api.ws_output(),n).slice();
  };
  const request=async([cmd,args=[]])=>{
    const d=raw([240,125,70,76,cmd,...args,247]);
    let p=0;while(p<d.length){const e=d.indexOf(247,p);if(e<0)break;if(d[p+4]===cmd)return Array.from(d.slice(p+5,e));p=e+1;}
    throw Error('Missing editor reply');
  };
  const snapshot=()=>{
    const n=api.ws_archive();if(!n)throw Error('Project exceeds the device section capacity. Shorten the pattern before saving.');
    const data=new Uint8Array(api.memory.buffer,api.ws_archive_ptr(),n),v=new DataView(data.buffer,data.byteOffset,data.byteLength);
    const count=v.getUint32(0,true),objects=[];let p=4;
    for(let k=0;k<count;k++){
      const id=v.getUint32(p,true),size=v.getUint32(p+4,true),crc=v.getUint32(p+8,true);p+=12;
      let s='';for(const b of data.subarray(p,p+size))s+=String.fromCharCode(b);p+=size;
      objects.push({id,size,crc,data:btoa(s)});
    }
    const file={format:'felucca-backup',version:1,firmware:'JIANT Rhythmer dev',created:new Date().toISOString(),objects};
    readBackup(file);return file;
  };
  const save=()=>{
    clearTimeout(timer);if(!dirty||restoring||closed)return;
    if(transferBusy||performance.now()-archiveReadAt<750){timer=setTimeout(save,750);return;}
    try{localStorage.setItem(KEY,JSON.stringify(snapshot()));dirty=false;onStatus('Saved in this browser · 4 projects / A–D');}
    catch(e){onStatus('Save failed: '+e.message);}
  };
  const mark=()=>{dirty=true;clearTimeout(timer);onStatus('Saving browser workspace…');timer=setTimeout(save,500);};
  const currentStored=localStorage.getItem(KEY);
  const previousStored=currentStored?null:localStorage.getItem('rhythmer-workspace-v1');
  const stored=currentStored||previousStored;
  if(stored){restoring=true;try{const file=readBackup(stored);if(file.firmware!=='JIANT Rhythmer dev')throw Error('Unsupported workspace firmware');await restoreBackup(request,file);if(previousStored)localStorage.setItem(KEY,stored);onStatus(previousStored?'Previous workspace copied into this editor · original browser data kept':'Browser workspace restored');}catch(e){throw Error('Stored workspace could not be restored; original data is kept. '+e.message);}finally{restoring=false;}}
  else onStatus('Browser workspace ready · edits autosave locally');
  const input={id:'rhythmer-workspace-in',name:'Rhythmer Browser Workspace',manufacturer:'Rhythmer',type:'input',state:'connected',connection:'open',onmidimessage:null,open:async()=>input,close:async()=>{}};
  const reads=new Set([1,2,4,5,6,10,16,17,25,29,34,36,37,65,66,68,70,77]);
  const output={id:'rhythmer-workspace-out',name:input.name,manufacturer:input.manufacturer,type:'output',state:'connected',connection:'open',open:async()=>output,close:async()=>{},send(bytes){
    const reply=raw(bytes),cmd=bytes[4];
    if(cmd===65||cmd===66)archiveReadAt=performance.now();
    if(cmd===67)transferBusy=bytes[5]<2;
    if(cmd===74)transferBusy=true;if(cmd===76)transferBusy=false;
    const readOnly=reads.has(cmd)||cmd===22||cmd===27&&bytes.length===6||cmd===33&&(bytes.length===6||bytes[5]===0)||cmd===28&&bytes.length===7||cmd===30&&bytes.length<=9||cmd===31&&bytes.length===8||cmd===64&&bytes[6]===0;
    if(!readOnly&&!restoring)mark();

    const messages=splitEditorFrames(reply);
    queueMicrotask(()=>{for(const data of messages)input.onmidimessage?.({data,timeStamp:performance.now()});});
  }};
  const access={inputs:new Map([[input.id,input]]),outputs:new Map([[output.id,output]]),onstatechange:null};
  const flush=()=>{if(dirty)save();};
  const leave=()=>{flush();releaseLease();closed=true;};
  window.addEventListener('pagehide',leave);
  return {access,snapshot,flush,
  breakInfo:slot=>audioState.slots[slot]?{...audioState.slots[slot],data:undefined}:null,
  breakSlot:slot=>{if(slot!==0&&slot!==1)throw Error('Invalid audio slot');return flash.slice(0xA0000+slot*0x14000,0xA0000+(slot+1)*0x14000);},
  resetBreak:slot=>{
    if(closed||transferBusy||restoring||slot!==0&&slot!==1)throw Error('Workspace is closed, busy or slot is invalid');
    const next={version:1,slots:[...audioState.slots]};next.slots[slot]=null;
    localStorage.setItem(audioKey,JSON.stringify(next));flash.set(factoryAudio[slot],0xA0000+slot*0x14000);audioState=next;onStatus('Bundled Break '+(slot?'B':'A')+' restored locally');
  },
  replaceBreak:(slot,bytes,meta={})=>{
    if(closed||transferBusy||restoring)throw Error('Workspace is closed or busy');if(slot!==0&&slot!==1)throw Error('Invalid audio slot');validateBreakSlot(bytes);
    const next={version:1,slots:[...audioState.slots]};next.slots[slot]={name:String(meta.name||'User break').slice(0,120),sourceDuration:Number(meta.sourceDuration)||3,pitchSemitones:Number(meta.pitchSemitones)||0,updated:new Date().toISOString(),data:encodeAudio(bytes)};
    localStorage.setItem(audioKey,JSON.stringify(next));flash.set(bytes,0xA0000+slot*0x14000);audioState=next;onStatus('User Break '+(slot?'B':'A')+' saved locally · included in SAVE FOR PLAY');
  },emulatorSave:()=>{
    if(api.ws_runtime_flash())throw Error('Current pattern cannot fit the firmware section capacity');
    const flash=new Uint8Array(api.memory.buffer,api.ws_flash(),1048576),sectors=[];
    for(let at=0;at<flash.length;at+=4096){const b=flash.subarray(at,at+4096);if(b.every(v=>v===255))continue;let text='';for(const v of b)text+=String.fromCharCode(v);sectors.push({at,data:btoa(text)});}
    return {format:'fm1-emulator-save',version:1,hash:release.sha256,name:'Rhythmer web workspace',audioNotice:'Recordings are separate from the firmware code license. Bundled Amen/Think public redistribution clearance is not documented; user audio rights are not assessed. Credits are not a license.',sampleSources:audioState.slots.map((x,i)=>x?{slot:i?'B':'A',source:'user-provided',name:x.name,rights:'not-assessed'}:{slot:i?'B':'A',source:i?'Lyn Collins / Think (About It) / Rhythm Lab':'The Winstons / Amen, Brother / Amenity',rights:'public-redistribution-clearance-not-documented'}),package:seed.package,sectors};
  },project:async n=>{if(api.ws_project(n))throw Error('Save the current section before switching projects.');mark();},current:()=>api.ws_project_current(),mask:n=>api.ws_project_mask(n),generate:(track,mutate)=>{if(api.ws_acid(track,mutate))throw Error('Select an Acid track first');mark();},close(){flush();closed=true;clearTimeout(timer);releaseLease();window.removeEventListener('pagehide',leave);}};
}
