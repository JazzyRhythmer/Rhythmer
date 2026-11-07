#!/usr/bin/env python3
"""Deterministic audio format/filter and actual device bank protocol tests, no hardware."""
from pathlib import Path
import json,re,os
ROOT=Path(__file__).resolve().parents[2]
g={'__file__':str(ROOT/'site/tests/test_site.py')};exec((ROOT/'site/tests/test_site.py').read_text().split('model=(SRC/')[0],g);run=g['run']
src=ROOT/'site/editor'
for name in ['break-samples.mjs','break-convert-worker.mjs']:
    s=re.sub(r'^import .*?;\n','',(src/name).read_text(),flags=re.M).replace('export ','')
    run('async function syntax(){'+s+'\n};"OK"')
code=(src/'break-audio.mjs').read_text().replace('export ','')
result=run(code+'''
const checks=[];const check=(v,m)=>{if(!v)throw Error(m);checks.push(m);};
const sine=(n,hz,fs=16000)=>Float32Array.from({length:n},(_,i)=>Math.sin(2*Math.PI*hz*i/fs)*.5+.03);
const pcm=resampleTwoBars(sine(64000,440)),packed=packBreak(pcm,'user-a');
check(packed.bytes.length===81920&&packed.quant.length===48000,'4-second two-bar input becomes fixed 3-second slot');
check(packed.bytes.slice(72512).every(v=>v===255),'only one bounded 80KiB bank, erased trailing space');
check(packed.quant[0]===0&&packed.quant.at(-1)===0,'quantized loop endpoints are exactly zero');
const v=new DataView(packed.bytes.buffer);check(v.getUint32(16,true)===72000&&v.getUint32(36,true)===48000&&v.getUint32(48,true)===23777,'exact firmware header/payload/frame/rate fields');
check(slotPCM(packed.bytes).every((x,i)=>x===packed.quant[i]),'packed signed PCM12 round trip matches preview');
const wav=previewWav(packed.quant);check(wav.length===96044&&new DataView(wav.buffer).getUint32(24,true)===16000,'preview WAV is the real quantized 16kHz output');
let bad=packed.bytes.slice();bad[600]^=1;let rejected=false;try{validateBreakSlot(bad);}catch{rejected=true;}check(rejected,'corrupt sample CRC rejected');
rejected=false;try{packBreak(new Float32Array(48000));}catch{rejected=true;}check(rejected,'silence rejected without normalizing noise');
for(const n of [8000,48000,256000])check(resampleTwoBars(sine(n,220)).length===48000,'duration limit/target size '+n);
// At 2x playback speed, source 6kHz would alias to 4kHz without the prefilter.
const noDC=(n,hz)=>Float32Array.from({length:n},(_,i)=>Math.sin(2*Math.PI*hz*i/16000)*.5);
const rms=a=>Math.sqrt(a.slice(512,-512).reduce((n,v)=>n+v*v,0)/(a.length-1024));
const pass=rms(resampleTwoBars(noDC(96000,1000))),stop=rms(resampleTwoBars(noDC(96000,6000)));
check(20*Math.log10(stop/pass)<-45,'repitch downsampling suppresses alias band by more than 45dB');
const plan=planTwoBarLoop({frames:48000*10,rate:48000,bpm:120,start:2});
check(plan.startFrame===96000&&plan.endFrame===288000&&plan.duration===4&&Math.abs(plan.pitchSemitones-4.980449991)<.00001,'known BPM selects exact eight-beat crop and reports classic-repitch pitch change');
for(const args of [{bpm:0,start:0},{bpm:NaN,start:0},{bpm:160,start:-1},{bpm:120,start:9}]){let bad=false;try{planTwoBarLoop({frames:480000,rate:48000,...args});}catch{bad=true;}check(bad,'invalid tempo/start/short crop rejected '+JSON.stringify(args));}
const high=Float32Array.from({length:48000*4},(_,i)=>.4*Math.sin(2*Math.PI*12000*i/48000));
const low=Float32Array.from({length:48000*4},(_,i)=>.4*Math.sin(2*Math.PI*1000*i/48000));
check(20*Math.log10(rms(resampleTwoBars(high,()=>{},{sourceRate:48000}))/rms(resampleTwoBars(low,()=>{},{sourceRate:48000})))<-45,'48kHz source is filtered before repitch and decimation');
const crop=resampleTwoBars(new Float32Array([...new Float32Array(48000),...low]),()=>{},{sourceRate:48000,startFrame:48000,endFrame:240000});
check(rms(crop)>.25,'crop excludes leading silence before conversion');
check(packed.quality.peakDb<-1.99&&packed.quality.peakDb>-2.01&&!packed.quality.clipped&&!packed.quality.seam,'packed output has two dB headroom, no clipping and zero loop seam');
const quiet=packBreak(Float32Array.from({length:48000},(_,i)=>.001*Math.sin(i*.05)));check(quiet.gainDb<=12.00001,'normalization never boosts quiet material beyond twelve dB');
let nonfinite=pcm.slice();nonfinite[15]=NaN;let no=false;try{packBreak(nonfinite);}catch{no=true;}check(no,'non-finite decoded PCM rejected');
check(new DataView(monoWav(low,48000).buffer).getUint32(24,true)===48000,'original crop preview retains source sample rate');
JSON.stringify({passed:checks.length,checks,aliasRejectionDb:20*Math.log10(stop/pass),physicalHardwareTested:false});
''')
# Exercise the shipped upload transaction; failures must never report 100%.
run("var uploadDone=false,uploadError=null;async function testUpload(){let calls=0,offset=0,committed=false,progress=[];const req=async([cmd,args],options)=>{check(cmd===80&&options.retries===0,'audio writes do not retry individual packets');if(!args.length)return [83,73,88,51,0,committed?2:0];if(args[0]===1)return [1,1,0,0];if(args[0]===2){let at=args[2]+(args[3]<<7)+(args[4]<<14);if(at!==offset)throw Error('nonsequential payload');offset+=Math.min(256,72000-offset);return [2,1,0,0];}if(args[0]===3){check(offset===72000,'whole payload precedes header commit');committed=true;return [3,1,0,2];}throw Error('unexpected operation');};const receipt=await uploadBreakBank(req,1,packed.bytes,(v,p)=>{if(v===1)check(committed&&p==='verified','100 percent only after device commit and validation');progress.push(v);});check(receipt.verified&&receipt.slot===1&&progress.at(-1)===1,'completed transaction returns a verified CRC receipt');for(const failure of ['wrong-slot','commit','query']){let committed=false,complete=false,rejected=false;const bad=async([cmd,a])=>{if(!a.length)return committed&&failure==='query'?[0,0,0,0,0,2]:[83,73,88,51,0,2];if(a[0]===3){committed=true;if(failure==='commit')throw Error('commit timeout');}return [a[0],failure==='wrong-slot'?0:1,0,2];};try{await uploadBreakBank(bad,1,packed.bytes,v=>{complete||=v===1;});}catch{rejected=true;}check(rejected&&!complete,'stopped upload never completes: '+failure);}uploadDone=true;}testUpload().catch(e=>uploadError=String(e));'running';")
state=json.loads(run('JSON.stringify({done:uploadDone,error:uploadError})'));assert state['done'] and not state['error'],state
report=json.loads(run('JSON.stringify({passed:checks.length,checks,aliasRejectionDb:20*Math.log10(stop/pass),physicalHardwareTested:false})'))
# Packet options are checked above, but count the transaction-level assertions once.
report['checks']=list(dict.fromkeys(report['checks']));report['passed']=len(report['checks'])
out=Path(os.environ.get('RHYTHMER_BREAK_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/break-upload-results.json'));out.write_text(json.dumps(report,indent=2)+'\n')
print('PASS:',report['passed'],'audio crop/conversion/filter/protocol checks; alias rejection',round(report['aliasRejectionDb'],1),'dB')
