// SPDX-License-Identifier: GPL-3.0-only
import {Updater,PORT_RE,pairOutput} from './transport.mjs?v=f4a3fd0a6f94';
import {Package,logicalImage,sha256hex,validateStockPackage} from './fm1pkg.js?v=f4a3fd0a6f94';
import {BackupConnection,captureBackup,bkPack,bkCrc} from './fm1backup.js?v=f4a3fd0a6f94';
import {parseInfo} from './midi.mjs?v=f4a3fd0a6f94';
export {validateStockPackage,sha256hex};
const ensure=(yes,why)=>{if(!yes)throw Error(why);};
const RHYTHMER_DEV_PRODUCTS=['FM-1_748007','FM-1_748008','FM-1_748010','FM-1_748011','FM-1_748012','FM-1_748013','FM-1_748014','FM-1_748015','FM-1_748016','FM-1_748017','FM-1_748018','FM-1_748019','FM-1_748020','FM-1_748021','FM-1_748022','FM-1_748023'];
const RHYTHMER_PRODUCTS=['FM-1_900','FM-1_748001','FM-1_748005',...RHYTHMER_DEV_PRODUCTS];
const engineCount=product=>RHYTHMER_DEV_PRODUCTS.includes(product)?25:22;
const globalCount=product=>['FM-1_748020','FM-1_748021','FM-1_748022','FM-1_748023'].includes(product)?77:
 ['FM-1_748018','FM-1_748019'].includes(product)?76:71;
