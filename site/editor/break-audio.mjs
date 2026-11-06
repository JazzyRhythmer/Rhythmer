// SPDX-License-Identifier: GPL-3.0-only
// Fixed device format. Conversion is browser-side; hardware receives one 80 KiB bank.
export const FRAMES=48000,RATE=16000,PAYLOAD=72000,SLOT=81920;
const ensure=(x,m)=>{if(!x)throw Error(m);};
export function crc32(bytes){let c=0xffffffff;for(const b of bytes){c^=b;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (~c)>>>0;}
export function validateBreakSlot(bytes){
 ensure(bytes instanceof Uint8Array&&bytes.length===SLOT,'Break bank must be exactly 81,920 bytes.');const h=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
 ensure(h.getUint32(0,true)===0x504d5346&&bytes[4]===12&&!bytes[5]&&bytes[6]===1&&h.getUint32(16,true)===PAYLOAD&&h.getUint32(32,true)===0&&h.getUint32(36,true)===FRAMES&&h.getUint32(48,true)===23777,'Incompatible Break bank header.');
 ensure(crc32(bytes.subarray(512,512+PAYLOAD))===h.getUint32(20,true),'Break bank payload CRC mismatch.');return bytes;
}
export function resampleTwoBars(mono,onProgress=()=>{}){
 ensure(mono.length>=8000&&mono.length<=16000*16,'Use a complete two-bar loop between 0.5 and 16 seconds.');
 const ratio=mono.length/FRAMES,cutoff=Math.min(1,1/ratio)*0.94,radius=Math.ceil(32/Math.min(1,1/ratio)),out=new Float32Array(FRAMES);
 for(let i=0;i<FRAMES;i++){
  const pos=i*ratio,lo=Math.max(0,Math.ceil(pos-radius)),hi=Math.min(mono.length-1,Math.floor(pos+radius));let sum=0,weight=0;
  for(let j=lo;j<=hi;j++){const x=j-pos,v=Math.PI*cutoff*x,sinc=Math.abs(v)<1e-9?1:Math.sin(v)/v,w=.42+.5*Math.cos(Math.PI*x/radius)+.08*Math.cos(2*Math.PI*x/radius),a=cutoff*sinc*w;sum+=mono[j]*a;weight+=a;}
  out[i]=weight?sum/weight:0;if(!(i%2048))onProgress(i/FRAMES);
 }
 onProgress(1);return out;
}
export function packBreak(pcm,name='USRBRK'){
 ensure(pcm.length===FRAMES,'Expected exactly 48,000 output frames.');
 let mean=0;for(const v of pcm){ensure(Number.isFinite(v),'Invalid PCM value.');mean+=v;}mean/=FRAMES;
 const faded=new Float32Array(FRAMES);let peak=0;
 for(let i=0;i<FRAMES;i++){const envelope=Math.min(1,i/8,(FRAMES-1-i)/48),v=(pcm[i]-mean)*envelope;faded[i]=v;peak=Math.max(peak,Math.abs(v));}
 ensure(peak>1e-7,'This file contains no usable audio.');const gain=10**(-1/20)/peak,quant=new Int16Array(FRAMES),bytes=new Uint8Array(SLOT);bytes.fill(255);bytes.fill(0,0,512);
 for(let i=0,p=512;i<FRAMES;i+=2,p+=3){const a=Math.max(-2048,Math.min(2047,Math.round(faded[i]*gain*2048))),b=Math.max(-2048,Math.min(2047,Math.round(faded[i+1]*gain*2048)));quant[i]=a*16;quant[i+1]=b*16;const x=a&4095,y=b&4095;bytes[p]=x&255;bytes[p+1]=(x>>8)|((y&15)<<4);bytes[p+2]=y>>4;}
 const h=new DataView(bytes.buffer);h.setUint32(0,0x504d5346,true);h.setUint16(4,12,true);bytes[6]=1;
 const label=String(name).normalize('NFKD').replace(/[^A-Za-z0-9 _-]/g,'').toUpperCase().slice(0,8)||'USERBRK';for(let i=0;i<label.length;i++)bytes[8+i]=label.charCodeAt(i);
 for(const [at,value] of [[16,PAYLOAD],[20,crc32(bytes.subarray(512,72512))],[32,0],[36,FRAMES],[40,0],[44,FRAMES],[48,23777]])h.setUint32(at,value,true);
 h.setInt16(52,60*16,true);bytes[58]=127;bytes[59]=1;validateBreakSlot(bytes);
 return {bytes,quant,peakDb:-1,sourceGain:gain,frames:FRAMES,rate:RATE};
}
export function slotPCM(bytes){validateBreakSlot(bytes);const pcm=new Int16Array(FRAMES);for(let p=512,i=0;i<FRAMES;p+=3,i+=2){const a=bytes[p]|(bytes[p+1]&15)<<8,b=(bytes[p+1]>>4)|bytes[p+2]<<4;pcm[i]=((a^2048)-2048)*16;pcm[i+1]=((b^2048)-2048)*16;}return pcm;}
export function previewWav(pcm){
 ensure(pcm.length===FRAMES,'Preview length mismatch.');const b=new Uint8Array(44+FRAMES*2),v=new DataView(b.buffer),txt=(o,s)=>{for(let i=0;i<s.length;i++)b[o+i]=s.charCodeAt(i);};
 txt(0,'RIFF');v.setUint32(4,b.length-8,true);txt(8,'WAVEfmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,1,true);v.setUint32(24,RATE,true);v.setUint32(28,RATE*2,true);v.setUint16(32,2,true);v.setUint16(34,16,true);txt(36,'data');v.setUint32(40,FRAMES*2,true);for(let i=0;i<FRAMES;i++)v.setInt16(44+i*2,pcm[i],true);return b;
}
export function pack7(bytes){const out=[];for(let i=0;i<bytes.length;i+=7){const c=bytes.subarray(i,i+7);out.push(c.reduce((m,v,k)=>m|((v>>7)<<k),0),...c.map(v=>v&127));}return out;}
export async function uploadBreakBank(request,slot,bytes,progress=()=>{}){
 ensure(slot===0||slot===1,'Choose audio slot A or B.');validateBreakSlot(bytes);
 const query=await request([80,[]],{retries:0});ensure(String.fromCharCode(...query.slice(0,5))==='SIX3\0','This firmware has no compatible Break upload protocol.');
 const send=async args=>{const r=await request([80,args],{timeout:10000,retries:0});ensure(r.length===4&&r[0]===args[0]&&r[1]===slot&&r[2]===0,`Audio write failed (${r[2]??'invalid reply'}). Stop playback; retry the complete bank upload, not an individual chunk.`);return r;};
 await send([1,slot]);for(let off=0;off<PAYLOAD;off+=256){const end=Math.min(off+256,PAYLOAD);await send([2,slot,off&127,(off>>7)&127,(off>>14)&127,...pack7(bytes.subarray(512+off,512+end))]);progress(end/PAYLOAD);}
 const r=await send([3,slot,...pack7(bytes.subarray(0,480))]);ensure(r[3]&(1<<slot),'Device did not validate the new audio header.');const end=await request([80,[]],{retries:0});ensure(end[5]&(1<<slot),'Uploaded audio slot was not verified.');return true;
}
