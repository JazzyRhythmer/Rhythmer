#!/usr/bin/env python3
"""Reflow the 36s Rhythmer opening for 9:16; copy original audio unchanged."""
from pathlib import Path
import argparse,base64,hashlib,json,math,os,shutil,subprocess
import numpy as np
from PIL import Image,ImageDraw,ImageFont,ImageChops,ImageSequence
import imageio_ffmpeg

ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);ap.add_argument('--source-video',type=Path,required=True);ap.add_argument('--storyboard',action='store_true');a=ap.parse_args()
R=Path(__file__).resolve().parents[2];O=a.output
assert O.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
O.mkdir(exist_ok=True,parents=True)
W,H,FPS,DURATION=1080,1920,30,36
FONTDIR=R/'rhythmer/assets/fonts';FONT={}
def font(size,bold=False,mono=False):
 key=(size,bold,mono)
 if key not in FONT:
  f=ImageFont.truetype(str(FONTDIR/('IBMPlexMono-SemiBold.ttf' if mono else 'InterTight[wght].ttf')),size)
  if not mono:f.set_variation_by_axes([900 if bold else 500])
  FONT[key]=f
 return FONT[key]
def clamp(x):return max(0.,min(1.,x))
def smooth(x):x=clamp(x);return x*x*(3-2*x)
def ease(x):return 1-(1-clamp(x))**3
def fade(im,x):return im.point(lambda p:round(p*clamp(x)))
def text(d,x,y,s,size=28,ink=220,center=False,bold=False,mono=False):
 f=font(size,bold,mono)
 if center:x-=d.textlength(s,font=f)/2
 box=d.textbbox((0,0),s,font=f);d.text((round(x),round(y-box[1])),s,font=f,fill=int(ink))
def fit(d,x,y,s,maxw,size=120,**kw):
 while d.textlength(s,font=font(size,kw.get('bold',False),kw.get('mono',False)))>maxw:size-=1
 text(d,x,y,s,size,**kw)
def layer():
 im=Image.new('L',(W,H),0);return im,ImageDraw.Draw(im)
NAMES=['DRUM','SERUM','PHASE','LOFI','ANALOG','FM6','WHEEL','FM+STAT','SWAV-SAW','SID-6581','ACID','BREAK']
IDS=['drum','serum','phase','lofi','analog','fm6','wheel','fmstat','swavsaw','sid','acid','break']
DESC=[('SYNTHESIZED PERCUSSION','KITS / HATS / METAL'),('WAVETABLE SYNTHESIS','MORPH / WARP / DETUNE'),('PHASE DISTORTION','ROUND KEYS / DIGITAL EDGE'),('LOW-RESOLUTION SYNTHESIS','STEPPED WAVES / GRAIN'),('CLASSIC WAVEFORMS','DETUNE / RESONANT FILTER'),('SIX-OPERATOR FM','RATIOS / FEEDBACK / NETWORKS'),('HARMONIC LAYERS','TONEWHEEL / ROTATION'),('COMPACT FM','SHIFTING RATIOS / FEEDBACK'),('LAYERED SAW WAVES','BRIGHT LEADS / WIDE TEXTURES'),('CHIP-STYLE SYNTHESIS','PULSE / SAW / TRIANGLE'),('303-STYLE BASS','ACCENTS / SLIDES / GENERATE'),('SEQUENCED BREAKBEATS','CHOP / REPEAT / SWITCH A-B')]
CLIPS={}
for name in IDS+['acid-generate','break-perform']:
 with Image.open(R/'site/media/motion'/f'{name}.webp') as src:CLIPS[name]=[im.convert('L').copy() for im in ImageSequence.Iterator(src)]
def graphic(im,name,x,y,w,h,t):
 frames=CLIPS[name];pic=frames[int(t/.08)%len(frames)].crop((15,101,225,191))
 scale=min(w/pic.width,h/pic.height);ww,hh=round(pic.width*scale),round(pic.height*scale)
 im.paste(pic.resize((ww,hh),Image.Resampling.NEAREST),(round(x+(w-ww)/2),round(y+(h-hh)/2)))
BASE=Image.new('L',(W,H),0);bd=ImageDraw.Draw(BASE)
for y in range(220,1600,28):
 for x in range(90,1000,28):bd.rectangle((x,y,x+1,y+1),fill=15)
for y in range(0,H,4):bd.line((0,y,W,y),fill=2)
def chrome(t):
 im,d=layer();alpha=smooth((t-.3)/.9)*(1-smooth((t-35.1)/.55))
 d.line((84,226,996,226),fill=52);d.line((84,1650,996,1650),fill=52)
 text(d,84,170,'R / OS',28,mono=True);text(d,996,174,'',22)
 text(d,775,174,'FM–1',24,150,mono=True)
 text(d,84,1690,'JAZZY RHYTHMER',22,150,mono=True)
 ct=t if t<6.65 else max(6.65,t-18)
 stage='SIGNAL' if ct<3.2 else 'MACHINES' if ct<6.8 else 'SYNC' if ct<10.4 else 'ASSEMBLY' if ct<13.8 else 'READY'
 text(d,818,1690,stage,22,200,mono=True)
 d.rectangle((84,1638,84+int(912*clamp(t/33.4)),1641),fill=180)
 return fade(im,alpha)
