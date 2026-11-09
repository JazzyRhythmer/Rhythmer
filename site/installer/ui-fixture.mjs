// SPDX-License-Identifier: GPL-3.0-only
// Explicit test page: no physical MIDI request, no inter-tab broadcasts or real device locks.
import {FakeFM1,infoOf,infoForProduct} from './fake.mjs';
import {validateInstallBundle} from './core.mjs';
const banner=document.createElement('div');banner.className='simulation';banner.textContent='SIMULATED FM-1 · TEST UI ONLY · NO HARDWARE ACCESS';document.body.prepend(banner);
try{
 const manifest=await(await fetch('manifest.json')).json();const bytes=new Uint8Array(await(await fetch(manifest.bundle.url)).arrayBuffer());
 const b=await validateInstallBundle(bytes,manifest),q=new URLSearchParams(location.search),stock=q.get('mode')==='stock',custom=q.get('source')==='rhythmer';
 const target=stock?b.recovery:b.firmware;
 const music=Uint8Array.from({length:640},(_,i)=>(i*7)&255);
 const objs=[0,1,2,3,4,5,6,7,8,9,10,...Array.from({length:16},(_,i)=>40+i)].map(id=>[id,id<2?music:new Uint8Array(0)]);
 const device=new FakeFM1(target.image,{identity:custom?'FM-1_900':'FM-1_015',info:infoOf(),afterInfo:infoForProduct(target.product),embeddedBanks:target.embeddedBanks,after:target.product,objs,failSample:q.has('fail-sample')});
 Object.defineProperty(navigator,'requestMIDIAccess',{configurable:true,value:async()=>device.access});
 Object.defineProperty(navigator,'locks',{configurable:true,value:{request:async(name,opts,fn)=>fn({name:'simulation-only'})}});
 window.BroadcastChannel=class{postMessage(){}close(){}};
 // Show only the fixture's observed protocol counters for read-only browser verification.
 const out=document.createElement('p');out.id='simulation-observed';out.className='simulation';document.body.append(out);
 setInterval(()=>{out.textContent=`SIMULATION / upgrade commands: ${device.upgrades} / sample writes: ${device.sampleWrites||0} / audio queries: ${device.sampleOps||0} / loop mask: ${device.mask||0} / incorrect served bytes: ${device.bad}`;},150);
 await import('./ui.mjs');
}catch(e){document.querySelector('#result').textContent='SIMULATION SETUP FAILED: '+e.message;}
