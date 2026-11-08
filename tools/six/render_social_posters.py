#!/usr/bin/env python3
"""Original Rhythmer social layouts using the website's actual UI captures.
No hardware mockup, generated UI, network access or publishing.
"""
from pathlib import Path
import base64,html,io,json,hashlib,os
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[2]
OUT=Path(os.environ.get('RHYTHMER_SOCIAL_OUTPUT',ROOT/'outputs/Rhythmer-social-2026-11-07'))
BG='#0d0e0e'; PAPER='#eeeee2'; GREY='#a3a59c'; LINE='#42443e'
FONTS={'display':'/System/Library/Fonts/Supplemental/Arial Black.ttf','sans':'/System/Library/Fonts/Supplemental/Arial.ttf','bold':'/System/Library/Fonts/Supplemental/Arial Bold.ttf','mono':'/System/Library/Fonts/Menlo.ttc'}

class Poster:
 def __init__(self,w,h):
  self.w,self.h=w,h;self.im=Image.new('RGB',(w,h),BG);self.d=ImageDraw.Draw(self.im)
  self.glyph_rasters=[]
  self.svg=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}"><rect width="100%" height="100%" fill="{BG}"/>']
 def rect(self,box,fill=None,outline=None,width=1):
  self.d.rectangle(box,fill=fill,outline=outline,width=width);x,y,x1,y1=box
  self.svg.append(f'<rect x="{x}" y="{y}" width="{x1-x}" height="{y1-y}" fill="{fill or "none"}" stroke="{outline or "none"}" stroke-width="{width}"/>')
 def line(self,pts,fill=LINE,width=2):
  self.d.line(pts,fill=fill,width=width)
  self.svg.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in pts)}" fill="none" stroke="{fill}" stroke-width="{width}"/>')
 def text(self,xy,s,size=30,style='mono',color=PAPER,tracking=0):
  font=ImageFont.truetype(FONTS[style],size);x,y=xy
  if tracking:
   top=font.getbbox(s)[1]
   for ch in s:
    self.d.text((x,y-top),ch,font=font,fill=color,anchor='la');x+=self.d.textlength(ch,font=font)+tracking
  else:self.d.text((x,y),s,font=font,fill=color,anchor='lt')
  family={'display':'Arial Black','sans':'Arial','bold':'Arial','mono':'Menlo'}[style]
  self.svg.append(f'<text x="{xy[0]}" y="{y}" dominant-baseline="text-before-edge" font-family="{family}" font-size="{size}" font-weight="{"bold" if style in ("bold","display") else "normal"}" letter-spacing="{tracking}" fill="{color}">{html.escape(s)}</text>')
 def fit(self,xy,s,width,size=240,style='display',tracking=-5,color=PAPER):
  while True:
   f=ImageFont.truetype(FONTS[style],size)
   if sum(self.d.textlength(c,font=f) for c in s)+tracking*(len(s)-1)<=width:break
   size-=1
  self.text(xy,s,size,style,color,tracking);return size
 def ui(self,name,frame,x,y,size):
  im=Image.open(ROOT/'site/media/motion'/f'{name}.webp');im.seek(frame);im=im.convert('RGB')
  self.im.paste(im.resize((size,size),Image.Resampling.NEAREST),(x,y))
  b=io.BytesIO();im.save(b,format='PNG');url='data:image/png;base64,'+base64.b64encode(b.getvalue()).decode()
  self.svg.append(f'<image x="{x}" y="{y}" width="{size}" height="{size}" image-rendering="pixelated" href="{url}"/>')
 def dotfield(self,box,step=24):
  x,y,x1,y1=box
  for yy in range(y,y1,step):
   for xx in range(x,x1,step):self.rect((xx,yy,xx+1,yy+1),fill='#383a34')
 def glyph(self,kind,x,y,w,h,color=PAPER):
  # Use the real model animation, not an independently drawn approximation.
  frames={'DRUM':0,'SERUM':0,'PHASE':0,'LOFI':0,'ACID':0,'BREAK':16}
  src=ROOT/'site/media/motion'/f'{kind.lower()}.webp'
  anim=Image.open(src);anim.seek(frames[kind])
  im=anim.convert('RGBA').crop((15,101,225,191))
  # Remove only the pure-black canvas; preserve original graphic pixels/colors.
  im.putdata([(r,g,b,0 if (r,g,b)==(0,0,0) else 255) for r,g,b,a in im.getdata()])
  scale=min(w/im.width,h/im.height);ww,hh=round(im.width*scale),round(im.height*scale)
  xx,yy=round(x+(w-ww)/2),round(y+(h-hh)/2)
  scaled=im.resize((ww,hh),Image.Resampling.NEAREST)
  self.im.paste(scaled,(xx,yy),scaled)
  self.glyph_rasters.append((im.copy(),x,y,w,h))
  raw=io.BytesIO();im.save(raw,format='PNG')
  self.svg.append(f'<image x="{xx}" y="{yy}" width="{ww}" height="{hh}" image-rendering="pixelated" href="data:image/png;base64,{base64.b64encode(raw.getvalue()).decode()}"/>')
  glyphdir=OUT/'Actual-animation-graphics';glyphdir.mkdir(exist_ok=True)
  im.save(glyphdir/f'{kind.lower()}.png')
  meta=glyphdir/f'{kind.lower()}.json'
  meta.write_text(json.dumps({'source':str(src.relative_to(ROOT)),'sourceSha256':hashlib.sha256(src.read_bytes()).hexdigest(),'frame':frames[kind],'crop':[15,101,225,191],'changes':'Only pure-black canvas made transparent; original graphic pixels retained.'},indent=2)+'\n')
 def card(self,x,y,w,name,action,frame,tag,caption,compact=False):
  # 32px gutters keep every original 240x240 screen intact.
  self.rect((x,y,x+w,y+w+(104 if compact else 158)),fill='#070808',outline=LINE,width=2)
  self.text((x+26,y+18 if compact else y+22),tag,22,color=GREY)
  self.text((x+26,y+51 if compact else y+70),action,35,'bold')
  self.ui(name,frame,x+26,y+(96 if compact else 132),w-52)
  self.text((x+26,y+w+(70 if compact else 106)),caption,21,color=GREY)
 def save(self,name):
  self.im.save(OUT/(name+'.png'),optimize=True)
  self.im.save(OUT/(name+'.jpg'),quality=96,subsampling=0,optimize=True)
  (OUT/(name+'.svg')).write_text(''.join(self.svg)+'</svg>')

