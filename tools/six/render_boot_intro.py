#!/usr/bin/env python3
"""Rhythmer OS opening: original procedural motion and synthesized sound, no samples."""
from pathlib import Path
import argparse,math,os,subprocess,sys,wave,shutil,json
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageChops,ImageSequence
ap=argparse.ArgumentParser();ap.add_argument('--output',required=True,type=Path);ap.add_argument('--storyboard',action='store_true');ap.add_argument('--extended-machines',action='store_true');ap.add_argument('--soundtrack',type=Path,help='Reuse an existing mastered WAV without changing its samples');a=ap.parse_args()
OUT=a.output;OUT.mkdir(parents=True,exist_ok=True)
ROOT=Path(__file__).resolve().parents[2]
FONTS=Path(__file__).parent/'fonts'
if not FONTS.is_dir():FONTS=ROOT/'jiant/assets/fonts'
MOTION=Path(__file__).parent/'motion'
if not MOTION.is_dir():MOTION=ROOT/'site/media/motion'
DEMO_NAMES=['drum','serum','phase','lofi','analog','fm6','wheel','fmstat','swavsaw','sid','acid','break']
SCREENS={}
for name in ['acid-generate','break-perform']+(DEMO_NAMES if a.extended_machines else []):
 with Image.open(MOTION/(name+'.webp')) as clip:
  SCREENS[name]=[f.convert('L').resize((480,480),Image.Resampling.NEAREST) for f in ImageSequence.Iterator(clip)]
INSERT=18 if a.extended_machines else 0
W,H,FPS,DURATION=1920,1080,30,18+INSERT
rng=np.random.default_rng(748001)
def font(n,bold=False,mono=False):
 f=ImageFont.truetype(str(FONTS/('IBMPlexMono-SemiBold.ttf' if mono else 'InterTight[wght].ttf')),n)
 if not mono:f.set_variation_by_axes([900 if bold else 500])
 return f
FONT={n:font(n,bold=n>=72,mono=n<=30) for n in [20,22,24,28,30,42,54,72,100,154,300]}
def clamp(x):return max(0.,min(1.,x))
def ease(x):x=clamp(x);return 1-(1-x)**3
def smooth(x):x=clamp(x);return x*x*(3-2*x)
def opacity(im,k):return im.point(lambda p:round(p*clamp(k)))
def label(d,xy,text,size=24,ink=200,center=False):
 f=FONT[size];x,y=xy
 if center:x-=d.textlength(text,font=f)/2
 box=d.textbbox((0,0),text,font=f);d.text((int(x),int(y-box[1])),text,font=f,fill=int(ink))
def line(d,points,ink=255,width=2):d.line([(round(x),round(y)) for x,y in points],fill=int(ink),width=width)
def dots(d,x,y,w,h,spacing=22,ink=25):
 for yy in range(int(y),int(y+h),spacing):
  for xx in range(int(x),int(x+w),spacing):d.rectangle((xx,yy,xx+1,yy+1),fill=ink)
NAMES=['DRUM','SERUM','PHASE','LOFI','ACID','BREAK']
def pulse(t,offset=0):return math.exp(-((t-offset)%.375)*12)
def glyph(d,kind,x,y,w,h,t,amp=1.,ink=255):
 cy=y+h/2;p=pulse(t,kind*.04);g=.42+.58*p
 if kind==0:
  for j in range(8):
   level=(.15+.85*(math.sin(t*3.5+j*2.31)**2))*(.5+.5*p);hh=h*level*.86
   xx=x+j*w/8;d.rectangle((xx,y+h-hh,xx+w/8-10,y+h),outline=ink,width=3)
 elif kind==5:
  peak=[.94,.36,.62,.24,.82,.42,.55,.28,1,.45,.69,.31,.8,.38,.61,.25]
  pos=int(t/.09375)%16
  for j,z in enumerate(peak):
   hh=h*z*(.75+.25*p);xx=x+j*w/16
   d.rectangle((xx,cy-hh/2,xx+w/16-5,cy+hh/2),fill=ink)
   if j==pos:d.rectangle((xx-5,y-12,xx+w/16,y+h+12),outline=ink,width=2)
 else:
  for lane in [0,1,2]:
   pts=[]
   for j in range(65):
    u=j/64;q=u*2*math.pi*(1.5 if kind==4 else 1.1)+t*2.4+lane*.17
    z=math.sin(q)+.2*math.sin(3*q) if kind==1 else math.sin(q+1.2*math.sin(q)) if kind==2 else round(math.sin(q)*5)/5 if kind==3 else 2*((u*2+t*.3+lane*.035)%1)-1
    pts.append((x+j*w/64,cy+z*h*.31*(.55+.45*g)*amp+lane*3))
   if lane==1:line(d,pts,ink,2)
   else:
    for xx,yy in pts[::2]:d.rectangle((int(xx),int(yy),int(xx)+3,int(yy)+3),fill=ink)
