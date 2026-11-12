// SPDX-License-Identifier: GPL-3.0-only
// V15/V16 validation follows JIANT's pinned official-package checks.
import {validateStockPackage} from './fm1pkg.js?v=install-safety-20261112.2';
export const INSTALLER_REVISION='install-safety-20261112.2';
export const STOCK_V16={product:'FM-1_016',bytes:718196,sha256:'d017fb65e70aca97ee29d944470c3515a74581df5c0576bf70de917c7b2b61d5'};
export const stockName=p=>p==='FM-1_016'?'Official V16':'Official V15';
export async function validateOfficial(bytes){return validateStockPackage(bytes,true);}
export function recoveryPolicy(source,recovery,mode){
 const id=typeof source?.id==='string'?source.id:source?.id?.text;
 if(!['FM-1_015','FM-1_016'].includes(recovery?.product))throw Error('Select and verify an unmodified official V15 or V16 recovery file.');
 if(['FM-1_015','FM-1_016'].includes(id)&&recovery.product!==id)
  throw Error(`${id} detected. Keep its matching ${stockName(id)} recovery file: select the original FM-1.fwsc below. Do not downgrade to V15 as an installation workaround.`);
 return true;
}
export function deviceSummary(source){
 if(!source)return null;
 const id=typeof source.id==='string'?source.id:source.id?.text;
 return {identity:id,firmwareName:source.name||null,kind:source.kind||(source.resume?'loader':/^FM-1_01[56]$/.test(id)?'official':'companion'),input:{name:source.input?.name||'',id:source.input?.id||'',state:source.input?.state||''},output:{name:source.output?.name||'',id:source.output?.id||'',state:source.output?.state||''},flash:{jedec:null,status:'not-exposed-by-current-midi-protocol'},bootOrigin:/^FM-1_01[56]$/.test(id)?id:'unknown-do-not-infer-from-name-or-colour'};
}
export function environment(){return {installerRevision:INSTALLER_REVISION,url:location.href,userAgent:navigator.userAgent,secureContext:globalThis.isSecureContext,webMIDI:typeof navigator.requestMIDIAccess==='function',webUSB:!!navigator.usb,webLocks:!!navigator.locks};}
export function failureDetails(e,{mode,target,stage,writing}={}){
 const code=e.code||'validation';let advice='Keep the diagnostic report and the exact error. Inspect the device before another write.';
 if(code==='noupdate')advice='The running firmware did not enter the updater. Stop playback and close MIDI apps. Older custom firmware may reject a newer flash chip; this is not proof of a defective chip. Use that firmware’s documented recovery route.';
 else if(e.phase==='stage'&&code==='stopped'&&mode==='stock'&&e.served>=4&&e.served<=35)advice=`The firmware stopped while comparing the official boot area. ${target?.product==='FM-1_016'?'V15':'V16'} may be the matching recovery version. Verify the original version before selecting it; the boot-area protection has not been bypassed.`;
 else if(code==='noloader')advice='The staging phase finished, but the loader did not reconnect. Keep stable power, check the USB port and save this report. Do not repeatedly restart a full installation.';
 else if(['lost','stopped'].includes(code))advice='Transfer stopped. Keep power stable and inspect the current device identity. Resume only the same reviewed package if its MIDI loader responds.';
 else if(code==='mismatch'||code==='noreturn'||stage==='identity'||stage==='final')advice='The transfer may have completed. Verify the running firmware and save this report before trying recovery; a failed final check is not proof that flashing failed.';
 return {code,message:e.message,phase:e.phase||stage||null,served:e.served??null,detail:e.detail||null,writeMayHaveStarted:!!writing,advice};
}
