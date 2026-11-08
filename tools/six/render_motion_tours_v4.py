#!/usr/bin/env python3
"""Four final films: real changing controls, current UI and a new manual score."""
from pathlib import Path
from functools import lru_cache
import importlib.util,subprocess,json,hashlib,sys,math
from PIL import Image,ImageDraw
import render_update_media as u
import render_manual_tour_v3 as old
m=old.m;R=Path(__file__).resolve().parents[2];Q=R/'qa/manual-motion-v4-20261108';O=R/'outputs/Rhythmer-Manual-Motion-v4';UO=R/'outputs/Rhythmer-Update-Motion-v4';FF=u.FF
u.load();clips={}
for p in Q.glob('*.rgb'):
 raw=p.read_bytes();clips[p.stem]=[Image.frombytes('RGB',(240,240),raw[i:i+172800]) for i in range(0,len(raw),172800)]
def cap(name,t,duration=None):
 fs=clips[name];speed=len(fs)/12.5/duration if duration else 1
 return fs[min(len(fs)-1,max(0,int(t*12.5*speed)))]
def lcd(im,name,t,xy,size,duration=None):
 im.paste(cap(name,t,duration).resize((size,size),Image.Resampling.NEAREST),xy);ImageDraw.Draw(im).rectangle((xy[0]-2,xy[1]-2,xy[0]+size+1,xy[1]+size+1),outline=m.LINE,width=2)
def shot(name,index):return Image.open(Q/f'{name}-{index}.png').convert('RGB')
GUIDE=[shot('guide',j) for j in range(3)];UPLOAD=[shot('upload',j) for j in [0,2,3,4]]
def browse(images,t,span):
 n=min(len(images)-1,int(t/span*len(images)));return images[n]
def badge(im,xy,s,sz=24):
 d=ImageDraw.Draw(im);w=round(d.textlength(s,font=m.font(sz,True)))+30;d.rectangle((xy[0],xy[1],xy[0]+w,xy[1]+sz+22),fill=m.INK);m.text(im,xy[0]+15,xy[1]+10,s,sz,True,m.BG)
def guidepanel(im,t,xy,width):
 a=browse(GUIDE,t,4);a=a.crop((40,90,1240,720));h=round(a.height*width/a.width);im.paste(a.resize((width,h),Image.Resampling.LANCZOS),xy)
 # Caption is editorial; all page pixels are from actual search/navigation.
 m.fitted(im,xy[0],xy[1]+h+32,['OPEN THE GUIDE','SEARCH: BREAK','OPEN A CHAPTER'][min(2,int(t/4*3))],width,25,True,color=m.GRAY)
def uploadpanel(im,t,xy,width,height):
 ix=min(3,int(t/6*4));a=UPLOAD[ix].crop((62,100,635,710));a.thumbnail((width,height),Image.Resampling.LANCZOS);im.paste(a,xy)
 badge(im,(xy[0],xy[1]+a.height+22),['01  CHOOSE AUDIO / SOURCE BPM','02  CONVERT TWO BARS','03  REVIEW BEFORE SAVING','04  EIGHT BEATS → 160 BPM'][ix],19)
