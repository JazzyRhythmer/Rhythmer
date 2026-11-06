// SPDX-License-Identifier: GPL-3.0-only
import {validateBreakSlot,slotPCM,previewWav,uploadBreakBank} from './break-audio.mjs';
const create=(tag,attrs={},text='')=>{const e=document.createElement(tag);for(const [k,v] of Object.entries(attrs))e.setAttribute(k,v);e.textContent=text;return e;};
export function initBreakSamples({container,mode,getWorkspace,getDevice,request,operation,downloadEmulator}){
 container.innerHTML='<h2>BREAK / YOUR TWO-BAR LOOPS</h2><p>Choose a complete two-bar audio file for A or B. The entire file is treated as eight beats: it is repitched to exactly 3 seconds at 160 BPM, mixed to mono, anti-alias resampled and packed to 12-bit / 16 kHz. Repitching changes pitch as well as tempo. Audio stays in this browser.</p><p class="dim">Each slot uses 72,000 audio bytes in an 80 KiB device bank. A/B are shared by all tracks and projects. Musical archives exclude audio; SAVE FOR PLAY includes both banks. Use audio you have the rights to use; uploading does not grant recording/composition or redistribution rights. <a href="../#samples" target="_top">Bundled sample credits & rights →</a></p>';
 const cards=create('div',{class:'rhy-sample-grid'});container.append(cards);
 const state=[];let writing=false;
 const showBank=(slot,card)=>{const ws=getWorkspace();if(mode==='browser'&&ws){const meta=ws.breakInfo(slot);card.querySelector('.bank-info').textContent=meta?'Saved user audio: '+meta.name:'Bundled audio: '+(slot?'Think / drum edit':'Amen');}else card.querySelector('.bank-info').textContent='Device audio is not readable by this backup protocol. Keep the original file.';};
 for(let slot=0;slot<2;slot++){
  const card=create('article',{class:'rhy-sample-card'}),title=create('h3',{},'SLOT '+(slot?'B':'A')),bank=create('p',{class:'bank-info dim'}),file=create('input',{type:'file',accept:'audio/*,.wav,.aif,.aiff,.flac,.mp3,.ogg,.slot', 'aria-label':'Two-bar audio for slot '+(slot?'B':'A')}),status=create('p',{role:'status'},'Choose a two-bar loop. WAV is recommended; a previously converted .slot bank can also be restored.'),audio=create('audio',{controls:'',preload:'metadata','aria-label':'Converted preview '+(slot?'B':'A')}),apply=create('button',{disabled:''},mode==='browser'?'SAVE TO BROWSER SLOT '+(slot?'B':'A'):'WRITE TO FM-1 SLOT '+(slot?'B':'A')),download=create('a',{hidden:'',download:'Rhythmer-user-'+(slot?'B':'A')+'.slot'},'DOWNLOAD CONVERTED BANK ↓');
  const savedPreview=create('button',{},'PREVIEW SAVED SLOT '+(slot?'B':'A'));savedPreview.hidden=mode!=='browser';
  audio.hidden=true;card.append(title,bank,savedPreview,file,status,audio,apply,download);cards.append(card);const item={slot,card,file,status,audio,apply,download,prepared:null,previewURL:null,bankURL:null,job:0};state.push(item);showBank(slot,card);
  if(mode==='browser'){
   const reset=create('button',{},'RESTORE BUNDLED '+(slot?'B':'A'));card.append(reset);
   reset.addEventListener('click',async()=>{
    const ws=getWorkspace();if(!ws){status.textContent='Connect the workspace first.';return;}
    const d=create('dialog',{class:'rhy-sample-confirm'}),form=create('form',{method:'dialog'});form.append(create('h2',{},'Restore bundled '+(slot?'B / Think':'A / Amen')+'?'),create('p',{},'This replaces only this browser audio slot. Keep your original user recording or download the converted bank first. Projects, patterns and the other slot are kept. This does not write a physical device.'),create('button',{value:'cancel'},'CANCEL'),create('button',{value:'restore'},'RESTORE BUNDLED AUDIO'));d.append(form);document.body.append(d);d.showModal();await new Promise(r=>d.addEventListener('close',r,{once:true}));const result=d.returnValue;d.remove();if(result!=='restore')return;
    try{ws.resetBreak(slot);item.prepared=null;apply.disabled=true;file.value='';audio.pause();audio.removeAttribute('src');audio.hidden=true;download.hidden=true;status.textContent='Bundled '+(slot?'Think / drum edit':'Amen')+' restored to browser slot '+(slot?'B':'A')+'.';showBank(slot,card);}catch(e){status.textContent=e.message;}
   });
  }
  savedPreview.addEventListener('click',()=>{try{const ws=getWorkspace();if(!ws)throw Error('Connect the workspace first');const bytes=ws.breakSlot(slot);if(item.previewURL)URL.revokeObjectURL(item.previewURL);item.previewURL=URL.createObjectURL(new Blob([previewWav(slotPCM(bytes))],{type:'audio/wav'}));audio.src=item.previewURL;audio.hidden=false;status.textContent='Saved slot preview ready: 3.000 seconds, actual PCM12 audio.';}catch(e){status.textContent=e.message;}});
  file.addEventListener('change',async()=>{
   const selected=file.files[0];if(!selected)return;const job=++item.job;item.prepared=null;apply.disabled=true;audio.hidden=true;download.hidden=true;
   try{
    if(selected.size>12*1024*1024)throw Error('Choose a file smaller than 12 MiB.');status.textContent='Decoding locally…';
    let converted,sourceDuration;
    if(/\.slot$/i.test(selected.name)){
      const bytes=validateBreakSlot(new Uint8Array(await selected.arrayBuffer()));converted={bytes,quant:slotPCM(bytes)};sourceDuration=3;
    }else{
      const Context=window.OfflineAudioContext||window.webkitOfflineAudioContext;if(!Context)throw Error('Audio decoding is unavailable. Use current Chrome or Edge.');
      const context=new Context(1,48000,16000);const decoded=await context.decodeAudioData(await selected.arrayBuffer());
      if(job!==item.job)return;if(decoded.duration<.5||decoded.duration>16||decoded.numberOfChannels>8)throw Error('Use a two-bar loop between 0.5 and 16 seconds, up to 8 channels.');sourceDuration=decoded.duration;
      const mono=new Float32Array(decoded.length);for(let c=0;c<decoded.numberOfChannels;c++){const ch=decoded.getChannelData(c);for(let i=0;i<mono.length;i++)mono[i]+=ch[i]/decoded.numberOfChannels;}
      const worker=new Worker('./break-convert-worker.mjs',{type:'module'});
      converted=await new Promise((resolve,reject)=>{worker.onmessage=e=>{const d=e.data;if(d.type==='progress'){if(job===item.job)status.textContent='Anti-alias conversion: '+Math.round(d.value*100)+'%';}else if(d.type==='done'){worker.terminate();resolve(d);}else{worker.terminate();reject(Error(d.message));}};worker.onerror=e=>{worker.terminate();reject(Error(e.message||'Conversion worker failed'));};worker.postMessage({mono,name:selected.name},[mono.buffer]);});
    }
    if(job!==item.job)return;validateBreakSlot(converted.bytes);const pitch=12*Math.log2(sourceDuration/3),meta={name:selected.name,sourceDuration,pitchSemitones:pitch};item.prepared={bytes:converted.bytes,meta};
    if(item.previewURL)URL.revokeObjectURL(item.previewURL);if(item.bankURL)URL.revokeObjectURL(item.bankURL);
    item.previewURL=URL.createObjectURL(new Blob([previewWav(converted.quant)],{type:'audio/wav'}));item.bankURL=URL.createObjectURL(new Blob([converted.bytes],{type:'application/octet-stream'}));audio.src=item.previewURL;audio.hidden=false;download.href=item.bankURL;download.hidden=false;apply.disabled=false;
    status.textContent=`Ready: ${sourceDuration.toFixed(3)}s (${(480/sourceDuration).toFixed(1)} BPM if two bars) → 3.000s / 160 BPM. Pitch ${pitch>=0?'+':''}${pitch.toFixed(2)} semitones. Mono PCM12 / 16 kHz / 72,000 B. Preview is the actual quantized audio.`;
   }catch(error){status.textContent=error.message;}
  });
  apply.addEventListener('click',async()=>{
   if(writing||!item.prepared)return;const prepared=item.prepared;
   if(mode==='browser'){
    try{const ws=getWorkspace();if(!ws)throw Error('Connect the browser workspace first.');ws.replaceBreak(slot,prepared.bytes,prepared.meta);status.textContent='Saved '+prepared.meta.name+' to browser audio slot '+(slot?'B':'A')+'. Included in SAVE FOR PLAY.';showBank(slot,card);}catch(e){status.textContent=e.message;}return;
   }
   const d=getDevice();if(!d){status.textContent='Connect the matching Rhythmer FM-1 first.';return;}
   const dialog=create('dialog',{class:'rhy-sample-confirm'});dialog.innerHTML='<form method="dialog"><h2>Replace Break '+(slot?'B':'A')+' on this FM-1?</h2><p>This writes one 80 KiB audio bank and stops playback. Other audio banks, projects and settings are not targeted. Current sample bytes cannot be backed up by this protocol; retain your original audio files. A disconnect can leave this slot invalid; retry the complete upload.</p><label><input type="checkbox" class="audio-write-notice"> I have read the risk notice: Rhythmer is independent of M-VAVE. Device damage, malfunction or data loss are possible. Warranty/liability limits apply only to the extent permitted by law; mandatory rights and non-excludable liability are unaffected.</label><div class="bar"><button value="cancel">CANCEL</button><button value="write" disabled>WRITE AUDIO SLOT '+(slot?'B':'A')+'</button></div></form>';document.body.append(dialog);const notice=dialog.querySelector('.audio-write-notice'),write=dialog.querySelector('[value=write]');notice.addEventListener('change',()=>write.disabled=!notice.checked);dialog.querySelector('form').addEventListener('submit',e=>{if(e.submitter?.value==='write'&&!notice.checked)e.preventDefault();});dialog.showModal();await new Promise(r=>dialog.addEventListener('close',r,{once:true}));const answer=dialog.returnValue;dialog.remove();if(answer!=='write'||!notice.checked)return;
   writing=true;for(const x of state){x.file.disabled=true;x.apply.disabled=true;}
   try{await navigator.locks.request('rhythmer-device-operation',{ifAvailable:true},async lock=>{if(!lock)throw Error('Another installer is using this FM-1.');return operation(async()=>{
    if(getDevice()!==d)throw Error('Device changed; reconnect before writing.');
    await request(d,[65,[]],{timeout:8000,retries:0});
    await uploadBreakBank((r,o)=>request(d,r,o),slot,prepared.bytes,v=>{status.textContent='Writing audio: '+Math.round(v*100)+'% — keep USB/power connected.';});
    status.textContent='Device verified '+prepared.meta.name+' in audio slot '+(slot?'B':'A')+'. Audio is ready; this firmware may keep the old LCD waveform preview until restart. Save musical edits before restarting.';
   });});}catch(e){status.textContent='Audio upload stopped: '+e.message;}finally{writing=false;for(const x of state){x.file.disabled=false;x.apply.disabled=!x.prepared;}}
  });
 }
 if(mode==='browser')container.append(create('button',{id:'sample-save-for-play'},'SAVE FOR PLAY WITH BOTH SAMPLES ↓'));container.querySelector('#sample-save-for-play')?.addEventListener('click',()=>downloadEmulator());
 window.addEventListener('beforeunload',e=>{if(writing){e.preventDefault();e.returnValue='An audio-bank write is in progress.';}});
 const refresh=setInterval(()=>{if(!container.isConnected){clearInterval(refresh);return;}for(const x of state)showBank(x.slot,x.card);},1500);
}
