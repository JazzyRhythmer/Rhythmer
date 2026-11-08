#!/usr/bin/env python3
"""Rhythmer update press pack: real UI, editable motion source, original score."""
from pathlib import Path
import sys,math,json,hashlib,subprocess,shutil,textwrap,os
from functools import lru_cache
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageChops
R=Path(__file__).resolve().parents[2];O=Path(os.environ.get('RHYTHMER_UPDATE_OUTPUT',R/'outputs/Rhythmer-update-2026-11-07-v2'));Q=R/'qa/update-media-20261107';LOG=R/'qa/update-media-20261107-v2'
FF='/Volumes/SSD/Development/Toolchains/rhythmer-motion/imageio_ffmpeg/binaries/ffmpeg-macos-aarch64-v7.1'
FONT=R/'rhythmer/assets/fonts';INK=(238,238,226);BG=(10,12,12);GRAY=(157,163,158);LINE=(55,61,58)
@lru_cache(None)
def font(size,mono=False):
 f=ImageFont.truetype(str(FONT/('IBMPlexMono-SemiBold.ttf' if mono else 'InterTight[wght].ttf')),size)
 if not mono:f.set_variation_by_axes([780])
 return f
def text(im,xy,s,size=32,mono=False,color=INK):ImageDraw.Draw(im).text(xy,s,font=font(size,mono),fill=color,anchor='lt')
def line(im,xy,color=LINE,width=2):ImageDraw.Draw(im).line(xy,fill=color,width=width)
def fit(im,xy,s,width,size,mono=False,color=INK):
 while ImageDraw.Draw(im).textlength(s,font=font(size,mono))>width:size-=1
 text(im,xy,s,size,mono,color)
def bg(w,h):
 im=Image.new('RGB',(w,h),BG);d=ImageDraw.Draw(im)
 for y in range(40,h,26):
  for x in range(40,w,26):d.point((x,y),fill=(29,34,31))
 return im
CLIPS={}
def load():
 for name in ['drum-sound','gen-target','gen-break','gen-styles']:
  p=Q/'native'/f'{name}.webp';a=Image.open(p);CLIPS[name]=[]
  for i in range(a.n_frames):a.seek(i);CLIPS[name].append(a.convert('RGB'))
 for name in ['drum','serum','phase','lofi','analog','fm6','wheel','fmstat','swavsaw','sid','acid','break','acid-generate','break-perform']:
  a=Image.open(R/'site/media/motion'/f'{name}.webp');CLIPS[name]=[]
  for i in range(a.n_frames):a.seek(i);CLIPS[name].append(a.convert('RGB'))
def frm(name,t=0):
 a=CLIPS[name];return a[int(t*12.5)%len(a)]
def screen(im,name,xy,size,t=0):
 im.paste(frm(name,t).resize((size,size),Image.Resampling.NEAREST),xy)
 ImageDraw.Draw(im).rectangle((xy[0]-2,xy[1]-2,xy[0]+size+1,xy[1]+size+1),outline=LINE,width=2)
def glyphs(im,x,y,w,h,t=0):
 for j,n in enumerate(['drum','serum','phase','lofi','acid','break']):
  raw=frm(n,t+j*.13).crop((15,101,225,191));ww=int(w/6*.82);hh=round(ww*90/210)
  raw=raw.resize((ww,hh),Image.Resampling.NEAREST);im.paste(raw,(int(x+j*w/6),y))
  text(im,(int(x+j*w/6),y+hh+12),n.upper(),max(13,int(w/75)),True,GRAY)
UPLOAD=Image.open(Q/'editor-upload.jpg').convert('RGB').crop((60,676,630,1668))
PROJECT=Image.open(R/'qa/editor-gen-20261107/project-bank.png').convert('RGB')
def save(im,name,scale=None):
 im.save(O/(name+'.png'),optimize=True);im.save(O/(name+'.jpg'),quality=96,subsampling=0)
 if scale:im.resize(scale,Image.Resampling.LANCZOS).save(O/(name+'-upload.jpg'),quality=97,subsampling=0)
