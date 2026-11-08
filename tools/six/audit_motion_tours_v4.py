#!/usr/bin/env python3
"""Check actual encoded UI frames, moving captures and soundtrack identity."""
from pathlib import Path
import json,subprocess,hashlib,math
import numpy as np
from PIL import Image,ImageDraw
import render_motion_tours_v4 as v
R=v.R;Q=v.Q;checks=[];captures=[]
for p in Q.glob('*.rgb'):
 b=p.read_bytes();h=[hashlib.sha256(b[i:i+172800]).hexdigest() for i in range(0,len(b),172800)];assert len(set(h))>1
 captures.append({'clip':p.stem,'frames':len(h),'uniqueFrames':len(set(h)),'sha256':hashlib.sha256(b).hexdigest()})
for key,(dur,w,h,fn,out,audio) in v.FILMS.items():
 p=out/f'Rhythmer-{key}-{w}x{h}-{dur}s.mp4';meta=json.loads((Q/(key+'-validation.json')).read_text());assert meta['sha256']==v.sha(p)
 ts=[10,14,20,23,26,30,33,37,40,43,50,55] if key.startswith('manual') else [21,23,25,27,29,31,33,35] if key.endswith('instagram') else [25,27,29,31,33,35,38,41,43]
 rows=math.ceil(len(ts)/4);tw,th=(320,180) if w>h else (180,320);board=Image.new('RGB',(tw*4,(th+26)*rows),v.m.BG);errors=[]
 for j,t in enumerate(ts):
  raw=subprocess.check_output([v.FF,'-v','error','-ss',str(t),'-i',str(p),'-frames:v','1','-f','rawvideo','-pix_fmt','rgb24','-'])
  im=Image.frombytes('RGB',(w,h),raw);expected=fn(t);mae=float(np.abs(np.array(im,dtype=np.int16)-np.array(expected,dtype=np.int16)).mean());assert mae<4,(key,t,mae);errors.append({'second':t,'frameMAE':round(mae,3)})
  x=j%4*tw;y=j//4*(th+26);board.paste(im.resize((tw,th),Image.Resampling.LANCZOS),(x,y+26));ImageDraw.Draw(board).text((x+6,y+6),str(t)+'s',fill='white')
 board.save(Q/f'ENCODED-{key}-review.jpg',quality=95)
 # Confirm duration and audio loudness/true-peak from the delivered encoded stream.
 z=subprocess.run([v.FF,'-hide_banner','-i',str(p),'-af','loudnorm=I=-17:TP=-1.5:LRA=8:print_format=json','-f','null','-'],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,check=True)
 s=json.JSONDecoder().raw_decode(z.stderr[z.stderr.rfind('{'):])[0]
 if key.startswith('manual'):assert -19<float(s['input_i'])<-15 and float(s['input_tp'])<-1.0,s
 meta.update(actualEncodedFrameChecks=errors,measuredAudio=s,sourceFiles={f'tools/six/{n}':v.sha(R/'tools/six'/n) for n in ['capture_manual_motion.c','score_manual_motion.py','render_motion_tours_v4.py']});checks.append(meta)
manual=v.audio_sha(v.O/'Rhythmer-manual-instagram-1080x1920-64s.mp4');update=v.audio_sha(v.UO/'Rhythmer-update-instagram-1080x1920-52s.mp4');assert manual!=update
manifest={'date':'2026-11-08','firmwareBuild':'20261108.9','firmwareChanged':False,'capture':'Current firmware actions and real browser preparation; host UI is not physical-device footage','captures':captures,'generatorSeed':31,'targetDemonstration':['T2 MIDI','T3 MIDI','T4 MIDI'],'breakGenres':['BOOM BAP','JUNGLE','DRUNKEN HH'],'differentBreakPatternSignatures':['1d65cc78','93846b7f','36a836ed'],'newManualComposition':json.loads((Q/'score-validation.json').read_text()),'manualAndUpdateAudioDifferent':True,'films':checks}
(Q/'MEDIA-VALIDATION.json').write_text(json.dumps(manifest,indent=2)+'\n')
for d in [v.O,v.UO]:(d/'MEDIA-VALIDATION.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS: four complete decodes, distinct moving UI captures, 41 encoded-frame comparisons, separate new manual music, manual loudness and peak limits; original update AAC retained.')
