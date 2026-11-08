#!/usr/bin/env python3
"""Copy refresh for the existing update/manual films; actual UI, original audio."""
from pathlib import Path
import argparse, hashlib, json, math, shutil, subprocess, sys, zipfile
from PIL import Image, ImageDraw
import render_update_media as u
import render_manual_tour_v3 as manual

R=Path(__file__).resolve().parents[2]
O=R/'outputs/Rhythmer-Media-Refresh-20261108'
Q=R/'qa/media-refresh-20261108'
FF=Path(u.FF)
VERSION='v0.1.1-beta3'
ap=argparse.ArgumentParser()
ap.add_argument('--video',choices=['update-youtube','update-instagram','manual-youtube','manual-instagram'])
ap.add_argument('--package',action='store_true')
args=ap.parse_args()
assert O.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
assert Q.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
u.load()
manual.m.CONTENTS=Image.open(Q/'manual-current.png').convert('RGB')
FILMS={
 'update-youtube':(68,1920,1080,lambda t:u.frame(t,False),R/'outputs/Rhythmer-update-2026-11-07-v2/Rhythmer-Update-YouTube-1920x1080-68s.mp4'),
 'update-instagram':(52,1080,1920,lambda t:u.frame(t,True),R/'outputs/Rhythmer-update-2026-11-07-v2/Rhythmer-Update-Reels-1080x1920-52s.mp4'),
 'manual-youtube':(64,1920,1080,manual.landscape,R/'outputs/Rhythmer-Manual-Launch-v3/Rhythmer-Manual-YouTube-1920x1080-64s.mp4'),
 'manual-instagram':(64,1080,1920,manual.portrait,R/'outputs/Rhythmer-Manual-Launch-v3/Rhythmer-Manual-Reels-1080x1920-64s.mp4')
}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def audio_sha(p):
 return hashlib.sha256(subprocess.check_output([str(FF),'-v','error','-i',str(p),'-map','0:a:0','-c','copy','-f','adts','-'])).hexdigest()