def gen_label(u):return ['BOOM BAP','JUNGLE','DRUNKEN HH'][min(2,int(u/2.5))]
def manual_scene(t):
 if t<4:return ('READ THE MANUAL.','BEFORE YOU PLAY.','16 guided sections. Search. Follow. Make music.','A few minutes here. More possibilities later.','intro',t,4)
 if t<8:return ('FIND THE BUTTON.','MAKE THE MOVE.','Open the guide. Search a topic. Follow the controls.','Download the PDF to keep it beside your instrument.','guide',t-4,4)
 if t<12:return ('FOUR TRACKS.','READY TO START.','T1–T4 are active by default.','PLAY starts the pattern. ALGORITHM selects a track.','four-home',t-8,4)
 if t<18:return ('NEED MORE ROOM?','GLO → SYSTEM.','Turn knob 1 / TRACKS from four to six.','The setting opens a CPU notice before anything changes.','track-system',t-12,6)
 if t<24:return ('READ THE NOTICE.','THEN CONFIRM.','Busy six-track patterns may cause noise or glitches.','Mute tracks or reduce simultaneous voices / FX.','cpu-confirm',t-18,6)
 if t<28.5:return ('MAKE A LINE.','IN YOUR SCALE.','SCL → EDIT → ACID','OCT+ NEW / OCT− MUTATE. Watch the notes change.','acid-create',t-24,4.5)
 if t<36:return ('BREAK SEQUENCES.','FOR YOUR GENRE.','GEN → TARGET: BREAK → choose a genre → OCT+','New chop patterns. Same tempo, samples and other parts.','break-gen',t-28.5,7.5)
 if t<44:return ('HOLD SEQ.','CHANGE THE PATTERN.','F3 STRAIGHT / G3 MUTATE / A3 RANDOM','Release SEQ to see and play the resulting steps.','held-seq',t-36,8)
 if t<48:return ('CUT. REPEAT.','PLAY THE VARIATION.','Release SEQ → EDIT → BREAK → BRK PERF','Arm REC to write live gestures into the sequence.','break-perform',t-44,4)
 if t<53:return ('ONE PART.','YOUR DECISION.','Swap one drum sound. Generate only the chosen track.','Your other sounds and parts stay yours.','split',t-48,5)
 if t<59:return ('YOUR BREAKS.','YOUR WORKFLOW.','Choose audio → source BPM → eight beats → convert.','Compare both previews, review, then confirm and save.','upload',t-53,6)
 return ('LESS MENU HUNTING.','MORE MUSIC.','SOUND / SEQUENCE / PERFORM / SAVE','jazzyrhythmer.github.io/Rhythmer/#guide','outro',t-59,5)
