// Rhythmer additions. Uses the upstream panel input and persistence functions.
function updateRhythmerMeter(peak) {
  const meter=document.getElementById('rhy-level'),db=document.getElementById('rhy-db');
  if(meter)meter.value=Math.min(1,peak);
  if(db)db.textContent=peak>0?'Output '+Math.max(-90,Math.round(20*Math.log10(peak)))+' dBFS':'Output −∞ dBFS';
}
document.querySelectorAll('[data-key]').forEach(button=>{
  button.addEventListener('click',()=>{if(!node)return;const id=Number(button.dataset.key);press(id);setTimeout(()=>unpress(id),90);canvas.focus({preventScroll:true});});
});
document.querySelectorAll('[data-enc]').forEach(button=>button.addEventListener('click',()=>{if(node)turn(2,Number(button.dataset.enc));canvas.focus({preventScroll:true});}));
// turn() takes panel encoder index: ALGORITHM is index 2 (hardware encoder 1).
canvas.tabIndex=0;
const audioButton=document.getElementById('rhy-audio');
audioButton.addEventListener('click',async()=>{
  if(!ac)return;
  audioButton.disabled=true;
  try{if(ac.state==='running'){await flushRunning();await ac.suspend();audioButton.textContent='Resume audio';updateRhythmerMeter(0);}else{await ac.resume();audioButton.textContent='Pause audio';}}catch(error){saveFailure(error);}finally{audioButton.disabled=false;}
});
window.addEventListener('pagehide',()=>{if(ac)ac.close().catch(()=>{});});

// The clock is visible so a slower browser can trade emulation headroom for load.
const clockLabel = mhzSel.parentElement;
clockLabel.classList.add('rhy-clock');
document.querySelector('.rhy-controls').append(clockLabel);

function prepareRhythmerExport(record) {
  const text=exportSave(record);
  if(window.parent!==window){
    window.parent.postMessage({source:'rhythmer-emulator',type:'backup',hash:record.hash,text},location.origin);
    saveMessage('Backup prepared. Use Download backup below the emulator.');
  }else{
    const previous=document.getElementById('rhy-ready-download');if(previous){URL.revokeObjectURL(previous.href);previous.remove();}
    const link=document.createElement('a');link.id='rhy-ready-download';link.href=URL.createObjectURL(new Blob([text],{type:'application/json'}));link.download='Rhythmer.fm1save';link.textContent='Download backup ↓';link.className='file';document.querySelector('.save-actions').append(link);
    saveMessage('Backup ready. Click Download backup to save the file.');
  }
}
// A background emulator must not compete with an editor or another playing tab.
// Resume is deliberate so changing tabs never unexpectedly starts sound.
document.addEventListener('visibilitychange',async()=>{
  if(document.hidden&&ac?.state==='running'){
    try{await flushRunning();await ac.suspend();audioButton.textContent='Resume audio';updateRhythmerMeter(0);saveMessage('Audio paused while this tab is in the background.');}catch(e){saveFailure(e);}
  }
});
