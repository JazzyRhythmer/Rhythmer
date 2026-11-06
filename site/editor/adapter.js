// Rhythmer-specific UI over the original JIANT editor and protocol.
$('mode-note').textContent=WORKSPACE?'Edit actual Rhythmer project data locally. No audio or hardware connection. Use PLAY to hear the firmware.':'Connect an FM-1 running this Rhythmer build. Sound edits affect the device; Save and Restore write its musical storage. Close other MIDI tools first.';
$('workspace-status').textContent=WORKSPACE?'Opening browser workspace…':'USB disconnected';
$('midiports').hidden=WORKSPACE;
$('mode-'+(WORKSPACE?'browser':'usb')).setAttribute('aria-current','page');
var machine=el('div',{id:'machine-tools',class:'group'});
$('p-sound').prepend(machine);
var projectsBox=el('div',{id:'project-bank',class:'group'});
$('p-projects').prepend(projectsBox);
const disconnectUpstream=disconnect;
disconnect=function(reason){disconnectUpstream(reason);if(workspace){workspace.close();workspace=null;access=null;}};
window.addEventListener('message',event=>{if(event.origin===location.origin&&event.source===parent&&event.data?.source==='rhythmer-shell'&&event.data.type==='release-editor'){wantConnected=false;disconnect();$('workspace-status').textContent='Workspace released to the full editor. Close that view before connecting here again.';}});
const oldRenderLive=renderLive;
renderLive=function(){oldRenderLive();renderRhythmer();};
const oldMixer=readMixer;
readMixer=async function(d=dev){await oldMixer(d);if(d===dev)renderRhythmer();};
const oldRenderPanels=renderPanels;
renderPanels=function(){oldRenderPanels();if(machine)renderRhythmer();};
function renderRhythmer(){
  if(!dev?.dump)return;
  const e=dev.dump.engine;
  const help=document.querySelector('[data-t=notesHelp]');if(help)help.textContent=e===15?'C1–C16 chops · R1–R16 repeats/up-down · A / B sample · RET return. Up to four commands per step; NOTE / TIE / REST control playback.':t('notesHelp');
  const owner=dev.mix?.tracks?.findIndex(t=>t.engine===15)??-1;
  const allowed=owner<0||owner===(dev.sel??0);
  $('engine').querySelectorAll('option').forEach(o=>{if(+o.value===15)o.hidden=o.disabled=!allowed;});
  machine.hidden=e!==15&&e!==16;
  const signature=[e,dev.sel,dev.built].join(':');
  if(machine.dataset.engine!==signature){
    machine.dataset.engine=signature;machine.replaceChildren();
    machine.append(el('h2',{text:e===16?'ACID / X0X GENERATOR':'BREAK / SEQUENCED PERFORMANCE'}));
    if(e===16){
      machine.append(el('p',{class:'dim',text:'NEW and MUTATE use the original X0X generator in the browser workspace. ROOT / SCALE always follow SCL; an unset scale defaults to MIN. Save to A–D when ready.'}));
      const row=el('div',{class:'bar'});
      for(const [title,mutate] of [['NEW 303 LINE',false],['MUTATE',true]])row.append(el('button',{text:title,disabled:!WORKSPACE,onclick:()=>op(async()=>{workspace.generate(dev.sel??0,mutate);await afterSoundChange();sayK('ready');})}));
      machine.append(row);
      if(!WORKSPACE)machine.append(el('p',{class:'dim',text:'On the FM-1: EDIT → ACID GEN → OCT+ for NEW / OCT− for MUTATE. Generator knobs below edit the same settings.'}));
      const grid=el('div',{class:'rhy-machine-knobs'});
      for(let k=0;k<8;k++){
        const pid=[83,84,85,86,25,26,89,90][k];const d=dev.pdesc[pid];if(!d)continue;
        const input=d.fmt===F.ENUM||pid===25?el('select',{'aria-label':d.label},...(pid===25?NOTE_NAMES:d.names).map((name,i)=>el('option',{value:i+d.min,text:name}))):el('input',{type:'range',min:d.min,max:d.max,'aria-label':d.label}); input.value=dev.dump.p[pid];input.dataset.pid=pid;
        const out=el('output',{text:fmtValue(d,dev.dump.p[pid])[0]});
        input.addEventListener('change',()=>op(async()=>{await rq(req.set(0,pid,+input.value));dev.dump=await getDump();out.textContent=fmtValue(d,dev.dump.p[pid])[0];}));
        grid.append(el('label',{},el('span',{text:d.label}),input,out));
      }
      machine.append(grid);
    }else{
      machine.append(el('p',{class:'dim',text:'On the FM-1, hold SEQ for STRAIGHT, MUTATE, RANDOM, fills and saved automatic variations. Generated events are visible here and included in projects/archives. SEQ owns playback: C1–C16 chops · R1–R16 recorded repeats / up-down / stretch · A / B sample changes · RET straight return. Enter up to four commands per step. NOTE triggers; TIE holds; REST releases. EDIT sound controls and live performance layer onto that sequence.'}));
      const source=el('select',{'aria-label':'Break sequence source'},el('option',{value:0,text:'LOOP · Amenity variations, gated by SEQ'}),el('option',{value:1,text:'CHOP · exact SEQ chops'}));source.value=dev.dump.p[83];source.addEventListener('change',()=>op(async()=>{await rq(req.set(0,83,+source.value));dev.dump=await getDump();}));machine.append(el('label',{},el('span',{text:'SOURCE '}),source));
      const row=el('div',{class:'bar'});
      row.append(el('button',{text:'OPEN BREAK SEQUENCE',onclick:()=>showTab('sequencer')}));
      machine.append(row);
    }
  }
  machine.querySelectorAll('[data-pid]').forEach(input=>{if(input!==document.activeElement)input.value=dev.dump.p[+input.dataset.pid];});
  projectsBox.replaceChildren(el('h2',{text:'PROJECT BANK / 01—04'}));
  if(WORKSPACE&&workspace){
    const row=el('div',{class:'bar'});
    for(let n=0;n<4;n++)row.append(el('button',{text:`0${n+1}${workspace.mask(n)?' ●':''}`,'aria-label':`Project ${n+1}`,'aria-pressed':workspace.current()===n,onclick:()=>op(async()=>{if(n===workspace.current())return;if(!confirm('Switch project? Save the current section to A–D first. Unsaved runtime edits will be replaced.'))return;await workspace.project(n);await load(dev);})}));
    row.append(el('button',{text:'SAVE FOR PLAY ↓',onclick:()=>op(async()=>{const data=workspace.emulatorSave();download('Rhythmer-web-workspace.fm1save',data);sayK('backupDone');})}));
    projectsBox.append(row,el('p',{class:'dim',text:'Four real firmware project slots. Each contains A–D sections. Empty slots start from the current music; save a section to keep it. Save a section before switching projects. Browser edits autosave as a recoverable musical archive; download a backup before clearing browser data. SAVE FOR PLAY includes your current edits and both Break samples; import the .fm1save in PLAY to hear it.'}));
  }else projectsBox.append(el('p',{class:'dim',text:'Select projects 01–04 on the FM-1 HOME / project library. A–D below belong to the currently selected project. Musical archive backs up all four projects together.'}));
}
// Keep the editor quiet in an inactive tab; JIANT already pauses its polling there.
const modeLinks=document.querySelectorAll('#rhythmer-mode a');
for(const a of modeLinks)a.addEventListener('click',()=>workspace?.flush());
// Framed page height avoids a second scroll area on the main website.
let lastEditorHeight=0;
new ResizeObserver(()=>{const h=Math.ceil(document.querySelector('main').getBoundingClientRect().height)+24;if(h!==lastEditorHeight){lastEditorHeight=h;parent.postMessage({source:'rhythmer-editor',type:'height',height:h},location.origin);}}).observe(document.querySelector('main'));
if(WORKSPACE)connect();
// Keep a real download link after async MIDI/archive work, including browsers
// that require a second explicit click before saving a prepared Blob.
let preparedFileURL=null;
download=function(name,obj){
 if(preparedFileURL)URL.revokeObjectURL(preparedFileURL);
 preparedFileURL=URL.createObjectURL(new Blob([JSON.stringify(obj,null,1)],{type:'application/json'}));
 let box=document.getElementById('prepared-file');if(!box){box=el('div',{id:'prepared-file',class:'bar','aria-live':'polite'});$('rhythmer-mode').append(box);}
 box.replaceChildren(el('a',{href:preparedFileURL,download:name,text:'DOWNLOAD '+name+' ↓'}));
};
// Audio is intentionally separate from the musical archive protocol.
const samplePanel=document.getElementById('p-samples');
import('./break-samples.mjs').then(({initBreakSamples})=>initBreakSamples({
 container:samplePanel,mode:WORKSPACE?'browser':'usb',getWorkspace:()=>workspace,getDevice:()=>dev,
 request:(d,r,o)=>sessionRequest(d,r,o),
 operation:async fn=>{if(!dev)throw Error('Connect Rhythmer first');const release=beginBusy();try{return await fn();}finally{release();}},
 downloadEmulator:()=>{try{if(!workspace)throw Error('Connect the browser workspace first');download('Rhythmer-with-user-breaks.fm1save',workspace.emulatorSave());}catch(error){samplePanel.querySelector('[role="status"]').textContent=error.message;}}
})).catch(error=>{samplePanel.textContent='Audio tools unavailable: '+error.message;});