BASE=Image.new('L',(W,H),0);bd=ImageDraw.Draw(BASE);dots(bd,72,151,1776,770,30,15)
for y in range(0,H,4):bd.line((0,y,W,y),fill=3)
MASK=Image.new('L',(W,H),0);md=ImageDraw.Draw(MASK)
f=FONT[300];bb=md.textbbox((0,0),'RHYTHMER',font=f);lw=bb[2]-bb[0];lx=(W-lw)//2;ly=398
md.text((lx,ly-bb[1]),'RHYTHMER',font=f,fill=255)
ma=np.array(MASK);yy,xx=np.where(ma[::10,::10]>140);TARGET=np.column_stack((xx*10,yy*10));order=rng.random(len(TARGET));sources=np.column_stack((W/2+np.cos(order*math.tau)*rng.uniform(250,1050,len(order)),H/2+np.sin(order*math.tau)*rng.uniform(160,510,len(order))))
PATTERNS=[[1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0],[1,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0],[1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0],[0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,1],[1,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0],[1,0,1,0,1,1,0,1,1,0,0,1,0,1,1,0]]
def chrome(t):
 im=Image.new('L',(W,H),0);d=ImageDraw.Draw(im);show=smooth((t-.3)/.9)*(1-smooth((t-(17.1+INSERT))/.55))
 if show<=0:return im
 line(d,[(72,126),(1848,126)],55,1);line(d,[(72,956),(1848,956)],55,1)
 label(d,(72,77),'R / OS',28,220);label(d,(1848-420,81),'SIX TRACKS. ONE PULSE.',22,160)
 label(d,(72,991),'BY JAZZY RHYTHMER',22,145)
 ct=t if t<6.65 else max(6.65,t-INSERT)
 stage='SIGNAL' if ct<3.2 else 'MACHINES' if ct<6.8 else 'SYNC' if ct<10.4 else 'ASSEMBLY' if ct<13.8 else 'READY'
 label(d,(1560,991),stage,22,200)
 progress=clamp(t/(15.4+INSERT));d.rectangle((72,943,72+int(1776*progress),946),fill=170)
 for j in range(6):
  k=clamp((t-3.2-j*.48)/.4);d.rectangle((1775+j*12,988,1780+j*12,1001),fill=int(30+190*k))
 return opacity(im,show)
