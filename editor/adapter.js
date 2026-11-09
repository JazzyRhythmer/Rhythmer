// Rhythmer-specific UI over the original JIANT editor and protocol.
$('mode-note').textContent=WORKSPACE?'Edit actual Rhythmer project data locally. No audio or hardware connection. Use PLAY to hear the firmware.':'Connect an FM-1 running this Rhythmer build. Sound edits affect the device; Save and Restore write its musical storage. Close other MIDI tools first.';
$('workspace-status').textContent=WORKSPACE?'Opening browser workspace…':'USB disconnected';
$('midiports').hidden=WORKSPACE;
$('mode-'+(WORKSPACE?'browser':'usb')).setAttribute('aria-current','page');
var machine=el('div',{id:'machine-tools',class:'group'});
$('p-sound').prepend(machine);
const lfoHelp=el('div',{class:'group',id:'dual-lfo-help'});
lfoHelp.append(el('h2',{text:'TWO LFOS / PER TRACK'}),el('p',{class:'dim',text:'Each of T1–T6 has independent LFO 1 and LFO 2. Each has a DEST page for PITCH, FILTER, SHAPE and AMP, separate from the four MOD slots. Set RATE, WAVE, MODE, PHASE and FADE in the LFO panels. MOD sources list LFO 1 then LFO 2. Save the sound or project to keep the settings.'}));
const lfoLinks=el('div',{class:'bar'});
for(const title of ['LFO 1','LFO 2','LFO 2 DEST','MOD'])lfoLinks.append(el('button',{text:'OPEN '+title,onclick:()=>{
  const heading=[...document.querySelectorAll('#groups h2,#groups h3')].find(h=>h.textContent.trim()===title);
  heading?.scrollIntoView({block:'start',behavior:'smooth'});
}}));
lfoHelp.append(lfoLinks);
$('p-sound').prepend(lfoHelp);
// Four independent destination depths stored in Rhythmer's reclaimed slots.
// Raw parameter IDs remain stable; the custom rows show decoded musical depths.
function lfo2DestRow(k){
  if(!dev.info.lfo2Dest)return null;
  const ids=[65,68,72,80],names=['PITCH','FILTER','SHAPE','AMP'];
  const input=el('input',{type:'range',min:k===3?0:-64,max:k===3?127:63,step:1,'aria-label':'LFO 2 '+names[k]});
  const out=el('span',{class:'v'}),row=el('div',{class:'row'},el('span',{text:names[k]}),input,out);
  row.dataset.lfo2depth=k;
  input.addEventListener('change',()=>op(async()=>{
    const d=dev,track=d.sel??0,value=+input.value;
    if(dev!==d||(d.sel??0)!==track)throw Error('Track changed; read the sound again');
    await rq(req.set(2,k,value));
    d.dump=await getDump(d);refreshAll();renderRhythmer();
  }));
  return row;
}
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
  document.querySelectorAll('[data-lfo2depth]').forEach(row=>{
    const k=+row.dataset.lfo2depth,ids=[65,68,72,80],raw=dev.dump.p;
    const v=raw[80]<=63?raw[ids[k]]+(k===3?64:-64):0;
    const input=row.querySelector('input');if(input!==document.activeElement)input.value=v;
    row.querySelector('.v').textContent=Math.round(v*100/(k===3?127:64))+' %';
  });
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
  const projectSignature=workspace?[workspace.current(),workspace.section(),...[0,1,2,3].map(n=>workspace.mask(n))].join(':'):'usb';
  if(projectsBox.dataset.signature===projectSignature)return;projectsBox.dataset.signature=projectSignature;
  projectsBox.replaceChildren(el('h2',{text:'PROJECT BANK / 01—04'}));
  if(WORKSPACE&&workspace){
    const current=workspace.current(),section=workspace.section(),grid=el('div',{class:'rhy-project-grid'});
    for(let n=0;n<4;n++){
      const mask=workspace.mask(n),card=el('article',{class:'rhy-project-card'});
      card.append(el('strong',{text:`PROJECT 0${n+1}${current===n?' / CURRENT':''}`}),el('p',{text:['A','B','C','D'].map((x,i)=>x+((mask>>i)&1?' ●':' ○')).join('  ')}));
      card.append(el('button',{text:current===n?'SELECTED':'OPEN PROJECT','aria-label':`Open project ${n+1}`,disabled:current===n,onclick:()=>op(async()=>{
        const dialog=el('dialog',{class:'rhy-project-confirm'}),form=el('form',{method:'dialog'}),select=el('select',{'aria-label':'Save current music to section'});
        for(let k=0;k<4;k++)select.append(el('option',{value:k,text:'SECTION '+String.fromCharCode(65+k)+((workspace.mask(current)>>k)&1?' / REPLACE':' / EMPTY')}));select.value=section<4?section:0;
        form.append(el('h2',{text:`Save & open project 0${n+1}`}),el('p',{text:`Save the current music into project 0${current+1} before switching. The selected section is replaced. The other sections are kept.`}),select,el('p',{class:'dim',text:mask?'The destination opens its saved section A when available; otherwise it keeps the current music.':'The destination is empty. Like JIANT, it starts with a copy of the current music; save A–D there to keep it.'}),el('div',{class:'bar'},el('button',{value:'cancel',text:'CANCEL'}),el('button',{value:'save',text:'SAVE & SWITCH'})));
        dialog.append(form);document.body.append(dialog);dialog.showModal();await new Promise(r=>dialog.addEventListener('close',r,{once:true}));const ok=dialog.returnValue==='save',to=+select.value;dialog.remove();if(!ok)return;
        await workspace.project(n,to);await load(dev);
      })}));grid.append(card);
    }
    const row=el('div',{class:'bar'}),saveTo=el('select',{'aria-label':'Save current music to project section'});
    for(let k=0;k<4;k++)saveTo.append(el('option',{value:k,text:'SECTION '+String.fromCharCode(65+k)+((workspace.mask(current)>>k)&1?' / REPLACE':' / EMPTY')}));saveTo.value=section<4?section:0;
    row.append(saveTo,el('button',{text:'SAVE CURRENT MUSIC',onclick:()=>op(async()=>{const n=+saveTo.value;if((workspace.mask(current)>>n)&1&&!confirm('Replace section '+String.fromCharCode(65+n)+' with the current music?'))return;workspace.saveSection(n);await load(dev);})}),el('button',{text:'BACKUP ALL 4 PROJECTS ↓',onclick:()=>op(async()=>{download('Rhythmer-all-projects.json',workspace.snapshot());sayK('backupDone');})}),el('button',{text:'SAVE FOR PLAY + AUDIO ↓',onclick:()=>op(async()=>{download('Rhythmer-web-workspace.fm1save',workspace.emulatorSave());sayK('backupDone');})}));
    projectsBox.append(grid,row,el('p',{class:'dim',text:'Browser autosave recovers your current edits and all four projects. A–D are explicit musical snapshots: save them before loading another section. Musical archives include all projects, sounds and settings; sample audio is separate. SAVE FOR PLAY includes current edits and both Break samples. Download backups before clearing browser data.'}));
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