def title(im,xy,text,width,size,color=u.INK,mono=False):u.fit(im,xy,text,width,size,mono,color)
def screen_grid(im,x,y,size,gap,names,t=.6):
 for i,(name,label) in enumerate(names):
  xx=x+(i%3)*(size+gap);yy=y+(i//3)*(size+gap+52)
  u.text(im,(xx,yy),label,24,True,u.GRAY)
  u.screen(im,name,(xx,yy+44),size,t+i*.17)
def thumbnail(topic,portrait=False):
 w,h=(2160,2700) if portrait else (3840,2160)
 im=u.bg(w,h);m=120 if portrait else 140;d=ImageDraw.Draw(im)
 u.text(im,(m,85),'R / OS',35,True)
 title(im,(w-m-(680 if portrait else 780),88),'FM-1 / GROOVEBOX FIRMWARE',680 if portrait else 780,32,u.GRAY,True)
 title(im,(m,205),'RHYTHMER',w-2*m,255 if portrait else 290)
 u.text(im,(m,520 if portrait else 545),'SOUND INTO MOTION.',49 if portrait else 65)
 u.line(im,[(m,630 if portrait else 690),(w-m,630 if portrait else 690)])
 if portrait:
  tag='UPDATE' if topic=='update' else 'THE MANUAL'
  title(im,(m,720),tag,1900,210)
  u.text(im,(m,985),VERSION if topic=='update' else 'FIND THE BUTTON. MAKE THE MOVE.',70 if topic=='update' else 59,topic!='update',u.GRAY)
  names=([('drum-sound','01 / SWAP ONE DRUM'),('gen-styles','02 / NEW GENRES'),('gen-target','03 / TARGET GEN'),('gen-break','04 / BREAK GEN'),('acid-generate','05 / ACID'),('break-perform','06 / PERFORMANCE')] if topic=='update' else [('drum','DRUM'),('serum','SERUM'),('phase','PHASE'),('lofi','LOFI'),('acid-generate','ACID / GEN'),('break-perform','BREAK / PERF')])
  u.text(im,(m,1080),'DRUM SOUNDS / GEN / SAMPLE UPLOAD' if topic=='update' else 'CONTROLS / TRACK MODES / SHORTCUTS',34,True,u.GRAY)
  screen_grid(im,120,1150,540,90,names)
  u.text(im,(m,2500),'SHAPE / SEQUENCE / PERFORM / SAVE',35,True)
  u.text(im,(m,2580),'JAZZY RHYTHMER / BUILT ON JIANT',27,True,u.GRAY)
 else:
  if topic=='update':
   d.rectangle((m,810,1375,1090),fill=u.INK)
   title(im,(m+35,828),'UPDATE',1150,245,u.BG)
   u.text(im,(m,1160),VERSION,104)
   for i,line in enumerate(['SWAP ONE DRUM SOUND','NEW GENRES / TARGET GEN','GENRE-AWARE BREAK GEN','BRING YOUR OWN SAMPLES']):
    title(im,(m+10,1350+i*118),line,1840,69)
   u.screen(im,'drum-sound',(2220,745),1180,.5)
   u.text(im,(2220,1965),'EDIT → SOUND → SELECT',36,True,u.GRAY)
  else:
   title(im,(m,850),'THE MANUAL',1990,206)
   title(im,(m,1160),'FIND THE BUTTON.',1990,104)
   title(im,(m,1295),'MAKE THE MOVE.',1990,104)
   u.text(im,(m,1510),'READ BEFORE YOUR FIRST SESSION',42,True,u.GRAY)
   u.text(im,(m,1655),'SOUND / GEN / SEQUENCE / SAVE',41,True,u.GRAY)
   screen_grid(im,2260,800,410,65,[('drum','DRUM'),('serum','SERUM'),('phase','PHASE'),('lofi','LOFI'),('acid-generate','ACID / GEN'),('break-perform','BREAK / PERF')])
  u.line(im,[(m,2040),(w-m,2040)])
  u.text(im,(m,2080),'JAZZY RHYTHMER / BUILT ON JIANT',33,True,u.GRAY)
  u.text(im,(2330,2080),'UP TO SIX ASSIGNABLE TRACKS',33,True,u.GRAY)
 return im
def assets():
 for topic in ['update','manual']:
  for portrait in [False,True]:
   im=thumbnail(topic,portrait);stem=f'Rhythmer-{topic.title()}-'+('Instagram' if portrait else 'YouTube')
   im.save(O/(stem+'-master.png'),optimize=True)
   size=(1080,1350) if portrait else (1280,720)
   im.resize(size,Image.Resampling.LANCZOS).save(O/(stem+'.jpg'),quality=97,subsampling=0)
   if portrait:
    # A separate reel cover keeps titles away from top/bottom overlay controls.
    cover=u.bg(1080,1920)
    scaled=im.resize((1080,1350),Image.Resampling.LANCZOS)
    cover.paste(scaled,(0,285))
    u.text(cover,(60,165),'RHYTHMER / SOUND INTO MOTION.',24,True,u.GRAY)
    u.text(cover,(60,1740),'UP TO SIX ASSIGNABLE TRACKS',23,True,u.GRAY)
    cover.save(O/(stem+'-Reels-Cover-1080x1920.jpg'),quality=97,subsampling=0)
 for key,(dur,w,h,f,src) in FILMS.items():
  ts=([1,6,16,26,37,48,58,66] if key=='update-youtube' else [1,6,14,22,30,40,50] if key=='update-instagram' else [1,6,10,15,21,26,32,40,45,50,56,62])
  tw,th=(480,270) if w>h else (270,480);cols=3 if w>h else 4
  board=u.bg(cols*tw,math.ceil(len(ts)/cols)*(th+34))
  for i,t in enumerate(ts):
   im=f(t);im.save(Q/f'{key}-{t:02d}s.jpg',quality=95,subsampling=0)
   x=i%cols*tw;y=i//cols*(th+34);board.paste(im.resize((tw,th),Image.Resampling.LANCZOS),(x,y+34));u.text(board,(x+10,y+8),f'{t:02d}s',17,True,u.GRAY)
  board.save(O/f'Storyboard-{key}.jpg',quality=95)
 # Contact sheet for reviewing all four thumbnails together.
 contact=u.bg(1600,1200)
 for i,topic in enumerate(['Update','Manual']):
  for j,plat in enumerate(['YouTube','Instagram']):
   im=Image.open(O/f'Rhythmer-{topic}-{plat}.jpg');im.thumbnail((780,565),Image.Resampling.LANCZOS)
   x=j*800+(800-im.width)//2;y=i*600+28
   contact.paste(im,(x,y));u.text(contact,(j*800+25,i*600+5),f'{topic.upper()} / {plat.upper()}',19,True,u.GRAY)
 contact.save(O/'Thumbnail-Preview.jpg',quality=95)
 print('Thumbnails and storyboards ready.',flush=True)
def video(key):
 dur,w,h,f,src=FILMS[key];dest=O/f'Rhythmer-{key}-{w}x{h}-{dur}s.mp4'
 cmd=[str(FF),'-v','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{w}x{h}','-framerate','30','-i','-','-i',str(src),'-map','0:v:0','-map','1:a:0','-t',str(dur),'-c:v','libx264','-crf','17','-preset','fast','-threads','3','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-c:a','copy','-movflags','+faststart','-metadata',f'title=Rhythmer {VERSION} / {key} / Sound into motion.','-metadata','artist=Jazzy Rhythmer',str(dest)]
 with (Q/(key+'-encode.log')).open('w') as log:
  p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
  try:
   for n in range(dur*30):
    p.stdin.write(f(n/30).tobytes())
    if n%300==0:print(key,n,'/',dur*30,flush=True)
  finally:p.stdin.close()
  if p.wait():raise RuntimeError(key+' encode failed')
 assert audio_sha(dest)==audio_sha(src),'Soundtrack must retain original AAC samples'
 # ffmpeg's decode path checks every video/audio packet, not just the header.
 subprocess.run([str(FF),'-v','error','-i',str(dest),'-f','null','-'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 (Q/(key+'-verification.json')).write_text(json.dumps({'file':dest.name,'size':[w,h],'duration':dur,'fps':30,'sha256':sha(dest),'originalAACBitstreamIdentical':True,'completeDecode':'PASS','visuals':'Existing host-rendered firmware UI and current website screenshot; not physical-device footage.','audioSource':str(src.relative_to(R))},indent=2)+'\n')
 print('Complete',dest,flush=True)
def package():
 manifest={'version':VERSION,'headline':'SOUND INTO MOTION.','capacity':'A groovebox with up to six assignable tracks.','firmware':'Media refresh only; no firmware write or hardware test performed.','source':'tools/six/render_brand_media.py','films':[json.loads((Q/(key+'-verification.json')).read_text()) for key in FILMS]}
 (O/'MEDIA-VALIDATION.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (O/'README.txt').write_text('RHYTHMER / SOUND INTO MOTION.\n\nUpdate: YouTube 68s (1920x1080), Instagram Reels 52s (1080x1920).\nManual: YouTube and Instagram Reels 64s each. All 30fps H.264 + original AAC.\n\nThumbnails: YouTube 1280x720, Instagram post 1080x1350, Instagram Reels cover 1080x1920. High-resolution PNG masters included.\n\nThe update starts with individual drum replacement, then new genres, target GEN, Break GEN and sample upload. The manual starts by recommending reading the guide, then covers the default mode, six-track activation/CPU confirmation, genre-aware Break GEN and held-SEQ shortcuts. Up to six tracks remains a feature, not the product headline.\n\nExisting actual UI/screens and music retained. All four original AAC streams are bit-identical. This music is the original atmospheric breakbeat soundtrack, not a direct FM-1 audio recording. No artist recordings or artist endorsement implied.\n\nUpstream credits: JIANT and the other firmware projects remain credited on the website. No social posts are uploaded by this tool.\n')
 source=O/'source';source.mkdir(exist_ok=True)
 for name in ['render_brand_media.py','render_update_media.py','render_manual_tour_v3.py','render_manual_short.py']:shutil.copy2(R/'tools/six'/name,source/name)
 for topic in ['update','manual']:
  files=list(O.glob(f'Rhythmer-{topic}-*.mp4'))+list(O.glob(f'Rhythmer-{topic.title()}-*'))+list(O.glob(f'Storyboard-{topic}-*.jpg'))
  with zipfile.ZipFile(O/f'Rhythmer-{topic.title()}-Media.zip','w',zipfile.ZIP_STORED) as z:
   for p in files:z.write(p,p.name)
   for name in ['README.txt','MEDIA-VALIDATION.json',topic.title()+'-YouTube.txt',topic.title()+'-Instagram.txt']:
    if (O/name).exists():z.write(O/name,name)
 print('Packages ready.',flush=True)
if args.video:video(args.video)
elif args.package:package()
else:assets()
