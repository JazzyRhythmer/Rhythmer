#!/usr/bin/env python3
"""Manual tour revision 3: native landscape + portrait, same original score."""
from pathlib import Path
import importlib.util,subprocess,json,hashlib,sys
from PIL import Image,ImageDraw
R=Path(__file__).resolve().parents[2];O=R/'outputs/Rhythmer-Manual-Launch-v3';Q=R/'qa/manual-tour-v3-20261108'
spec=importlib.util.spec_from_file_location('manual_v2',R/'tools/six/render_manual_short.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
m.load();m.CONTENTS=Image.open(R/'qa/public-audio-rollback-20261108.2/manual-track-mode.png').convert('RGB');m.UPLOAD=Image.open(R/'qa/update-media-20261107/editor-upload.jpg').convert('RGB').crop((55,676,640,1740));m.BREAK_KEYS=Image.open(R/'qa/manual-short-v2-20261107/break-held-seq.ppm').convert('RGB')
DURATION=64;FPS=30
shots={n:Image.open(Q/(n+'.ppm')).convert('RGB') for n in ('four-track-home','track-system','cpu-notice','six-track-home')}
def portrait(t):
 if t<8 or t>=24:
  im=m.frame(t if t<8 else t-16)
  if t<4:
   ImageDraw.Draw(im).rectangle((86,648,490,960),fill=m.BG);m.text(im,86,648,'16',280)
  if t>=59:
   ImageDraw.Draw(im).rectangle((86,955,980,1037),fill=m.BG);m.text(im,86,955,'16 SECTIONS',66)
 else:
  im=m.background(t)
  if t<12:
   m.heading(im,'FOUR TRACKS.','READY TO START.','NEW / THE DEFAULT WORKSPACE')
   im.paste(shots['four-track-home'].resize((760,760),Image.Resampling.NEAREST),(160,600))
   m.lines(im,['T1–T4 active. T5–T6 stay safely stored.','Add two more tracks when you need them.'],1435,32)
  elif t<18:
   m.heading(im,'NEED SIX TRACKS?','GLO → SYSTEM.','SETUP / KNOB 1: TRACKS')
   im.paste(shots['track-system'].resize((760,760),Image.Resampling.NEAREST),(160,600))
   ImageDraw.Draw(im).rectangle((160,726,350,860),outline=m.INK,width=4)
   m.lines(im,['Turn knob 1 from 4 TRACK to 6 TRACK.','MENU → AUDIO → knob 2 is another path.'],1435,30)
  else:
   m.heading(im,'READ THE NOTICE.','THEN CONFIRM.','SIX TRACKS / CPU HEADROOM')
   im.paste(shots['cpu-notice'].resize((760,760),Image.Resampling.NEAREST),(160,600))
   m.lines(im,['Busy patterns may cause noise or glitches.','Mute tracks or reduce voices / FX.','OCT+ ENABLE / OCT− CANCEL'],1415,31)
 d=ImageDraw.Draw(im);d.rectangle((86,1655,994,1663),fill=m.BG);d.line((86,1658,86+int(908*t/DURATION),1658),fill=m.INK,width=3)
 return im
# Native landscape layout: editorial text left, actual UI / guide right.
def scene(t):
 if t<4:return ('READ THE MANUAL.','BEFORE YOU PLAY.','16 guided sections. Search. Follow. Make music.','Read the guide before your first session.','machines')
 if t<8:return ('FIND THE BUTTON.','MAKE THE MOVE.','Search by topic. Open a chapter. Print / save PDF.','The updated guide includes the six-track setup.','guide')
 if t<12:return ('FOUR TRACKS.','READY TO START.','T1–T4 are active by default.','T5–T6 keep their sounds, notes and automation.','four-track-home')
 if t<18:return ('NEED SIX TRACKS?','GLO → SYSTEM.','Turn knob 1 / TRACKS to 6 TRACK MODE.','Alternative: MENU → AUDIO → knob 2 / TRACKS.','track-system')
 if t<24:return ('READ THE NOTICE.','THEN CONFIRM.','Busy patterns may cause noise or glitches.','Mute tracks or reduce voices / FX.','cpu-notice')
 a=t-16
 if a<12.5:return ('MAKE A LINE.','IN YOUR SCALE.','SCL → EDIT → ACID','OCT+ NEW / OCT− MUTATE','acid-generate')
 if a<20:return ('BREAK SEQUENCES.','FOR YOUR GENRE.','HOME + SELECT → GEN → TARGET: BREAK','Genre-aware chops. Same tempo, samples + other parts.','gen-break')
 if a<28:return ('SELECT BREAK.','THEN HOLD SEQ.','F3 STRAIGHT / G3 MUTATE / A3 RANDOM','Ordered two-bar loop, variation or a new chop order.','keys')
 if a<32:return ('CUT. REPEAT.','PLAY THE VARIATION.','Release SEQ → EDIT → BREAK → BRK PERF','Arm REC to write performance gestures into the sequence.','break-perform')
 if a<37:return ('ONE PART.','YOUR DECISION.','Swap one drum sound. Generate one track.','GEN ALL follows the active track mode.','drum-sound')
 if a<43:return ('YOUR BREAKS.','YOUR WORKFLOW.','SOURCE BPM → EIGHT BEATS → PREVIEW → CONFIRM','Keep four projects + A–D sections. Retain original audio.','upload')
 return ('LESS MENU HUNTING.','MORE MUSIC.','SOUND / SEQUENCE / PERFORM / SAVE','jazzyrhythmer.github.io/Rhythmer/#guide','machines')
def landscape(t):
 im=Image.new('RGB',(1920,1080),m.BG);d=ImageDraw.Draw(im)
 for y in range(80,1000,28):
  for x in range(85,1840,28):d.point((x,y),fill=(26,31,27))
 m.text(im,100,65,'R / OS   RHYTHMER',26,True);m.text(im,1190,65,'v0.1.1-beta3 / THE MANUAL',24,True,m.GRAY)
 d.line((100,120,1820,120),fill=m.LINE,width=2)
 a,b,c,e,n=scene(t);m.fitted(im,95,215,a,885,76);m.fitted(im,95,310,b,885,76)
 m.fitted(im,100,495,c,875,32);m.fitted(im,100,552,e,875,29,color=m.GRAY)
 if 12<=t<18:m.text(im,100,710,'01 SELECT SYSTEM   02 TURN TRACKS',27,True)
 if 18<=t<24:
  m.text(im,100,710,'OCT+  ENABLE SIX TRACKS',34,True);m.text(im,100,775,'OCT−  CANCEL / KEEP FOUR',30,True,m.GRAY)
 if n=='machines':
  for i,k in enumerate(('drum','wavetable','phase','lofi','acid','break')):
   x=1090+(i%3)*235;y=225+(i//3)*290
   im.paste(m.clip(k,t).resize((215,215),Image.Resampling.NEAREST),(x,y));m.text(im,x,y+230,k.upper(),20,True,m.GRAY)
 elif n=='guide':
  a=m.CONTENTS.copy();a.thumbnail((790,690),Image.Resampling.LANCZOS);im.paste(a,(1030,240))
 elif n=='upload':
  a=m.UPLOAD.copy();a.thumbnail((700,710),Image.Resampling.LANCZOS);im.paste(a,(1310,165))
 else:
  a=shots[n] if n in shots else m.BREAK_KEYS if n=='keys' else m.clip(n,t)
  im.paste(a.resize((720,720),Image.Resampling.NEAREST),(1100,160))
  if n=='track-system':d.rectangle((1100,280,1277,400),outline=m.INK,width=3)
 d.line((100,956,1820,956),fill=m.LINE,width=2);d.line((100,956,100+int(1720*t/DURATION),956),fill=m.INK,width=3)
 m.text(im,100,995,'JAZZY RHYTHMER / BUILT ON JIANT',20,True,m.GRAY);m.text(im,1230,995,'HOST-RENDERED FIRMWARE UI',20,True,m.GRAY)
 return im
def encode(kind):
 f=portrait if kind=='Reels' else landscape;w,h=(1080,1920) if kind=='Reels' else (1920,1080);path=O/f'Rhythmer-Manual-{kind}-{w}x{h}-64s.mp4'
 cmd=[m.FF,'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{w}x{h}','-framerate',str(FPS),'-i','-','-i',str(m.SCORE),'-map','0:v','-map','1:a','-t',str(DURATION),'-c:v','libx264','-crf','18','-preset','fast','-threads','4','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-c:a','aac','-b:a','256k','-ar','48000','-af','volume=0.85,afade=t=in:d=0.4,afade=t=out:st=62.7:d=1.3','-movflags','+faststart','-metadata','title=Rhythmer Manual / Four Tracks to Six',str(path)]
 with (Q/f'{kind}-encode.log').open('w') as log:
  p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
  try:
   for i in range(DURATION*FPS):
    p.stdin.write(f(i/FPS).tobytes())
    if i%240==0:print(kind,i,'/',DURATION*FPS,flush=True)
  finally:p.stdin.close()
  if p.wait():raise RuntimeError(kind+' encoding failed')
 print('Complete',path,flush=True)
if __name__=='__main__':
 assert O.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
 for kind,f in [('Reels',portrait),('YouTube',landscape)]:
  for t in [1,10,15,21,30,39,62]:f(t).save(O/f'{kind}-Preview-{t:02d}s.jpg',quality=96,subsampling=0)
 if '--video' in sys.argv:
  for kind in ['Reels','YouTube']:encode(kind)
 (O/'MEDIA-PROVENANCE.json').write_text(json.dumps({'duration':64,'fps':30,'firmwareBuild':'20261108.2','firmwareVersion':'0.1.1-beta3','revision':3,'visuals':'Actual manual screenshot, host-rendered firmware UI and editorial guidance; no physical-device footage.','audio':'Existing original Rhythmer atmospheric breakbeat score; no artist recordings.','audioSha256':hashlib.sha256(m.SCORE.read_bytes()).hexdigest(),'newScenes':{'8-12':'Four tracks by default; T5/T6 data retained','12-18':'GLO SYSTEM knob 1 / alternate MENU AUDIO knob 2','18-24':'Noise/glitch notice; mute tracks/reduce voices or FX; OCT+ enable/OCT− cancel'}},indent=2)+'\n')