def reddit():
 p=Poster(2400,1350);m=100
 p.dotfield((100,126,2300,470),26)
 p.text((m,62),'R / OS',29)
 p.text((1400,65),'M–VAVE FM–1 / CUSTOM FIRMWARE',26,color=GREY)
 p.rect((2078,48,2300,104),fill=PAPER);p.text((2098,64),'EARLY BETA',24,color=BG)
 p.fit((m,165),'RHYTHMER',2200,280,tracking=-11)
 p.text((m,445),'SOUND INTO MOTION.',64,'bold',tracking=-2)
 p.line([(m,535),(2300,535)])
 p.text((m,654),'SHAPE SOUNDS.',65,'display',tracking=-2)
 p.text((m,737),'BUILD PATTERNS.',65,'display',tracking=-2)
 p.text((m,853),'Your sounds. Your machines.',32,'sans',GREY)
 p.text((m,902),'A new workflow for the FM–1.',32,'sans',GREY)
 kinds=['DRUM','SERUM','PHASE','LOFI','ACID','BREAK']
 for i,k in enumerate(kinds):
  x=m+i*135;p.glyph(k,x,1010,124,70);p.text((x,1116),k,18,color=GREY)
 p.text((m,1190),'FIND YOUR OWN GROOVE.',23,color=GREY)
 p.card(1000,558,600,'acid-generate','ACID GENERATOR',13,'05 / CREATE + MUTATE','NOTES / ACCENTS / SLIDES',True)
 p.card(1640,558,600,'break-perform','BREAK MANIPULATOR',8,'06 / CHOP + PERFORM','CHOP / REPEAT / SWITCH A–B',True)
 p.line([(m,1285),(2300,1285)])
 p.text((m,1310),'BY JAZZY RHYTHMER  /  BUILT ON JIANT',22,color=GREY)
 p.text((1265,1305),'jazzyrhythmer.github.io/Rhythmer',28)
 p.save('Rhythmer-Reddit-2400x1350')
 return p

