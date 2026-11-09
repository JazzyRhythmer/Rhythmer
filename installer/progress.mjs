// SPDX-License-Identifier: GPL-3.0-only
// Stage-weighted overall progress; only protocol-confirmed completion reaches 100%.
const DEFINITIONS=[['prepare','Check integrated package',8],['device','Identify the FM-1',5],['backup','Prepare musical backup',7],['review','Final confirmation',5],['stage','Stage update loader',10],['loader','Reconnect update loader',5],['write','Transfer firmware package',49],['reboot','Reconnect firmware',5],['identity','Verify firmware identity',5],['final','Verify installed package',1]];
export class InstallProgress{
 constructor(now=()=>performance.now()){this.now=now;this.configure();this.reset();}
 configure({mode='rhythmer',resume=false}={}){
  const old=new Map((this.steps||[]).map(x=>[x.id,x]));
  this.steps=DEFINITIONS.filter(([id])=>!(resume&&id==='stage')).map(([id,label,weight])=>old.get(id)||{id,label,weight,status:'pending',fraction:0,bytes:0,total:0,requests:0,ranges:[]});
 }
 reset(){for(const s of this.steps)Object.assign(s,{status:'pending',fraction:0,bytes:0,total:0,requests:0,ranges:[]});this.status='idle';this.active=null;this.started=null;this.ended=null;this.updated=this.now();this.detail='No device write has started.';this.maxPercent=0;}
 begin(options){this.configure(options);this.reset();this.started=this.now();this.status='running';this.enter('prepare','Checking the integrated firmware package and recovery file.');}
 enter(id,detail=''){
  if(['failed','cancelled','complete'].includes(this.status))return;
  const s=this.steps.find(x=>x.id===id);if(!s||s.status==='done')return;
  if(this.active&&this.active!==id)this.done(this.active);
  this.active=id;s.status='active';this.status=id==='review'?'waiting':'running';this.detail=detail;this.updated=this.now();
 }
 done(id){const s=this.steps.find(x=>x.id===id);if(s&&s.status!=='skipped'){s.status='done';s.fraction=1;}this.updated=this.now();}
 skip(id){const s=this.steps.find(x=>x.id===id);if(s){s.status='skipped';s.fraction=1;}}
 bytes(id,bytes,total,detail=''){
  if(['failed','cancelled','complete'].includes(this.status))return;this.enter(id,detail);const s=this.steps.find(x=>x.id===id);if(!s||s.status==='done')return;
  s.bytes=Math.max(s.bytes,Math.min(Math.max(0,bytes),total));s.total=total;s.fraction=total>0?Math.min(.98,s.bytes/total):0;this.updated=this.now();
 }
 packet(id,{address,bytes,imageBytes},requests){
  if(['failed','cancelled','complete'].includes(this.status))return;this.enter(id,id==='stage'?'Supplying the loader data requested by the running firmware.':'Supplying package data requested by the update loader.');
  const s=this.steps.find(x=>x.id===id);if(!s||!Number.isInteger(address)||!Number.isInteger(bytes)||bytes<0||address<0||address+bytes>imageBytes)return;
  const ranges=[...s.ranges,[address,address+bytes]].sort((a,b)=>a[0]-b[0]),merged=[];
  for(const r of ranges){const prev=merged.at(-1);if(prev&&r[0]<=prev[1])prev[1]=Math.max(prev[1],r[1]);else merged.push([...r]);}
  s.ranges=merged;s.bytes=merged.reduce((n,[a,b])=>n+b-a,0);s.total=imageBytes;s.requests=requests;
  // Staging reads sparse archive headers; there is no meaningful byte total for it.
  s.fraction=id==='write'&&imageBytes?Math.min(.98,s.bytes/imageBytes):0;this.updated=this.now();
 }
 fail(message){if(this.status==='complete')return;this.status='failed';this.detail=message;if(this.active)this.steps.find(x=>x.id===this.active).status='failed';this.ended=this.now();}
 cancel(){this.status='cancelled';this.detail='Cancelled before the device write. No firmware or sample write was started.';if(this.active)this.steps.find(x=>x.id===this.active).status='cancelled';this.ended=this.now();}
 finish(detail){for(const s of this.steps){if(s.status!=='skipped')s.status='done';s.fraction=1;}this.status='complete';this.detail=detail;this.active='final';this.ended=this.now();this.maxPercent=100;}
 snapshot(){
  const weight=this.steps.reduce((n,s)=>n+s.weight,0),raw=this.steps.reduce((n,s)=>n+s.weight*s.fraction,0)/weight*100;
  this.maxPercent=Math.max(this.maxPercent,Math.min(this.status==='complete'?100:99,Math.floor(raw)));
  const current=this.steps.find(s=>s.id===this.active),now=this.ended??this.now();
  return {status:this.status,percent:this.maxPercent,active:this.active,label:current?.label||'Ready to install',detail:this.detail,elapsed:this.started===null?0:Math.max(0,now-this.started),quiet:Math.max(0,now-this.updated),bytes:current?.bytes||0,total:current?.total||0,requests:current?.requests||0,step:current?this.steps.indexOf(current)+1:0,steps:this.steps.map(({ranges,...s})=>({...s}))};
 }
}
export const duration=ms=>{const s=Math.floor(ms/1000);return `${String(Math.floor(s/60)).padStart(2,'0')}:${String(s%60).padStart(2,'0')}`;};
export function mountProgress(root){
 const model=new InstallProgress();
 root.innerHTML='<div class="transfer-top"><div><span class="eyebrow">INSTALLATION STATUS</span><h2 id="transfer-title" aria-live="polite">Ready to install</h2></div><strong id="transfer-percent">0%</strong></div><progress id="progress" max="100" value="0" aria-label="Overall installation progress"></progress><div class="transfer-meta"><span id="transfer-step">Not started</span><span>ELAPSED <b id="transfer-elapsed">00:00</b></span></div><p id="transfer-detail">No device write has started.</p><p id="transfer-bytes"></p><ol id="transfer-steps"></ol><p class="small transfer-method">Overall percentage is weighted by installation stage. Firmware byte counts are unique package bytes served to device requests, not a flash readback. Erase and reconnection stages wait for device responses. 100% appears only after the required checks finish.</p><a class="text-link" href="#log">VIEW TRANSFER LOG ↓</a>';
 let last=0,timer,renderTimer;
 function render(force=false){
  if(!force&&performance.now()-last<100){if(!renderTimer)renderTimer=setTimeout(()=>{renderTimer=null;render(true);},100);return;}last=performance.now();
  const s=model.snapshot(),find=id=>root.querySelector('#'+id);root.dataset.state=s.status;
  find('transfer-title').textContent=s.status==='failed'?`Stopped · ${s.label}`:s.status==='cancelled'?'Installation cancelled':s.status==='complete'?'Transfer complete & verified':s.status==='idle'?'Ready to install':s.status==='waiting'?'Waiting for your confirmation':s.label;
  find('transfer-percent').textContent=s.percent+'%';find('progress').value=s.percent;
  find('transfer-step').textContent=s.step?`STEP ${String(s.step).padStart(2,'0')} / ${String(s.steps.length).padStart(2,'0')}`:'Not started';find('transfer-elapsed').textContent=duration(s.elapsed);
  find('transfer-detail').textContent=s.detail;
  find('transfer-bytes').textContent=s.total?`${s.bytes.toLocaleString()} / ${s.total.toLocaleString()} ${s.active==='write'||s.active==='stage'?'package bytes served':'audio bytes sent'}${s.requests?' · '+s.requests.toLocaleString()+(s.requests===1?' request':' requests'):''}`:['running','waiting'].includes(s.status)&&s.quiet>=3000?`Waiting for ${s.status==='waiting'?'your review':'the device'} · ${duration(s.quiet)} since last update`:' ';
  const list=find('transfer-steps');if(list.children.length!==s.steps.length||Array.from(list.children).some((li,i)=>li.dataset.id!==s.steps[i].id)){list.replaceChildren(...s.steps.map(x=>{const li=document.createElement('li');li.dataset.id=x.id;const name=document.createElement('span'),status=document.createElement('b');name.textContent=x.label;li.append(name,status);return li;}));}
  for(let i=0;i<s.steps.length;i++){const step=s.steps[i],li=list.children[i];li.dataset.state=step.status;li.lastChild.textContent=({pending:'—',active:s.status==='waiting'?'REVIEW':'IN PROGRESS',done:'✓',skipped:'NOT AVAILABLE',failed:'STOPPED',cancelled:'CANCELLED'})[step.status];if(step.status==='active')li.setAttribute('aria-current','step');else li.removeAttribute('aria-current');}
  if(['complete','failed','cancelled'].includes(s.status)){clearInterval(timer);timer=null;}
 }
 return {model,render,begin(options){clearInterval(timer);model.begin(options);timer=setInterval(()=>render(true),1000);render(true);root.scrollIntoView({behavior:'smooth',block:'start'});},enter(id,detail){model.enter(id,detail);render();},done(id){model.done(id);render();},packet(id,detail,n){model.packet(id,detail,n);render();},bytes(id,n,total,detail){model.bytes(id,n,total,detail);render();},configure(o){model.configure(o);render(true);},fail(message){model.fail(message);render(true);},cancel(){model.cancel();render(true);},finish(detail){model.finish(detail);render(true);}};
}