def manual_l(t):
 im=Image.new('RGB',(1920,1080),m.BG);d=ImageDraw.Draw(im)
 for y in range(80,1000,28):
  for x in range(85,1840,28):d.point((x,y),fill=(26,31,27))
 m.text(im,100,65,'R / OS   RHYTHMER',26,True);m.text(im,1190,65,'v0.1.1-beta3 / THE MANUAL',24,True,m.GRAY);d.line((100,120,1820,120),fill=m.LINE,width=2)
 a,b,c,e,n,s,dur=manual_scene(t);m.fitted(im,95,210,a,900,76);m.fitted(im,95,305,b,900,76);m.fitted(im,100,490,c,880,31);m.fitted(im,100,548,e,880,29,color=m.GRAY)
 if n in ['intro','outro']:
  for i,k in enumerate(('drum','serum','phase','lofi','acid','break')):
   x=1090+(i%3)*235;y=225+(i//3)*290;im.paste(m.clip(k,t).resize((215,215),Image.Resampling.NEAREST),(x,y));m.text(im,x,y+230,k.upper(),20,True,m.GRAY)
  if n=='intro':m.text(im,100,710,'16 SECTIONS / PDF + WEB GUIDE',30,True)
 elif n=='guide':guidepanel(im,s,(1020,270),810)
 elif n=='upload':uploadpanel(im,s,(1140,160),720,700)
 elif n=='split':
  lcd(im,'drum-swap',s,(1055,190),350,5);lcd(im,'target-gen',s,(1460,190),350,5)
  m.text(im,1060,565,'KNOB 3 / SOUND',23,True,m.GRAY);m.text(im,1465,565,'KNOB 4 / TARGET',23,True,m.GRAY)
  badge(im,(1090,690),'CHANGE ONE PART. KEEP THE REST.',25)
 else:
  if n=='break-perform':im.paste(m.clip('break-perform',s).resize((720,720),Image.Resampling.NEAREST),(1100,160))
  else:lcd(im,n,s,(1100,160),720,dur)
  if n=='track-system':badge(im,(100,710),'KNOB 1  →  SIX TRACKS',30)
  if n=='cpu-confirm':badge(im,(100,710),'OCT+  ENABLE / OCT−  CANCEL' if s<4.24 else 'CONFIRMED / SIX TRACKS ACTIVE',29)
  if n=='acid-create':badge(im,(100,710),'OCT+ NEW' if s<2.96 else 'OCT− MUTATE',30)
  if n=='break-gen':badge(im,(100,710),gen_label(s)+' / TARGET: BREAK',27)
  if n=='held-seq':
   i=min(2,int(s/(8/3)));badge(im,(100,710),['F3 / STRAIGHT','G3 / MUTATE','A3 / RANDOM'][i],31)
   m.text(im,100,785,['Ordered two-bar loop','Vary chops; keep beat anchors','A different chop order'][i],28,False,m.GRAY)
 d.line((100,956,1820,956),fill=m.LINE,width=2);d.line((100,956,100+int(1720*t/64),956),fill=m.INK,width=3);m.text(im,100,995,'JAZZY RHYTHMER / BUILT ON JIANT',20,True,m.GRAY);m.text(im,1230,995,'HOST-RENDERED FIRMWARE UI',20,True,m.GRAY)
 return im
def manual_p(t):
 im=m.background(t);d=ImageDraw.Draw(im);a,b,c,e,n,s,dur=manual_scene(t)
 m.heading(im,a,b,'THE RHYTHMER MANUAL / FOLLOW THE CONTROLS')
 if n in ['intro','outro']:
  m.glyphs(im,760,t);m.text(im,86,1040,'16 GUIDED SECTIONS',64);m.text(im,86,1170,'READ. TRY. MAKE IT YOURS.',35,True,m.GRAY)
  m.lines(im,['jazzyrhythmer.github.io','/Rhythmer/#guide'],1370,38)
 elif n=='guide':guidepanel(im,s,(86,740),908)
 elif n=='upload':uploadpanel(im,s,(190,600),700,770)
 elif n=='split':
  lcd(im,'drum-swap',s,(86,690),440,5);lcd(im,'target-gen',s,(554,690),440,5);m.text(im,86,1170,'KNOB 3 / SOUND',25,True,m.GRAY);m.text(im,554,1170,'KNOB 4 / TARGET',25,True,m.GRAY)
 else:
  size=700 if n=='held-seq' else 760;xy=((1080-size)//2,610)
  if n=='break-perform':m.screen(im,'break-perform',xy[0],xy[1],size,s)
  else:lcd(im,n,s,xy,size,dur)
  if n=='track-system':badge(im,(160,1385),'KNOB 1  →  SIX TRACKS',27)
  if n=='cpu-confirm':badge(im,(160,1385),'OCT+ ENABLE / OCT− CANCEL' if s<4.24 else 'SIX TRACKS ACTIVE',26)
  if n=='acid-create':badge(im,(160,1385),'OCT+ NEW' if s<2.96 else 'OCT− MUTATE',27)
  if n=='break-gen':badge(im,(160,1385),gen_label(s)+' / TARGET: BREAK',25)
  if n=='held-seq':badge(im,(190,1350),['F3 / STRAIGHT','G3 / MUTATE','A3 / RANDOM'][min(2,int(s/(8/3)))],28)
 if n not in ['intro','outro']:m.lines(im,[c,e],1485 if n!='guide' else 1370,29)
 d.rectangle((86,1655,994,1663),fill=m.BG);d.line((86,1658,86+int(908*t/64),1658),fill=m.INK,width=3)
 return im
# Update films retain their original music, with corrected, newly captured actions.
basefrm=u.frm
speed=1
def update_frm(name,t=0):
 if name in ['gen-target','gen-break']:return cap('target-gen' if name=='gen-target' else 'break-gen',t,8 if speed>1 else 10)
 return basefrm(name,t)
u.frm=update_frm
u.FEATURES[2]=('03','ONE TRACK.','NEW POSSIBILITIES.','gen-target',['Turn TARGET: T2 MIDI → T3 MIDI → T4 MIDI.','OCT+ generates only the selected part.'],'KNOB 4: TARGET / OCT+ GENERATE / OTHER TRACKS KEPT')
u.FEATURES[3]=('04','GENERATE','THE BREAKS.','gen-break',['BOOM BAP → JUNGLE → DRUNKEN HH','Choose a genre. Generate. See the different chop pattern.'],'KNOB 1: GENRE / TARGET: BREAK / OCT+ GENERATE')
def update(t,portrait):
 global speed
 speed=1.25 if portrait else 1
 im=u.frame(t,portrait)
 a,b=(28,36) if portrait else (34,44)
 if a<=t<b:
  sec=(t-a)*10/(b-a);g=['BOOM BAP','JUNGLE','DRUNKEN HH'][min(2,int(sec/3.36))]
  phase=sec%3.36;badge(im,(72,600) if portrait else (1160,105),g+(' / CHOOSE GENRE' if phase<1.44 else ' / GENERATED CHOPS'),21)
 a,b=(20,28) if portrait else (24,34)
 if a<=t<b:
  sec=(t-a)*10/(b-a);g='T2 MIDI' if sec<1.36 else 'T3 MIDI' if sec<4.56 else 'T4 MIDI'
  badge(im,(72,600) if portrait else (1160,105),'KNOB 4 / TARGET: '+g,21)
 return im
FILMS={'manual-instagram':(64,1080,1920,manual_p,O,R/'outputs/Rhythmer-Manual-Motion-v4/Rhythmer-Manual-Quiet-Circuit-120BPM.wav'),
 'manual-youtube':(64,1920,1080,manual_l,O,R/'outputs/Rhythmer-Manual-Motion-v4/Rhythmer-Manual-Quiet-Circuit-120BPM.wav'),
 'update-instagram':(52,1080,1920,lambda t:update(t,True),UO,R/'site/media/video/../updates/rhythmer-update-52s-instagram.mp4'),
 'update-youtube':(68,1920,1080,lambda t:update(t,False),UO,R/'site/media/updates/rhythmer-update-68s-youtube.mp4')}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def audio_sha(p):return hashlib.sha256(subprocess.check_output([FF,'-v','error','-i',str(p),'-map','0:a:0','-c','copy','-f','adts','-'])).hexdigest()
def encode(key):
 dur,w,h,fn,out,score=FILMS[key];path=out/f'Rhythmer-{key}-{w}x{h}-{dur}s.mp4'
 cmd=[FF,'-v','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{w}x{h}','-framerate','30','-i','-','-i',str(score),'-map','0:v:0','-map','1:a:0','-t',str(dur),'-c:v','libx264','-crf','17','-preset','fast','-threads','3','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709']
 cmd+=['-c:a','aac','-b:a','256k','-ar','48000'] if key.startswith('manual') else ['-c:a','copy']
 cmd+=['-movflags','+faststart','-metadata',f'title=Rhythmer v0.1.1-beta3 / {key} / Controls in motion','-metadata','artist=Jazzy Rhythmer',str(path)]
 with (Q/(key+'-encode.log')).open('w') as log:
  p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
  try:
   for n in range(dur*30):
    p.stdin.write(fn(n/30).tobytes())
    if n%300==0:print(key,n,'/',dur*30,flush=True)
  finally:p.stdin.close()
  if p.wait():raise RuntimeError(key+' encode failed')
 subprocess.run([FF,'-v','error','-i',str(path),'-f','null','-'],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,check=True)
 meta={'file':path.name,'duration':dur,'size':[w,h],'fps':30,'sha256':sha(path),'completeDecode':'PASS','actualUIActions':True,'hardwareFootage':False,'audio':'New original Quiet Circuit / 120 BPM' if key.startswith('manual') else 'Original update soundtrack retained','firmwareUnchanged':True}
 if key.startswith('update'):meta['originalAACBitstreamIdentical']=audio_sha(path)==audio_sha(score);assert meta['originalAACBitstreamIdentical']
 (Q/(key+'-validation.json')).write_text(json.dumps(meta,indent=2)+'\n');print('Complete',path,flush=True)
def previews():
 for key,(dur,w,h,f,out,a) in FILMS.items():
  ts=[1,5,10,15,20,23,26,30,33,37,40,43,45,50,55,57,62] if key.startswith('manual') else [1,6,16,21,23,25,27,29,31,33,35,38,41,43,48,55,66] if key.endswith('youtube') else [1,6,15,21,23,25,27,29,31,33,35,40,50]
  ts=[t for t in ts if t<dur];tw,th=(480,270) if w>h else (270,480);cols=3 if w>h else 4;board=Image.new('RGB',(cols*tw,math.ceil(len(ts)/cols)*(th+28)),m.BG)
  for j,t in enumerate(ts):
   im=f(t);im.save(Q/f'{key}-{t:02d}s.jpg',quality=94,subsampling=0);x=j%cols*tw;y=j//cols*(th+28);board.paste(im.resize((tw,th),Image.Resampling.LANCZOS),(x,y+28));m.text(board,x+8,y+5,f'{t:02d}s',16,True,m.GRAY)
  board.save(out/f'Storyboard-{key}.jpg',quality=95)
 print('Review storyboards ready',flush=True)
if __name__=='__main__':
 if len(sys.argv)>1:encode(sys.argv[1])
 else:previews()