def signal(t):
 im=Image.new('L',(W,H),0);d=ImageDraw.Draw(im);s=ease(t/1.4);cx,cy=W//2,450
 width=int(24+1180*ease((t-.4)/1.4));d.line((cx-width//2,cy,cx+width//2,cy),fill=220,width=2)
 gap=int(7+18*ease((t-.6)/.7))
 for j in range(-3,4):
  hh=int((50+20*math.cos(j+t*3))*smooth((t-.2)/.8));d.rectangle((cx+j*gap-4,cy-hh//2,cx+j*gap+4,cy+hh//2),fill=255)
 for sign in [-1,1]:
  x=cx+sign*(width//2+15);line(d,[(x,cy-32),(x,cy+32)],120,2)
 label(d,(cx,590),'RHYTHMER OS',54,255,True)
 words=['SIGNAL FOUND','INSTRUMENT LOADING','READY TO BUILD A GROOVE']
 for j,text in enumerate(words):
  at=.75+j*.5;n=int(clamp((t-at)/.35)*len(text));label(d,(cx,686+j*40),text[:n],22,100+j*25,True)
 return opacity(im,smooth(t/.35)*(1-smooth((t-2.75)/.65)))
def machines(t):
 im=Image.new('L',(W,H),0);d=ImageDraw.Draw(im);u=t-3.05
 label(d,(94,221),'THE MACHINES',72,245);label(d,(98,312),'ONE CLOCK / SIX CHANNELS',24,125)
 for j,name in enumerate(NAMES):
  reveal=ease((u-j*.38)/.7);x=90+j*293;y=405+(1-reveal)*90
  if reveal<=0:continue
  ink=int(255*reveal);d.rectangle((x,y,x+273,y+375),outline=int(70*reveal),width=2)
  label(d,(x+22,y+24),f'{j+1:02}',22,int(120*reveal));label(d,(x+22,y+69),name,30,ink)
  glyph(d,j,x+30,y+155,213,140,t,ink=ink)
  count=min(12,int(reveal*12));
  for n in range(12):d.rectangle((x+23+n*19,y+338,x+32+n*19,y+343),fill=ink if n<count else 25)
 return opacity(im,smooth(u/.55)*(1-smooth((t-6.45)/.6)))
def sequence(t):
 im=Image.new('L',(W,H),0);d=ImageDraw.Draw(im);u=t-6.65
 # Both are real animated firmware UI captures. This scene contributes no
 # machine audio: the original opening soundtrack continues uninterrupted.
 for x,title,caption,name,number in [
  (96,'ACID GENERATOR','NEW / MUTATE / SCALE / SLIDE','acid-generate','05'),
  (1000,'BREAKBEAT MANIPULATOR','CHOP / REPEAT / UP-DOWN / A-B','break-perform','06')]:
  label(d,(x,222),title,54,245);label(d,(x+2,302),caption,22,135)
  d.rectangle((x,360,x+824,898),outline=55,width=2)
  label(d,(x+24,385),number,22,110)
  frames=SCREENS[name];idx=int(max(0,u)/.08)%len(frames)
  im.paste(frames[idx],(x+172,388))
  for j in range(16):
   yy=469+j*21;active=j==int(max(0,u)/.09375)%16
   d.rectangle((x+38,yy,x+43+(14 if active else 0),yy+3),fill=220 if active else 45)
   d.rectangle((x+779-(14 if active else 0),yy,x+784,yy+3),fill=220 if active else 45)
 return opacity(im,smooth(u/.5)*(1-smooth((t-10.0)/.65)))
def showcase(t):
 im=Image.new('L',(W,H),0);u=t-6.65
 if not 0<=u<INSERT:return im
 d=ImageDraw.Draw(im);j=min(11,int(u/1.5));v=u-j*1.5
 names=['DRUM','SERUM','PHASE','LOFI','ANALOG','FM6','WHEEL','FM+STAT','SWAV-SAW','SID-6581','ACID','BREAK']
 descriptions=[('SYNTHESIZED PERCUSSION','KITS / HATS / METAL'),('WAVETABLE SYNTHESIS','MORPH / WARP / DETUNE'),('PHASE DISTORTION','ROUND KEYS / DIGITAL EDGE'),('LOW-RESOLUTION SYNTHESIS','STEPPED WAVES / GRAIN'),('CLASSIC WAVEFORMS','DETUNE / RESONANT FILTER'),('SIX-OPERATOR FM','RATIOS / FEEDBACK / NETWORKS'),('HARMONIC LAYERS','TONEWHEEL / ROTATION'),('COMPACT FM','SHIFTING RATIOS / FEEDBACK'),('LAYERED SAW WAVES','BRIGHT LEADS / WIDE TEXTURES'),('CHIP-STYLE SYNTHESIS','PULSE / SAW / TRIANGLE'),('303-STYLE BASS','ACCENTS / SLIDES / GENERATE'),('SEQUENCED BREAKBEATS','CHOP / REPEAT / SWITCH A-B')]
 label(d,(100,222),f'{j+1:02} / 12     THE MACHINES',24,130)
 label(d,(96,342),names[j],100,245)
 for k,text in enumerate(descriptions[j]):label(d,(102,487+k*44),text,24,160)
 kinds=[0,1,2,3,1,2,1,2,1,3,4,5]
 glyph(d,kinds[j],112,646,630,125,t,ink=160)
 for k in range(12):d.rectangle((104+k*57,850,142+k*57,855),fill=240 if k==j else 35)
 d.rectangle((1000,238,1824,894),outline=55,width=2)
 frames=SCREENS[DEMO_NAMES[j]];idx=min(len(frames)-1,int(v/1.5*len(frames)))
 im.paste(frames[idx],(1172,296));label(d,(1412,830),'RHYTHMER / FIRMWARE UI',22,110,True)
 return opacity(im,smooth(v/.10)*(1-smooth((v-1.40)/.10)))
def logo(t):
 im=Image.new('L',(W,H),0);d=ImageDraw.Draw(im);u=t-10.2
 if u<0:return im
 solid=smooth((t-12.9)/.7)
 for i,((tx,ty),(sx,sy)) in enumerate(zip(TARGET,sources)):
  z=ease((u-order[i]*.6)/2.6);bend=math.sin(z*math.pi)*(order[i]-.5)*180
  x=sx+(tx-sx)*z;y=sy+(ty-sy)*z+bend
  if z<1 and i%7==0:line(d,[(x,y),(x+(tx-x)*.11,y+(ty-y)*.11)],70,1)
  r=2 if z<.95 else 3
  if solid<1:d.rectangle((round(x)-r,round(y)-r,round(x)+r,round(y)+r),fill=int((100+155*z)*(1-solid)))
 solid=smooth((t-12.9)/.7)
 if solid>0:im=ImageChops.lighter(im,opacity(MASK,solid));d=ImageDraw.Draw(im)
 if t>13.25:
  k=smooth((t-13.25)/.55);label(d,(W/2,700),'SIX TRACKS. ONE PULSE.',30,int(220*k),True)
  d.rectangle((lx+lw-122,330,lx+lw,375),outline=int(180*k),width=2);label(d,(lx+lw-61,341),'OS',28,int(245*k),True)
  for j in range(6):glyph(d,j,560+j*142,823,90,44,t,ink=int(150*k))
  label(d,(W/2,265),'RHYTHMER',22,int(90*k),True)
 # A brief horizontal register shift, never a full-frame flash.
 for hit in [12.65,13.45,16.75]:
  if 0<=t-hit<.065:
   for yy,shift in [(451,23),(535,-15)]:
    strip=im.crop((0,yy,W,yy+8));im.paste(0,(0,yy,W,yy+8));im.paste(strip,(shift,yy))
 return opacity(im,smooth(u/.35)*(1-smooth((t-17.1)/.55)))
def frame(t):
 ct=t-INSERT if INSERT and t>=6.65+INSERT else t
 base=opacity(BASE,(.55+.25*pulse(t))*(1-smooth((t-(17.1+INSERT))/.6)))
 layers=[chrome(t),signal(t),machines(t)]
 if INSERT and 6.65<=t<6.65+INSERT:layers.append(showcase(t))
 else:layers.extend([sequence(ct),logo(ct)])
 for layer in layers:base=ImageChops.lighter(base,layer)
 return base.convert('RGB')
def audio(path):
 sr=48000;mix=np.zeros((int(DURATION*sr),2),dtype=np.float64);rr=np.random.default_rng(303)
 def add(at,dur,freq=440,kind='tone',level=.2,pan=0,montage=False):
  if INSERT and at>=6.65 and not montage:at+=INSERT
  n=int(dur*sr);tt=np.arange(n)/sr;env=np.minimum(tt/.004,1)*np.exp(-tt/(dur*.25));env*=np.minimum((dur-tt)/.02,1)
  if kind=='kick':phase=2*np.pi*(47*tt+95*.025*(1-np.exp(-tt/.025)));y=np.sin(phase)*np.exp(-tt*9)
  elif kind=='hat':noise=rr.standard_normal(n);y=np.r_[0,np.diff(noise)]*.35*np.exp(-tt*45)
  elif kind=='snare':noise=rr.standard_normal(n);y=(noise*.35+np.sin(2*np.pi*185*tt)*.3)*np.exp(-tt*20)
  elif kind=='acid':y=sum(np.sin(2*np.pi*freq*k*tt)*((-1)**(k+1))/k for k in range(1,11))*.5
  elif kind=='fm':y=np.sin(2*np.pi*freq*tt+2.2*np.sin(2*np.pi*freq*2*tt))*np.exp(-tt*1.3)
  else:y=np.sin(2*np.pi*freq*tt)+.15*np.sin(2*np.pi*freq*3*tt)
  y=y*env*level;i=int(at*sr);n=min(n,len(mix)-i)
  mix[i:i+n,0]+=y[:n]*math.sqrt((1-pan)/2);mix[i:i+n,1]+=y[:n]*math.sqrt((1+pan)/2)
 for at in [.3,.77,1.3,2.05]:add(at,.13,880 if at<1 else 1320,level=.09)
 for j,note in enumerate([60,63,67,70,72,79]):add(3.05+j*.38,.45,440*2**((note-69)/12),'fm',.2,(j-2.5)/5)
 if INSERT:
  for s in range(192):
   at=6.65+s*.09375;step=s%16;bar=s//16
   if step in [0,8]:add(at,.28,kind='kick',level=.32,montage=True)
   if step in [4,12]:add(at,.18,kind='snare',level=.15,montage=True)
   if step%2==0:add(at,.07,kind='hat',level=.075,pan=.2,montage=True)
   if step in [0,3,6,10]:
    note=[48,55,58,63][(bar//3)%4]+[0,3,7,10][[0,3,6,10].index(step)]
    add(at,.28,440*2**((note-69)/12),'fm',.11,pan=-.18,montage=True)
 for s in range(32):
  at=6.65+s*.09375
  if s%16 in [0,6,10]:add(at,.3,kind='kick',level=.5)
  if s%16 in [4,12]:add(at,.21,kind='snare',level=.23)
  if s%2==0:add(at,.075,kind='hat',level=.12,pan=.18)
  if PATTERNS[4][s%16]:add(at,.19,[65.406,77.782,97.999,116.541][(s//3)%4],'acid',.16,pan=-.12)
 for j in range(14):add(10.3+j*.19,.19,261.626*2**(([0,3,7,10][j%4]+12*(j//8))/12),'tone',.065,(j%3-1)*.3)
 add(13.42,.45,kind='kick',level=.54)
 for n in [48,55,63,67]:add(13.48,2.3,440*2**((n-69)/12),'fm',.16)
 add(16.8,.18,1046.5,'tone',.085)
 mix=np.tanh(mix*1.2);mix*=.8/max(np.max(np.abs(mix)),1e-9);mix[-24000:]*=np.linspace(1,0,24000)[:,None]
 with wave.open(str(path),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(sr);f.writeframes((mix*32767).astype('<i2').tobytes())
times=[1.5,7.4,13.4,19.4,26.0,32.8] if INSERT else [1.5,4.9,8.0,11.6,14.8,16.3];thumbs=[]
for t in times:
 im=frame(t);im.save(OUT/f'frame-{t:04.1f}.png');tile=Image.new('RGB',(640,390),'#141414');tile.paste(im.resize((640,360),Image.Resampling.LANCZOS),(0,30));ImageDraw.Draw(tile).text((12,8),f'{t:04.1f}s',fill='white');thumbs.append(tile)
board=Image.new('RGB',(1280,1170),'black')
for i,im in enumerate(thumbs):board.paste(im,(i%2*640,i//2*390))
board.save(OUT/'Storyboard.jpg',quality=93);frame(14.8+INSERT).save(OUT/'Rhythmer-OS-Opening-Poster.jpg',quality=96)
if a.storyboard:sys.exit(0)
ffmpeg=os.environ.get('FFMPEG_BINARY')
if not ffmpeg:
 import imageio_ffmpeg
 ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
silent=OUT/f'Rhythmer-OS-Opening-{DURATION}s-1080p-Silent.mp4';video=OUT/f'Rhythmer-OS-Opening-{DURATION}s-1080p.mp4'
cmd=[ffmpeg,'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{W}x{H}','-framerate',str(FPS),'-i','-','-an','-c:v','libx264','-preset','medium','-crf','16','-pix_fmt','yuv420p','-threads','4','-movflags','+faststart','-metadata','title=Rhythmer OS Opening','-metadata','artist=Jazzy Rhythmer','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709',str(silent)]
with (OUT/'render.log').open('wb') as log:
 p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
 try:
  for n in range(FPS*DURATION):
   p.stdin.write(frame(n/FPS).tobytes())
   if n%90==0:print(f'Rendered {n}/{FPS*DURATION} frames',flush=True)
 finally:p.stdin.close()
 if p.wait():raise RuntimeError('Video encode failed; inspect render.log')
raw=OUT/'boot-sound-unmastered.wav';sound=OUT/'Rhythmer-OS-Boot-Sound.wav'
if a.soundtrack:shutil.copy2(a.soundtrack,sound)
else:
 audio(raw)
 subprocess.run([ffmpeg,'-hide_banner','-loglevel','warning','-y','-i',str(raw),'-af','loudnorm=I=-16:TP=-1.5:LRA=11','-ar','48000','-c:a','pcm_s24le',str(sound)],check=True)
subprocess.run([ffmpeg,'-hide_banner','-loglevel','warning','-y','-i',str(silent),'-i',str(sound),'-c:v','copy','-c:a','aac','-b:a','256k','-ar','48000','-shortest','-movflags','+faststart','-metadata','title=Rhythmer OS Opening','-metadata','artist=Jazzy Rhythmer',str(video)],check=True)
(OUT/'timeline.json').write_text(json.dumps({'duration':DURATION,'fps':30,'size':[1920,1080],'scenes':{'signal':[0,3.4],'machines':[3.05,7.05],'all_model_screens':[6.65,6.65+INSERT] if INSERT else None,'acid_generator_and_breakbeat_manipulator':[6.65+INSERT,10.65+INSERT],'logo':[10.2+INSERT,17.7+INSERT],'black_tail':[17.7+INSERT,DURATION]},'audio':'Original synthesized opening soundtrack; no machine-demo audio or third-party recordings before the separate Amen mix.','source':Path(__file__).name},indent=2)+'\n')
source=OUT/'source';(source/'fonts').mkdir(parents=True,exist_ok=True);shutil.copy2(__file__,source/Path(__file__).name)
for name in ['InterTight[wght].ttf','IBMPlexMono-SemiBold.ttf','OFL.txt','OFL-IBMPlexMono.txt']:shutil.copy2(FONTS/name,source/'fonts'/name)
(source/'motion').mkdir(exist_ok=True)
for name in ['acid-generate','break-perform']+(DEMO_NAMES if INSERT else []):shutil.copy2(MOTION/(name+'.webp'),source/'motion'/(name+'.webp'))
(OUT/'README.txt').write_text(f'Rhythmer OS opening / Jazzy Rhythmer\n{DURATION} seconds / 1920 x 1080 / 30 fps / H.264 MP4\nThe main MP4 includes an original synthesized boot sound. Silent MP4 and 48 kHz / 24-bit stereo WAV are supplied separately. Last 0.3 seconds are black for an easy cut to your live jam.\nNo third-party audio samples or stock footage were used. Fonts retain their SIL OFL notices in source/fonts.\nEditable source: source/render_boot_intro.py. Requires Python 3, Pillow, NumPy and imageio-ffmpeg. Run with --output YOUR_FOLDER.\n')
print('Complete:',video,flush=True)