def signal(t):
 im,d=layer();width=round(30+760*ease((t-.4)/1.4));cy=715
 d.line((540-width//2,cy,540+width//2,cy),fill=200,width=2)
 for j in range(-3,4):
  hh=round((80+32*math.cos(j+t*3))*smooth((t-.2)/.8));xx=540+j*30
  d.rectangle((xx-5,cy-hh//2,xx+5,cy+hh//2),fill=255)
 text(d,540,900,'RHYTHMER OS',80,255,True,True)
 for j,s in enumerate(['SIGNAL FOUND','INSTRUMENT LOADING','READY TO BUILD A GROOVE']):
  n=round(clamp((t-(.75+j*.5))/.35)*len(s));text(d,540,1050+j*55,s[:n],24,110+j*25,True,mono=True)
 return fade(im,smooth(t/.35)*(1-smooth((t-2.75)/.65)))
def machines(t):
 im,d=layer();u=t-3.05
 text(d,84,300,'THE MACHINES',76,245,bold=True)
 text(d,88,410,'SOUND / PATTERN / PERFORMANCE',25,140,mono=True)
 for j,(name,title) in enumerate(zip(['drum','serum','phase','lofi','acid','break'],['DRUM','SERUM','PHASE','LOFI','ACID','BREAK'])):
  k=ease((u-j*.38)/.7)
  if k<=0:continue
  x=84+(j%2)*470;y=515+(j//2)*340+round((1-k)*60)
  d.rectangle((x,y,x+442,y+306),outline=65,width=2)
  text(d,x+23,y+25,f'{j+1:02}',23,120,mono=True);text(d,x+23,y+73,title,43,255,bold=True)
  graphic(im,name,x+24,y+153,394,112,t)
 return fade(im,smooth(u/.55)*(1-smooth((t-6.45)/.6)))
def showcase(t):
 im,d=layer();u=t-6.65;j=min(11,int(u/1.5));v=u-j*1.5
 text(d,84,305,f'{j+1:02} / 12   THE MACHINES',26,145,mono=True)
 fit(d,84,407,NAMES[j],912,130,ink=250,bold=True)
 for k,s in enumerate(DESC[j]):text(d,88,565+k*46,s,27,160,mono=True)
 size=756;x=162;y=710
 d.rectangle((x-22,y-22,x+size+22,y+size+22),outline=55,width=2)
 frames=CLIPS[IDS[j]];idx=min(len(frames)-1,int(v/1.5*len(frames)))
 im.paste(frames[idx].resize((size,size),Image.Resampling.NEAREST),(x,y))
 text(d,540,1518,'RHYTHMER / FIRMWARE UI',22,120,True,mono=True)
 for n in range(12):d.rectangle((84+n*77,1584,137+n*77,1589),fill=240 if n==j else 32)
 return fade(im,smooth(v/.1)*(1-smooth((v-1.4)/.1)))
def sequence(t):
 im,d=layer();u=t-24.65
 for name,title,caption,y in [('acid-generate','ACID GENERATOR','NEW / MUTATE / SCALE / SLIDE',315),('break-perform','BREAKBEAT MANIPULATOR','CHOP / REPEAT / UP-DOWN / A-B',970)]:
  fit(d,84,y,title,912,52,ink=245,bold=True)
  text(d,88,y+72,caption,23,145,mono=True)
  pic=CLIPS[name][int(max(0,u)/.08)%len(CLIPS[name])]
  yy=y+125;size=500
  d.rectangle((274,yy-12,806,yy+size+12),outline=55,width=2)
  im.paste(pic.resize((size,size),Image.Resampling.NEAREST),(290,yy))
 return fade(im,smooth(u/.5)*(1-smooth((t-28.0)/.65)))
# Reassemble the same wordmark as particles, reflowed to the portrait center.
MASK=Image.new('L',(W,H),0);md=ImageDraw.Draw(MASK)
logo_size=165
while md.textlength('RHYTHMER',font=font(logo_size,True))>912:logo_size-=1
text(md,540,825,'RHYTHMER',logo_size,255,True,True)
ma=np.array(MASK);yy,xx=np.where(ma[::8,::8]>140);TARGET=np.column_stack((xx*8,yy*8))
rng=np.random.default_rng(748001);ORDER=rng.random(len(TARGET));SOURCES=np.column_stack((540+np.cos(ORDER*math.tau)*rng.uniform(250,700,len(ORDER)),900+np.sin(ORDER*math.tau)*rng.uniform(250,800,len(ORDER))))
def logo(t):
 im,d=layer();ct=t-18;u=ct-10.2
 if u<0:return im
 solid=smooth((ct-12.9)/.7)
 for j,((tx,ty),(sx,sy)) in enumerate(zip(TARGET,SOURCES)):
  z=ease((u-ORDER[j]*.6)/2.6);bend=math.sin(z*math.pi)*(ORDER[j]-.5)*160
  x=sx+(tx-sx)*z;y=sy+(ty-sy)*z+bend;r=2 if z<.95 else 3
  if solid<1:d.rectangle((round(x)-r,round(y)-r,round(x)+r,round(y)+r),fill=round((100+155*z)*(1-solid)))
 if solid>0:im=ImageChops.lighter(im,fade(MASK,solid));d=ImageDraw.Draw(im)
 if ct>13.25:
  k=smooth((ct-13.25)/.55)
  text(d,540,1035,'SOUND INTO',44,220*k,True,True)
  text(d,540,1095,'MOTION.',44,220*k,True,True)
  d.rectangle((899,730,987,780),outline=round(180*k),width=2);text(d,943,744,'OS',28,245*k,True,mono=True)
  for j,name in enumerate(['drum','serum','phase','lofi','acid','break']):graphic(im,name,144+(j%3)*288,1280+(j//3)*120,216,82,t)
 for hit in [12.65,13.45,16.75]:
  if 0<=ct-hit<.065:
   for y,shift in [(860,15),(914,-11)]:
    strip=im.crop((0,y,W,y+6));im.paste(0,(0,y,W,y+6));im.paste(strip,(shift,y))
 return fade(im,smooth(u/.35)*(1-smooth((ct-17.1)/.55)))
def frame(t):
 base=fade(BASE,(.55+.25*math.exp(-(t%.375)*12))*(1-smooth((t-35.1)/.6)))
 layers=[chrome(t),signal(t),machines(t)]
 if 6.65<=t<24.65:layers.append(showcase(t))
 else:layers.extend([sequence(t),logo(t)])
 for pic in layers:base=ImageChops.lighter(base,pic)
 return base.convert('RGB')

times=[1.5,4.9,7.4,13.4,19.4,26.0,30.4,33.3]
board=Image.new('RGB',(1440,1360),'#141414');bd=ImageDraw.Draw(board)
for j,t in enumerate(times):
 im=frame(t);im.save(O/f'frame-{t:04.1f}.png')
 board.paste(im.resize((324,576),Image.Resampling.LANCZOS),(j%4*360+18,j//4*680+48));bd.text((j%4*360+18,j//4*680+16),f'{t:.1f}s',fill='white')
board.save(O/'Vertical-storyboard.jpg',quality=96)
frame(33.3).save(O/'Rhythmer-Vertical-Cover.jpg',quality=97)
if a.storyboard:raise SystemExit(0)
ff=imageio_ffmpeg.get_ffmpeg_exe();silent=O/'Rhythmer-OS-Opening-36s-1080x1920-Silent.mp4';video=O/'Rhythmer-OS-Opening-36s-1080x1920-Amen.mp4'
cmd=[ff,'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{W}x{H}','-framerate',str(FPS),'-i','-','-an','-c:v','libx264','-preset','medium','-crf','16','-pix_fmt','yuv420p','-threads','4','-movflags','+faststart','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709',str(silent)]
with (O/'render.log').open('wb') as log:
 p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
 try:
  for n in range(FPS*DURATION):
   p.stdin.write(frame(n/FPS).tobytes())
   if n%90==0:print(f'Rendered {n}/{FPS*DURATION} frames',flush=True)
 finally:p.stdin.close()
 if p.wait():raise RuntimeError('Video encode failed')
subprocess.run([ff,'-hide_banner','-loglevel','warning','-y','-i',str(silent),'-i',str(a.source_video),'-map','0:v:0','-map','1:a:0','-c','copy','-shortest','-movflags','+faststart','-metadata','title=Rhythmer OS Opening - Vertical','-metadata','artist=Jazzy Rhythmer',str(video)],check=True)
(O/'timeline.json').write_text(json.dumps({'durationSeconds':36,'size':[1080,1920],'fps':30,'allTwelveModelViews':True,'acidAndBreakSimultaneouslyVisible':[24.65,28.65],'sound':'AAC stream copied from the approved 36-second Amen opening, no re-encoding or remix.','sourceVideoSHA256':hashlib.sha256(a.source_video.read_bytes()).hexdigest(),'videoSHA256':hashlib.sha256(video.read_bytes()).hexdigest(),'source':Path(__file__).name},indent=2)+'\n')
shutil.copy2(__file__,O/Path(__file__).name)
print('Complete:',video,flush=True)