export function strictInfo(body,product){
 const v=parseInfo([240,125,70,76,1,...body,247]);
 if(!v?.isRhythmer)return null;
 const z=body.indexOf(0);
 const globals=body[z+3];
 const validGlobals=product?globals===globalCount(product):
  globals===71||(v.engines===25&&[76,77].includes(globals));
 return body[z+2]===127&&validGlobals&&body[z+4]===64&&body[z+5]===119?{...v,globals}:null;
}
export function sourceKind(id,info){
 if(id.model==='ota-FM-1')return 'loader';
 if(id.model!=='FM-1')return 'unsupported';
 const layout=strictInfo(info||[],id.text);
 if(RHYTHMER_PRODUCTS.includes(id.text)&&layout&&
    (engineCount(id.text)===25?layout.engines===25:layout.engines===21||layout.engines===22))return 'rhythmer';
 if(id.version>=1&&id.version<=16)return id.version===15?'v15':'stock';
 return 'custom';
}
export function authorize({mode,source,recovery,target,backupSaved,ack,checkpoint,repair=false}){
 ensure(recovery?.product==='FM-1_015','Prepare the verified official V15 recovery file first.');
 ensure(mode==='stock'?target?.product==='FM-1_015':RHYTHMER_PRODUCTS.includes(target?.product),'No verified target package.');
 ensure(source&&source.kind!=='unsupported','Inspect one compatible FM-1 first.');
 ensure(ack,'Read and acknowledge the installation notice.');
 ensure(backupSaved,'Download your backup and acknowledge it, or acknowledge that stock/loader data cannot be backed up.');
 if(source.kind==='loader'){
  ensure(!repair,'Sample repair needs a running Rhythmer.');
  ensure(mode==='stock'||checkpoint?.sha256===target.sha256&&checkpoint?.product===target.product,'Unknown update session: use official V15 recovery first.');
 }else if(mode==='rhythmer')ensure(source.kind==='v15'||source.kind==='rhythmer','Return to official V15 before installing Rhythmer from this firmware.');
 if(repair)ensure(source.kind==='rhythmer','Sample repair needs the matching Rhythmer build.');
 return true;
}
export async function validateRhythmer(bytes,meta){
 ensure(bytes.length===meta.bytes&&await sha256hex(bytes)===meta.sha256,'Rhythmer package checksum mismatch. Nothing was written.');
 const p=new Package(bytes);ensure(p.product===meta.product,'Wrong Rhythmer product.');
 ensure(meta.embeddedBreaks===true&&p.flash.length===0xc8000,'This installer requires the package with both embedded Break banks.');
 const marker=p.flash.subarray(0x93000,0x93020),v=new DataView(marker.buffer,marker.byteOffset,32);
 ensure(String.fromCharCode(...marker.subarray(0,8))==='RHYBRK1\0'&&v.getUint32(8,true)===1&&v.getUint32(12,true)===0xa0000&&v.getUint32(16,true)===0xc8000&&v.getUint32(20,true)===0x14000&&v.getUint32(24,true)===2&&bkCrc(marker.subarray(0,28))===v.getUint32(28,true),'Invalid integrated factory-bank marker.');
 return {product:p.product,image:logicalImage(bytes),sha256:meta.sha256,embeddedBanks:[p.flash.slice(0xa0000,0xb4000),p.flash.slice(0xb4000,0xc8000)]};
}
export async function validateLoop(bytes,meta){
 ensure(bytes.length===meta.bytes&&await sha256hex(bytes)===meta.sha256,'Factory loop checksum mismatch.');
 const h=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
 ensure(bytes.length===81920&&h.getUint32(0,true)===0x504d5346&&bytes[4]===12&&bytes[5]===0&&bytes[6]===1&&h.getUint32(16,true)===72000&&h.getUint32(32,true)===0&&h.getUint32(36,true)===48000&&h.getUint32(48,true)===23777&&bkCrc(bytes.subarray(512,72512))===h.getUint32(20,true),'Invalid factory loop header or payload CRC.');
 return bytes;
}
// One browser-side install bundle. This is not a device-flash image and is never
// expanded into an address range that could overwrite project/settings sectors.
export async function validateInstallBundle(bytes,manifest){
 ensure(bytes.length===manifest.bundle.bytes&&bytes.length<4000000&&await sha256hex(bytes)===manifest.bundle.sha256,'Installation bundle checksum mismatch.');
 const b=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));
 ensure(b.format==='rhythmer-install'&&b.version===2&&b.audioEmbedded===true&&typeof b.localOnly==='boolean'&&b.localOnly===manifest.localOnly&&b.publicRedistributionCleared===false&&Array.isArray(b.loops)&&b.loops.length===2,'Unsupported installation bundle.');
 ensure(b.localOnly||b.audioRights?.distributionDecision?.ownerDirectedPublicDistribution===true,'Public distribution has not been requested by this project owner.');
 const decode=(entry,meta)=>{
  ensure(entry?.bytes===meta.bytes&&entry.sha256===meta.sha256&&entry.product===meta.product&&typeof entry.data==='string'&&entry.data.length===4*Math.ceil(meta.bytes/3)&&/^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$/.test(entry.data),'Invalid bundled asset metadata.');
  return Uint8Array.from(atob(entry.data),x=>x.charCodeAt(0));
 };
 const firmware=await validateRhythmer(decode(b.firmware,manifest.firmware),manifest.firmware);
 const recovery=await validateStockPackage(decode(b.recovery,manifest.recovery));
 const loops=await Promise.all(b.loops.map((x,i)=>validateLoop(decode(x,manifest.loops[i]),manifest.loops[i])));
 ensure(firmware.embeddedBanks.every((x,i)=>x.length===loops[i].length&&x.every((v,j)=>v===loops[i][j])),'Embedded Break banks do not match the reviewed factory audio.');
 ensure(b.audioRights?.publicRedistributionCleared===false&&b.audioRights.samples?.length===2&&b.audioRights.samples.every((x,i)=>x.slotSHA256===manifest.loops[i].sha256&&x.status==='clearance-not-documented'),'Missing or incompatible audio rights notice.');
 return {firmware,recovery,loops,audioRights:b.audioRights};
}
const candidate=p=>p.state!=='disconnected'&&(PORT_RE.test(p.name||'')||/rhythmer/i.test(p.name||''));
export class DeviceUpdater extends Updater{
 async write(ota,image,step){step('write-start',ota.id.text);return super.write(ota,image,step);}
 async find(filter,opts){
  // Conservatively refuse multiple endpoints rather than risk updating the wrong unit.
  const ins=[...this.access.inputs.values()].filter(candidate),outs=[...this.access.outputs.values()].filter(candidate);
  ensure(ins.length<=1&&outs.length<=1,'Connect only one FM-1; close duplicate MIDI endpoints before continuing.');
  const found=await super.find(filter,{...opts,others:false});
  if(found&&this.expectedFirst){const s=this.expectedFirst;this.expectedFirst=null;try{ensure(found.id.text===s.id.text&&found.link.input.id===s.input.id&&found.link.output.id===s.output.id,'Device changed at upgrade entry. Inspect and back up again.');}catch(e){found.link.close();throw e;}}
  return found;
 }
}
export async function inspect(up){
 const d=await up.find(()=>true,{tries:1});ensure(d,'No responding FM-1. Check the USB data cable and close other MIDI apps.');
 d.link.close();
 let info=[];
 if(d.id.model==='FM-1'&&d.id.version>=700){const c=new BackupConnection(d.link.input,d.link.output);try{info=await c.request([1,[]],{timeout:1800,retries:1});}catch{}finally{c.close();}}
 return {kind:sourceKind(d.id,info),id:d.id,info,input:d.link.input,output:d.link.output};
}
export async function backup(source,progress){
 ensure(source.kind==='rhythmer'||source.kind==='custom','No musical archive protocol on this firmware.');
 const c=new BackupConnection(source.input,source.output);
 try{return await captureBackup((r,o)=>c.request(r,o),String.fromCharCode(...source.info.slice(0,source.info.indexOf(0))),progress,{complete:true});}finally{c.close();}
}
export async function verifyRhythmer(up,target){
 const s=await inspect(up);
 const expected=target?.product??s.id.text;
 ensure(s.kind==='rhythmer'&&s.id.text===expected&&strictInfo(s.info,expected)?.engines===engineCount(expected),
   'Firmware returned but its product identity or Rhythmer INFO layout did not match the selected build. Installation is not verified.');
 return s;
}
export async function verifyEmbeddedBanks(source){
 const c=new BackupConnection(source.input,source.output);
 try{const r=await c.request([80,[]],{timeout:8000});
  ensure(r.length===6&&String.fromCharCode(...r.slice(0,5))==='SIX3\0','Installed factory-bank validation failed: invalid device response. Stop and inspect before retrying the full package.');
  const mask=r[5];ensure(mask===3,`Installed factory-bank validation failed: A / Amen ${mask&1?'ready':'not readable'}, B / Think ${mask&2?'ready':'not readable'} (mask ${mask}). Stop and inspect before retrying the full package.`);return mask;
 }finally{c.close();}
}
export async function confirmSource(up,source){
 const now=await inspect(up);
 ensure(now.id.text===source.id.text&&now.input.id===source.input.id&&now.output.id===source.output.id&&now.kind===source.kind&&JSON.stringify(now.info)===JSON.stringify(source.info),'Device changed after review. Reconnect and create a fresh backup.');
 return now;
}
export async function transfer(up,source,target,progress,{resume=false}={}){
 await confirmSource(up,source);
 up.expectedFirst=source;
 const guarded=(stage,value,detail)=>{if(stage==='start')ensure(value===source.id.text,'Firmware identity changed before writing.');progress(stage,value,detail);};
 if(resume)ensure(await up.resume(target.image,guarded,target.product),'Loader resume did not complete. Keep the same target file and reconnect.');
 else await up.install(target.image,target.product,guarded);
 return RHYTHMER_PRODUCTS.includes(target.product)?await verifyRhythmer(up,target):null;
}
