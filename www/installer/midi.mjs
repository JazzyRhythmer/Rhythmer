// SPDX-License-Identifier: GPL-3.0-only
// Deliberately read-only. No firmware, project, sample or parameter writes.
export function parseInfo(data){
 const a=Array.from(data);
 if(a.length<12||a[0]!==0xf0||a[1]!==0x7d||a[2]!==0x46||a[3]!==0x4c||a[4]!==1||a.at(-1)!==0xf7)return null;
 const end=a.indexOf(0,5);if(end<6||end>30)return null;
 const name=String.fromCharCode(...a.slice(5,end));
 const engines=a[end+1];let cursor=end+6;
 if(!engines||engines>64)return null;
 for(let i=0;i<engines;i++){const z=a.indexOf(0,cursor);if(z<cursor||z-cursor>8)return null;cursor=z+1;}
 const tracks=a[cursor];
 if(!Number.isInteger(tracks)||tracks<1||tracks>16)return null;
 // This public release supplies FELUCCA_VERSION; the browser data engine
 // retains its development ABI label. Do not accept arbitrary JIANT versions.
 const knownName=['Rhythmer dev','Rhythmer v0.1.1-beta3','JIANT Rhythmer dev','JIANT v0.1.1-beta3'].includes(name);
 return {name,engines,tracks,isRhythmer:knownName&&engines===21&&tracks===6};
}
export async function getMidi(){if(!navigator.requestMIDIAccess)throw Error('Web MIDI is unavailable in this browser. Use Chrome or Edge on a computer.');return navigator.requestMIDIAccess({sysex:true});}
export async function queryInfo(access,inputID,outputID){
 const input=access.inputs.get(inputID),output=access.outputs.get(outputID);
 if(!input||!output||input.state!=='connected'||output.state!=='connected')throw Error('Check the selected input and output ports.');
 await input.open();await output.open();
 return new Promise((resolve,reject)=>{let done=false;const finish=(error,value)=>{if(done)return;done=true;clearTimeout(timeout);input.removeEventListener('midimessage',receive);error?reject(error):resolve(value);};const receive=e=>{const info=parseInfo(e.data);if(info)finish(null,info);};const timeout=setTimeout(()=>finish(Error('No INFO response. Check the selected ports and USB data cable.')),3500);input.addEventListener('midimessage',receive);try{output.send([0xf0,0x7d,0x46,0x4c,1,0xf7]);}catch(e){finish(e);}});
}
