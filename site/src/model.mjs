// SPDX-License-Identifier: GPL-3.0-only
export const MODELS = ['DRUM','WAVETABLE','FMINI-1','OBERVAVE','FM+STAT','SWAV-SAW','SID-6581','ANALOG','FM6','ACID','BREAK'];
const modelLabel = name => name === 'SERUM' ? 'WAVETABLE' : name; // legacy web draft import
const RETIRED = ['PHASE','LOFI','WHEEL'];
export const SCALES = {MIN:[0,2,3,5,7,8,10],MAJ:[0,2,4,5,7,9,11],DOR:[0,2,3,5,7,9,10],PHR:[0,1,3,5,7,8,10],LYD:[0,2,4,6,7,9,11],MIX:[0,2,4,5,7,9,10],LOC:[0,1,3,5,6,8,10],HMIN:[0,2,3,5,7,8,11],CHR:[0,1,2,3,4,5,6,7,8,9,10,11]};
export const ROOTS = ['C','C♯','D','E♭','E','F','F♯','G','A♭','A','B♭','B'];
export const kind = m => m === 'DRUM' ? 'drum' : m === 'ACID' ? 'acid' : m === 'BREAK' ? 'break' : 'synth';
const integer = (n,min,max) => Number.isInteger(n) && n>=min && n<=max;
export function blankSection(){return Array.from({length:6},(_,t)=>Array.from({length:16},(_,s)=>({on:t===0?s%4===0:t===5?s===0:t===4?[0,3,6,8,10,14].includes(s):[0,6,10].includes(s),note:48+[0,3,7,10][s%4],velocity:100,accent:false,slide:false,command:'C1',source:'A'})));}
export function makeProject(){return {format:'rhythmer-web',version:1,name:'UNTITLED',tempo:160,mode:6,root:0,scale:'MIN',tracks:['DRUM','WAVETABLE','FMINI-1','OBERVAVE','ACID','BREAK'].map(model=>({model,preset:0,level:100,mute:false,params:[64,32,64,48,96,32,0,32]})),sections:Array.from({length:4},blankSection)};}
export function availableModels(p,index){return MODELS.filter(m=>m!=='BREAK'||!p.tracks.some((t,i)=>i!==index&&t.model==='BREAK'));}
export function setModel(p,index,model){if(!integer(index,0,5)||!availableModels(p,index).includes(model))throw Error('BREAK can be assigned to one track only.');p.tracks[index].model=model;p.tracks[index].preset=0;}
export function validateProject(p){
 if(!p||p.format!=='rhythmer-web'||p.version!==1)throw Error('Import a Rhythmer web draft v1 JSON file.');
 if(typeof p.name!=='string'||p.name.length>48||!integer(p.tempo,40,240)||![4,6].includes(p.mode)||!integer(p.root,0,11)||!Object.hasOwn(SCALES,p.scale))throw Error('Invalid project settings.');
 if(!Array.isArray(p.tracks)||p.tracks.length!==6||p.tracks.filter(t=>t?.model==='BREAK').length>1)throw Error('A project needs six tracks and no more than one BREAK.');
 for(const t of p.tracks)if(!t||!MODELS.includes(modelLabel(t.model))&&!RETIRED.includes(t.model)||!integer(t.preset,0,127)||!integer(t.level,0,127)||typeof t.mute!=='boolean'||!Array.isArray(t.params)||t.params.length!==8||!t.params.every(n=>integer(n,0,127)))throw Error('Invalid track data.');
 if(!Array.isArray(p.sections)||p.sections.length!==4)throw Error('A project needs sections A–D.');
 for(const sec of p.sections){if(!Array.isArray(sec)||sec.length!==6)throw Error('Invalid section track count.');for(const steps of sec){if(!Array.isArray(steps)||steps.length!==16)throw Error('A web draft needs 16 steps per pattern.');for(const s of steps)if(!s||typeof s.on!=='boolean'||!integer(s.note,0,127)||!integer(s.velocity,1,127)||typeof s.accent!=='boolean'||typeof s.slide!=='boolean'||!['A','B'].includes(s.source)||!(/^(C|R)([1-9]|1[0-6])$|^RET$/.test(s.command)))throw Error('Invalid step data.');}}
 // Whitelist fields; never merge arbitrary imported object properties into live state.
 return {format:p.format,version:1,name:p.name,tempo:p.tempo,mode:p.mode,root:p.root,scale:p.scale,tracks:p.tracks.map(t=>({model:RETIRED.includes(t.model)?'ANALOG':modelLabel(t.model),preset:RETIRED.includes(t.model)?0:t.preset,level:t.level,mute:t.mute,params:[...t.params]})),sections:p.sections.map(sec=>sec.map(steps=>steps.map(s=>({on:s.on,note:s.note,velocity:s.velocity,accent:s.accent,slide:s.slide,command:s.command,source:s.source}))))};
}
export function randomDraft(p,track,section,rng=Math.random){const scale=SCALES[p.scale];for(let i=0;i<16;i++){const s=p.sections[section][track][i];s.on=rng()>.35;s.note=36+p.root+scale[Math.floor(rng()*scale.length)];s.accent=rng()>.7;s.slide=rng()>.8;}return p;}