def chrome(im,note='FIRMWARE UPDATE',m=70):
 w,h=im.size;text(im,(m,m//2),'R / OS     RHYTHMER',26,True);text(im,(w-m-290,m//2),note,23,True,GRAY)
 line(im,[(m,h-80),(w-m,h-80)])
 text(im,(m,h-53),'JAZZY RHYTHMER / BUILT ON JIANT',20,True,GRAY)
 text(im,(w-m-430,h-53),'FM-1 / PUBLIC BETA',20,True,GRAY)
FEATURES=[
 ('01','CHANGE ONE SOUND.','KEEP THE KIT.','drum-sound',['Pick a lane. Browse the sound list.','Replace one drum without changing the rest.'],'EDIT > SOUND > SELECT'),
 ('02','NEW GENRES.','NEW DIRECTIONS.','gen-styles',['59 styles, with ten new profiles.','From drunken hip-hop to atmospheric breaks.'],'DRUNKEN HH / ATMOS BREAKS / AMBIENT IDM'),
 ('03','ONE TRACK.','NEW POSSIBILITIES.','gen-target',['Choose T1-T6 MIDI as the GEN target.','Keep your sounds, tempo and other tracks.'],'HOME + SELECT > GEN > TARGET > OCT+'),
 ('04','GENERATE','THE BREAKS.','gen-break',['Choose BREAK. GEN finds your Break track.','Genre-aware chops, groove and jungle answers.'],'HIP-HOP SPACE / DRUNKEN TIMING / JUNGLE ROLLS'),
 ('05','BRING YOUR','OWN BREAKS.','upload',['Upload a loop in the web editor.','Set its BPM. Select two bars. Preview and save.'],'SAMPLES > SOURCE BPM > TWO BARS > CONVERT')]
def posters():
 # Hero has a drum selector first and large type that survives thumbnail size.
 im=bg(3840,2160);chrome(im,m=140)
 text(im,(140,220),'RHYTHMER',265)
 text(im,(145,535),'BUILD YOUR KIT.',154)
 text(im,(145,710),'KEEP YOUR GROOVE.',154)
 text(im,(150,940),'NEW GENRES / TARGET GEN / BREAK GEN',47,True,GRAY)
 screen(im,'drum-sound',(2300,270),1220,.2)
 text(im,(2310,1540),'INDIVIDUAL DRUM SOUND SELECTION',34,True)
 line(im,[(145,1120),(2160,1120)])
 text(im,(150,1190),'+ BRING YOUR OWN BREAKS',64)
 text(im,(150,1290),'WEB EDITOR / TWO-BAR SAMPLE UPLOAD',34,True,GRAY)
 glyphs(im,150,1510,1930,240,.7)
 text(im,(150,1910),'jazzyrhythmer.github.io/Rhythmer',42,True)
 save(im,'YouTube-Thumbnail-3840x2160',(1280,720))
 # Instagram cover, followed by five clear standalone feature cards.
 im=bg(2160,2700);chrome(im,m=120)
 text(im,(120,210),'RHYTHMER',270)
 text(im,(125,540),'BUILD YOUR KIT.',128)
 text(im,(125,700),'KEEP YOUR GROOVE.',128)
 text(im,(125,910),'IN THIS UPDATE',36,True,GRAY)
 screen(im,'drum-sound',(125,1040),1000,.2)
 for j,(a,b) in enumerate([('01','SWAP ONE DRUM'),('02','EXPLORE 59 STYLES'),('03','GENERATE ONE TRACK'),('04','GENERATE BREAK CHOPS'),('05','UPLOAD YOUR BREAKS')]):
  text(im,(1220,1100+j*175),a,32,True,GRAY);fit(im,(1220,1150+j*175),b,780,51)
 glyphs(im,130,2170,1900,220,.8)
 text(im,(125,2500),'jazzyrhythmer.github.io/Rhythmer',38,True)
 save(im,'Instagram-01-Cover-2160x2700',(1080,1350))
 for j,(num,h1,h2,name,body,path) in enumerate(FEATURES):
  im=bg(2160,2700);chrome(im,m=120)
  text(im,(125,185),num+' / WHAT IS NEW',35,True,GRAY)
  fit(im,(120,320),h1,1920,175);fit(im,(120,530),h2,1920,175)
  if name=='upload':
   piece=UPLOAD.crop((15,130,555,970)).resize((760,1182),Image.Resampling.LANCZOS);im.paste(piece,(1270,900))
   text(im,(125,1020),'YOUR LOOP.',96);text(im,(125,1150),'YOUR MACHINE.',96)
   for k,s in enumerate(['01  CHOOSE AUDIO','02  SET SOURCE BPM','03  CROP EIGHT BEATS','04  COMPARE PREVIEWS','05  CONFIRM + SAVE']):text(im,(130,1390+k*115),s,31,True)
   text(im,(130,2040),'160 BPM / MONO / 16 kHz / PCM12',28,True,GRAY)
   text(im,(130,2110),'Classic repitch changes pitch.',31,False,GRAY)
  else:
   screen(im,name,(480,870),1200,.3)
   if name=='gen-styles':
    text(im,(125,2150),'BOUNCE HOUSE / 90S HOUSE / G-FUNK / NOCTURNAL HH',28,True,GRAY)
    text(im,(125,2200),'TRIP HOP / LOFI HOUSE / ABSTRACT IDM / AMBIENT IDM',28,True,GRAY)
    text(im,(125,2250),'ATMOS BREAKS / DRUNKEN HH',28,True,GRAY)
  for k,s in enumerate(body):text(im,(125,2330+k*67),s,43)
  fit(im,(125,2510),path,1900,29,True,GRAY)
  save(im,f'Instagram-{j+2:02d}-{name}-2160x2700',(1080,1350))
 # Individual upscaled actual LCD screens, no invention of extra detail.
 screens=O/'Screens';screens.mkdir(exist_ok=True)
 names=['drum-sound','gen-styles','gen-target','gen-break','acid-generate','break-perform','drum','serum','phase','lofi','analog','fm6','wheel','fmstat','swavsaw','sid','acid','break']
 sheet=bg(3200,math.ceil(len(names)/4)*910+220);text(sheet,(90,55),'RHYTHMER / ACTUAL FIRMWARE UI',58)
 for j,name in enumerate(names):
  f=frm(name,.3);f.resize((1920,1920),Image.Resampling.NEAREST).save(screens/(name+'-1920.png'))
  x=90+j%4*790;y=200+j//4*910;text(sheet,(x,y),name.upper().replace('-',' / '),26,True,GRAY)
  sheet.paste(f.resize((720,720),Image.Resampling.NEAREST),(x,y+58))
 save(sheet,'Major-Screens-3200px')
 # Full update overview for Reddit, firmware-first with small creator credit.
 im=bg(3200,2000);chrome(im,m=100);text(im,(100,155),'RHYTHMER / IN THIS UPDATE',135)
 for j,(num,h1,h2,name,body,path) in enumerate(FEATURES[:4]):
  x=100+j*775;text(im,(x,390),num+' / '+name.upper().replace('-',' '),25,True,GRAY)
  screen(im,name,(x,470),675,.3);fit(im,(x,1200),h1,680,55);fit(im,(x,1275),h2,680,55)
 text(im,(100,1500),'05 / BRING YOUR OWN BREAKS',90)
 text(im,(105,1650),'WEB EDITOR / UPLOAD > SET BPM > SELECT TWO BARS > PREVIEW > SAVE',38,True,GRAY)
 text(im,(105,1810),'55 HORTATOR MD SOUNDS / 19 DRUM KITS / FOUR ADDITIONAL ACID PRESETS',34,True)
 save(im,'Reddit-Update-3200x2000')

INTRO=4
NEW_GENRES=['BOUNCE HOUSE','90S HOUSE','G-FUNK','NOCTURNAL HH','TRIP HOP','LOFI HOUSE','ABSTRACT IDM','AMBIENT IDM','ATMOS BREAKS','DRUNKEN HH']
SCENES=[(0,INTRO,7)]+[(a+INTRO,b+INTRO,k) for a,b,k in [
 (0,10,0),(10,20,1),(20,30,2),(30,40,3),(40,48,5),(48,60,4),(60,64,6)]]
PS=[(0,INTRO,7)]+[(a+INTRO,b+INTRO,k) for a,b,k in [(0,8,0),(8,16,1),(16,24,2),(24,32,3),(32,44,4),(44,48,6)]]
BASE={}
def ease(t):t=max(0,min(t,1));return t*t*(3-2*t)
def frame(t,portrait=False):
 w,h=(1080,1920) if portrait else (1920,1080)
 if (w,h) not in BASE:BASE[w,h]=bg(w,h)
 im=BASE[w,h].copy();d=ImageDraw.Draw(im);m=72 if portrait else 80
 text(im,(m,95 if portrait else 40),'R / OS    RHYTHMER',26,True)
 if portrait:text(im,(m,142),'FIRMWARE UPDATE',20,True,GRAY)
 else:text(im,(1490,42),'FIRMWARE UPDATE',24,True,GRAY)
 scenes=PS if portrait else SCENES
 a,b,idx=next(s for s in scenes if s[0]<=t<s[1]);u=t-a
 dur=52 if portrait else 68
 if idx==7:
  if portrait:
   text(im,(m,420),'RHYTHMER',140)
   text(im,(m,650),'v0.1.1-beta3',96)
   text(im,(m,800),'FIRMWARE UPDATE',48,True)
   text(im,(m,900),'SOUND INTO MOTION.',25,True,GRAY)
   glyphs(im,m,1150,880,160,u)
   x0,y0,ww=m,1460,880
  else:
   text(im,(m,245),'RHYTHMER',245)
   text(im,(m+5,545),'v0.1.1-beta3',112)
   text(im,(m+5,700),'FIRMWARE UPDATE',48,True)
   text(im,(m+5,800),'SOUND INTO MOTION.',25,True,GRAY)
   glyphs(im,1250,565,580,150,u)
   x0,y0,ww=m,905,1740
  for j in range(24):
   x=x0+j*ww/24
   d.rectangle((int(x),y0,int(x+ww/28),y0+5),fill=INK if j<24*ease(u/3.5) else LINE)
 elif idx<5:
  num,h1,h2,name,body,path=FEATURES[idx]
  if portrait:
   text(im,(m,245),num+' / IN THIS UPDATE',23,True,GRAY)
   fit(im,(m,325),h1,900,97);fit(im,(m,445),h2,900,97)
   if name=='gen-styles':
    text(im,(m,600),'10 NEW PROFILES / 59 STYLES TOTAL',24,True,GRAY)
    for j,genre in enumerate(NEW_GENRES):
     fit(im,(m+(j//5)*450,665+(j%5)*59),f'{j+1:02d}  {genre}',435,28,True)
    screen(im,name,(252,1000),500,u)
   elif name=='upload':
    # Real crop, waveform and previews remain entirely visible in separate close-ups.
    crop=UPLOAD.crop((15,130,555,690)) if u<6 else UPLOAD.crop((15,430,555,970))
    scale=min(860/crop.width,860/crop.height);piece=crop.resize((round(crop.width*scale),round(crop.height*scale)),Image.Resampling.LANCZOS)
    im.paste(piece,(m,675))
    msg='SET BPM / SELECT TWO BARS' if u<6 else 'COMPARE / CONFIRM / SAVE'
    text(im,(m,610),msg,25,True,GRAY)
   else:screen(im,name,(m,660),860,u)
   for k,s in enumerate(body):fit(im,(m,1580+k*57),s,880,32)
   fit(im,(m,1720),path,870,19,True,GRAY)
  else:
   text(im,(m,220),num+' / IN THIS UPDATE',25,True,GRAY)
   fit(im,(m,320),h1,1010,108);fit(im,(m,455),h2,1010,108)
   if name=='gen-styles':
    text(im,(m,610),'10 NEW PROFILES / 59 STYLES TOTAL',24,True,GRAY)
    for j,genre in enumerate(NEW_GENRES):
     fit(im,(m+(j//5)*500,666+(j%5)*48),f'{j+1:02d}  {genre}',485,30,True)
   else:
    for k,s in enumerate(body):fit(im,(m,645+k*55),s,990,35)
   if name=='upload':
    crop=UPLOAD.crop((15,130,555,860)) if u<4 else UPLOAD.crop((15,240,555,970));piece=crop.resize((535,724),Image.Resampling.LANCZOS);im.paste(piece,(1260,160))
    text(im,(m,810),'160 BPM / TWO BARS / LOCAL CONVERSION',25,True,GRAY)
   else:screen(im,name,(1160,150),680,u)
   fit(im,(m,930),path,1740,23,True,GRAY)
 elif idx==5:
  text(im,(m,220),'MORE VOICES. MORE CHARACTER.',80)
  text(im,(m,330),'55 MD SOUNDS / 19 KITS / FOUR HORTATOR ACID PRESETS',28,True,GRAY)
  screen(im,'acid-generate',(180,420),460,u);screen(im,'break-perform',(730,420),460,u)
  glyphs(im,1290,475,520,150,u)
  text(im,(1290,700),'SEQUENCE.',44);text(im,(1290,765),'MUTATE. PERFORM.',44)
 else:
  if portrait:
   text(im,(m,460),'RHYTHMER',140);text(im,(m,650),'SOUND INTO MOTION.',69)
   glyphs(im,m,890,880,150,u)
   text(im,(m,1210),'EXPLORE THE PROJECT',29,True)
   text(im,(m,1300),'jazzyrhythmer.github.io',33,True,GRAY);text(im,(m,1350),'/Rhythmer',33,True,GRAY)
   text(im,(m,1515),'UP TO SIX ASSIGNABLE TRACKS',23,True,GRAY)
  else:
   text(im,(m,260),'RHYTHMER',245);text(im,(m+5,560),'SOUND INTO MOTION.',88)
   text(im,(m+5,750),'jazzyrhythmer.github.io/Rhythmer',39,True)
   text(im,(m+5,850),'UP TO SIX ASSIGNABLE TRACKS',24,True,GRAY)
   glyphs(im,1250,540,580,150,u)
 # Thin beat register, scan line reveal and progress strip; never full-frame strobe.
 for j in range(16):
  x=m+j*(w-2*m)/16;y=1800 if portrait else 1007
  d.rectangle((int(x),y,int(x)+max(3,int((w-2*m)/21)),y+4),fill=INK if j==int(t*135/60*4)%16 else LINE)
 footer_y=1850 if portrait else 1040
 text(im,(m,footer_y),'JAZZY RHYTHMER / BUILT ON JIANT',16 if portrait else 19,True,GRAY)
 d.line((m,1778 if portrait else 990,m+(w-2*m)*t/dur,1778 if portrait else 990),fill=INK,width=2)
 # Short clean entry fade, no clipped screen panels or zooming pixel blur.
 fade=ease(u/.24)*(1-ease((t-(dur-.45))/.45))
 if fade<1:im=Image.blend(Image.new('RGB',(w,h),BG),im,fade)
 return im

def storyboards():
 for portrait in (False,True):
  ts=[2,6,16,26,37,48,58,66] if not portrait else [2,6,14,22,30,40,50]
  tw,th=(640,360) if not portrait else (360,640)
  cols=2 if not portrait else 3;rows=math.ceil(len(ts)/cols)
  sheet=Image.new('RGB',(cols*tw,rows*(th+40)),(23,26,24))
  for j,t in enumerate(ts):
   im=frame(t,portrait);im.save(O/f'Frame-{("Reels" if portrait else "YouTube")}-{t:02d}s.png')
   x=j%cols*tw;y=j//cols*(th+40);sheet.paste(im.resize((tw,th),Image.Resampling.LANCZOS),(x,y+40));text(sheet,(x+12,y+10),f'{t:02d}s',19,True)
  save(sheet,'Storyboard-'+('Reels' if portrait else 'YouTube'))
def video(portrait=False):
 dur=52 if portrait else 68;w,h=(1080,1920) if portrait else (1920,1080);name=f'Rhythmer-Update-{("Reels-1080x1920-52s" if portrait else "YouTube-1920x1080-68s")}.mp4'
 cmd=[FF,'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{w}x{h}','-framerate','30','-i','-','-i',str(O/'Rhythmer-Update-Score-With-Intro.wav'),'-map','0:v','-map','1:a','-t',str(dur),'-c:v','libx264','-crf','17','-preset','fast','-threads','4','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-c:a','aac','-b:a','256k','-ar','48000','-af',f'afade=t=out:st={dur-1.4}:d=1.4','-movflags','+faststart','-metadata','title=Rhythmer v0.1.1-beta3 / Firmware Update','-metadata','artist=Jazzy Rhythmer',str(O/name)]
 with (LOG/('encode-reels.log' if portrait else 'encode-youtube.log')).open('w') as log:
  p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
  try:
   for i in range(dur*30):
    p.stdin.write(frame(i/30,portrait).tobytes())
    if i%300==0:print(name,i,'/',dur*30,flush=True)
  finally:p.stdin.close()
  if p.wait():raise RuntimeError('encode failed')
 print('COMPLETE',name,flush=True)

if __name__=='__main__':
 assert O.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
 load()
 if '--video-landscape' in sys.argv:video(False)
 elif '--video-portrait' in sys.argv:video(True)
 else:posters();storyboards()