def instagram():
 p=Poster(2160,2700);m=120
 p.text((m,100),'R / OS',31)
 p.text((1090,100),'FM–1 / CUSTOM FIRMWARE',31,color=GREY)
 p.dotfield((m,210,2040,535),28)
 p.fit((m,228),'RHYTHMER',1920,285,tracking=-12)
 p.text((m,556),'SOUND INTO MOTION.',78,'bold',tracking=-3)
 p.line([(m,698),(2040,698)])
 p.rect((m,752,m+226,812),fill=PAPER);p.text((m+21,771),'EARLY BETA',25,color=BG)
 p.text((m,878),'SHAPE SOUNDS.',100,'display',tracking=-4)
 p.text((m,1005),'BUILD PATTERNS.',100,'display',tracking=-4)
 p.card(m,1205,930,'acid-generate','ACID GENERATOR',13,'05 / CREATE + MUTATE','SCALE / ACCENT / SLIDE')
 p.card(1110,1205,930,'break-perform','BREAK MANIPULATOR',8,'06 / CHOP + PERFORM','CHOP / REPEAT / SWITCH A–B')
 # Six graphics extracted directly from the real website model animations.
 for i,k in enumerate(['DRUM','SERUM','PHASE','LOFI','ACID','BREAK']):
  x=m+i*326;p.glyph(k,x,2370,225,88);p.text((x,2487),k,24,color=GREY)
 p.line([(m,2305),(2040,2305)])
 p.text((m,2575),'JAZZY RHYTHMER / BUILT ON JIANT',26,color=GREY)
 p.text((m,2630),'jazzyrhythmer.github.io/Rhythmer',35)
 p.save('Rhythmer-Instagram-2160x2700')
 upload=p.im.resize((1080,1350),Image.Resampling.LANCZOS)
 # Preserve the native white dots when the upload image is reduced to 1080px.
 ud=ImageDraw.Draw(upload)
 for graphic,x,y,w,h in p.glyph_rasters:
  ud.rectangle((x//2,y//2,(x+w)//2,(y+h)//2),fill=BG)
  scale=min((w/2)/graphic.width,(h/2)/graphic.height)
  ww,hh=round(graphic.width*scale),round(graphic.height*scale)
  xx,yy=round(x/2+(w/2-ww)/2),round(y/2+(h/2-hh)/2)
  piece=graphic.resize((ww,hh),Image.Resampling.NEAREST);upload.paste(piece,(xx,yy),piece)
 upload.save(OUT/'Rhythmer-Instagram-1080x1350.jpg',quality=97,subsampling=0,optimize=True)
 return p

if __name__=='__main__':
 assert OUT.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
 a=reddit();b=instagram()
 preview=Image.new('RGB',(1600,1110),'#252622')
 preview.paste(a.im.resize((1120,630),Image.Resampling.LANCZOS),(0,240))
 preview.paste(b.im.resize((480,600),Image.Resampling.LANCZOS),(1120,255))
 preview.save(OUT/'Preview.jpg',quality=94)
 (OUT/'README.txt').write_text('RHYTHMER / SOCIAL ARTWORK\n\nReddit: Rhythmer-Reddit-2400x1350.jpg (16:9)\nInstagram: Rhythmer-Instagram-1080x1350.jpg (4:5, ready to upload)\nInstagram master: 2160x2700 PNG/JPG. SVG layouts are editable.\n\nEnglish artwork based on the current website: off-white on black, dot/pixel graphics, actual firmware UI animation frames. Acid frame 13 / Break frame 8. Screens are host-rendered firmware UI, not physical-device photography. No device photo or fabricated interface.\n\nRetain JIANT and other upstream credits in the accompanying post. This is early-beta promotional artwork, not a claim of resolved hardware audio issues. No social post was published by the rendering script.\n')
 print(OUT)
