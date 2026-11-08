#!/usr/bin/env python3
"""48-second manual tour: genre-aware Break GEN and the real held-SEQ map."""
from pathlib import Path
from functools import lru_cache
import hashlib, json, math, subprocess, sys
from PIL import Image, ImageDraw, ImageFont

R=Path(__file__).resolve().parents[2]
O=R/'outputs/Rhythmer-Manual-Launch-v2'
Q=R/'qa/manual-short-v2-20261107'
FF='/Volumes/SSD/Development/Toolchains/rhythmer-motion/imageio_ffmpeg/binaries/ffmpeg-macos-aarch64-v7.1'
INK=(238,239,226);BG=(10,12,12);GRAY=(157,165,157);LINE=(54,61,55)
W,H,FPS,DURATION=1080,1920,30,48
SCORE=R/'outputs/Rhythmer-update-2026-11-07-v2/Rhythmer-Update-Original-Score.wav'
CLIPS={}
@lru_cache(None)
def font(size,mono=False):
    p=R/'rhythmer/assets/fonts'/('IBMPlexMono-SemiBold.ttf' if mono else 'InterTight[wght].ttf')
    f=ImageFont.truetype(str(p),size)
    if not mono:f.set_variation_by_axes([780])
    return f
def text(im,x,y,s,size=40,mono=False,color=INK):
    ImageDraw.Draw(im).text((x,y),s,font=font(size,mono),fill=color,anchor='lt')
def rule(im,y):ImageDraw.Draw(im).line((86,y,994,y),fill=LINE,width=2)
def fitted(im,x,y,s,max_width=900,size=84,mono=False,color=INK):
    d=ImageDraw.Draw(im)
    while d.textlength(s,font=font(size,mono))>max_width:size-=1
    text(im,x,y,s,size,mono,color)
def load():
    for n in ('drum','serum','phase','lofi','acid','break','acid-generate','break-perform','drum-sound','gen-target','gen-break'):
        a=Image.open(R/'site/media/motion'/f'{n}.webp');CLIPS[n]=[]
        for i in range(a.n_frames):a.seek(i);CLIPS[n].append(a.convert('RGB'))
def clip(n,t):return CLIPS[n][int(t*12.5)%len(CLIPS[n])]
def screen(im,n,x,y,size,t):
    im.paste(clip(n,t).resize((size,size),Image.Resampling.NEAREST),(x,y))
    ImageDraw.Draw(im).rectangle((x-2,y-2,x+size+1,y+size+1),outline=LINE,width=2)
def glyphs(im,y,t):
    for i,n in enumerate(('drum','serum','phase','lofi','acid','break')):
        a=clip(n,t+i*.12).crop((15,101,225,191)).resize((130,56),Image.Resampling.NEAREST)
        im.paste(a,(86+i*154,y));text(im,86+i*154,y+72,n.upper(),18,True,GRAY)
def background(t):
    im=Image.new('RGB',(W,H),BG);d=ImageDraw.Draw(im)
    for y in range(108,1760,28):
        for x in range(86,1000,28):d.point((x,y),fill=(26,31,27))
    text(im,86,130,'R / OS',25,True)
    fitted(im,420,134,'v0.1.1-beta3 / THE MANUAL',570,25,True,GRAY)
    rule(im,195);rule(im,1660)
    text(im,86,1694,'JAZZY RHYTHMER',20,True,GRAY)
    text(im,680,1694,'FM-1 / EARLY BETA',20,True,GRAY)
    d.line((86,1658,86+int(908*min(t/DURATION,1)),1658),fill=INK,width=3)
    return im
def heading(im,a,b,label):
    text(im,86,255,label,23,True,GRAY)
    fitted(im,80,330,a,size=100)
    fitted(im,80,448,b,size=100)
def lines(im,ss,y=1460,size=32):
    for i,s in enumerate(ss):fitted(im,86,y+i*(size+21),s,900,size,False,INK if i==0 else GRAY)
CONTENTS=None;UPLOAD=None;BREAK_KEYS=None
def frame(t):
    im=background(t)
    if t<4:
        heading(im,'READ THE MANUAL.','BEFORE YOU PLAY.','RHYTHMER / SOUND INTO MOTION.')
        text(im,86,648,'15',280)
        text(im,495,690,'GUIDED',65);text(im,495,775,'SECTIONS',65)
        text(im,88,1035,'BUTTON BY BUTTON.',49,True)
        text(im,88,1115,'FROM THE FIRST NOTE.',42,True,GRAY)
        glyphs(im,1270,t)
        lines(im,['A few minutes here. More possibilities later.','Learn the shortcuts you might have missed.'],1460,30)
    elif t<8:
        heading(im,'FIND THE BUTTON.','MAKE THE MOVE.','01 / SEARCHABLE. SHAREABLE. PRINTABLE.')
        a=CONTENTS.resize((908,511),Image.Resampling.LANCZOS);im.paste(a,(86,700))
        rule(im,1245)
        for i,s in enumerate(('SEARCH BY TOPIC','FOLLOW THE KEY MAP','PRINT / SAVE PDF')):
            text(im,90,1290+i*75,f'0{i+1}  '+s,31,True)
        lines(im,['Direct links open the chapter you need.'],1540,29)
    elif t<12.5:
        heading(im,'MAKE A LINE.','IN YOUR SCALE.','02 / ACID GENERATOR')
        screen(im,'acid-generate',130,610,820,t-8)
        lines(im,['SCL → EDIT → ACID','OCT+ NEW / OCT− MUTATE'],1470,33)
    elif t<20:
        heading(im,'BREAK SEQUENCES.','FOR YOUR GENRE.','03 / GENRE-AWARE BREAK GEN')
        screen(im,'gen-break',190,600,700,t-12.5)
        text(im,86,1340,'HOME + SELECT → GEN',31,True)
        text(im,86,1400,'GENRE → TARGET: BREAK → OCT+',31,True)
        lines(im,['Hip-hop space + loose timing. Jungle cuts + rolls.','New chop sequences. Same tempo, samples + other parts.'],1485,28)
    elif t<28:
        heading(im,'SELECT BREAK.','THEN HOLD SEQ.','04 / THREE SHORTCUTS WORTH KNOWING')
        im.paste(BREAK_KEYS.resize((700,700),Image.Resampling.NEAREST),(190,600))
        d=ImageDraw.Draw(im)
        k=min(2,int((t-20)/2.667))
        # Editorial highlight sits outside the actual UI; the captured pixels stay unchanged.
        d.rectangle((77,1330+k*78,1002,1394+k*78),outline=INK,width=2)
        for i,(key,action,detail) in enumerate((('F3','STRAIGHT','Ordered two-bar loop'),('G3','MUTATE','Vary chops; keep beat anchors'),('A3','RANDOM','Create a new chop order'))):
            text(im,94,1345+i*78,f'{key}  {action}',30,True)
            fitted(im,452,1348+i*78,detail,535,28,color=INK if i==k else GRAY)
        text(im,86,1585,'These actions replace/edit the Break sequence.',24,True,GRAY)
    elif t<32:
        heading(im,'CUT. REPEAT.','PLAY THE VARIATION.','05 / BREAK PERFORMANCE')
        screen(im,'break-perform',190,600,700,t-28)
        lines(im,['Release SEQ → EDIT → BREAK → BRK PERF','Play repeats, pitch up/down and A/B changes.','Arm REC to write gestures into the sequence.'],1370,30)
    elif t<37:
        heading(im,'ONE PART.','YOUR DECISION.','06 / TARGET GEN + DRUM SOUND REPLACEMENT')
        screen(im,'drum-sound',86,690,440,t);screen(im,'gen-target',554,690,440,t+.7)
        text(im,86,1160,'SWAP ONE DRUM',25,True)
        text(im,554,1160,'GENERATE ONE TRACK',25,True)
        lines(im,['Keep the rest of your groove.','GEN ALL writes T1–T4 only.'],1390,36)
        text(im,86,1555,'59 GEN STYLES / STEP-BY-STEP RECIPES',23,True,GRAY)
    elif t<43:
        heading(im,'YOUR BREAKS.','YOUR WORKFLOW.','07 / SAMPLE UPLOAD + PROJECT BACKUPS')
        a=UPLOAD.copy();a.thumbnail((415,810),Image.Resampling.LANCZOS);im.paste(a,(575,680))
        for i,(num,title) in enumerate((('01','SOURCE BPM'),('02','EIGHT BEATS'),('03','PREVIEW'),('04','CONFIRM'))):
            text(im,86,700+i*172,num,25,True,GRAY);text(im,86,748+i*172,title,37)
        lines(im,['Keep four projects and A–D sections.','Music archives ≠ sample audio.'],1505,29)
    else:
        heading(im,'LESS MENU HUNTING.','MORE MUSIC.','THE RHYTHMER USER MANUAL')
        glyphs(im,740,t)
        text(im,86,955,'15 SECTIONS',66)
        text(im,86,1050,'ACID / BREAK / GEN / SAVE',29,True,GRAY)
        text(im,86,1190,'OPEN THE MANUAL',54)
        fitted(im,86,1325,'jazzyrhythmer.github.io',900,42,True)
        text(im,86,1393,'/Rhythmer/#guide',42,True)
        lines(im,['Free / open source / built on JIANT','Actual guide + host-rendered firmware UI.'],1525,25)
    # Short crossfades retain full legibility for each scene.
    return im
def poster():
    frame(1.4).save(O/'Rhythmer-Manual-Shorts-Cover-1080x1920.jpg',quality=97,subsampling=0)
    im=Image.new('RGB',(1080,1350),BG)
    text(im,72,75,'R / OS    RHYTHMER',26,True)
    text(im,72,165,'THE MANUAL.',120);text(im,72,305,'NOW ONLINE.',120)
    text(im,76,470,'15 SECTIONS / v0.1.1-beta3',30,True,GRAY)
    screen(im,'acid-generate',76,580,440,1.2);screen(im,'break-perform',564,580,440,1.4)
    text(im,76,1060,'ACID / BREAK / GEN / YOUR SAMPLES',29,True)
    text(im,76,1130,'Search. Follow the buttons. Make music.',31,False,GRAY)
    rule(im,1210);text(im,76,1250,'jazzyrhythmer.github.io/Rhythmer/#guide',26,True)
    im.save(O/'Rhythmer-Manual-Instagram-1080x1350.jpg',quality=97,subsampling=0)
def video():
    target=O/'Rhythmer-Manual-Shorts-1080x1920-48s.mp4'
    cmd=[FF,'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{W}x{H}','-framerate',str(FPS),'-i','-','-ss','0','-i',str(SCORE),'-map','0:v','-map','1:a','-t',str(DURATION),'-c:v','libx264','-crf','18','-preset','fast','-threads','4','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-c:a','aac','-b:a','256k','-ar','48000','-af',f'volume=0.85,afade=t=in:d=0.4,afade=t=out:st={DURATION-1.3}:d=1.3','-movflags','+faststart','-metadata','title=Rhythmer / Read the Manual / Genre-aware Break GEN','-metadata','artist=Jazzy Rhythmer',str(target)]
    with (Q/'manual-short-encode.log').open('w') as log:
        proc=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=log)
        try:
            for i in range(DURATION*FPS):
                proc.stdin.write(frame(i/FPS).tobytes())
                if i%150==0:print('Manual short:',i,'/',DURATION*FPS,flush=True)
        finally:proc.stdin.close()
        if proc.wait():raise RuntimeError('Video encoding failed')
    print('Complete:',target,flush=True)
if __name__=='__main__':
    assert O.resolve().is_relative_to(Path('/Volumes/SSD/Development'))
    load();CONTENTS=Image.open(R/'qa/site-manual-20261107/manual-contents.png').convert('RGB')
    BREAK_KEYS=Image.open(Q/'break-held-seq.ppm').convert('RGB')
    UPLOAD=Image.open(R/'qa/update-media-20261107/editor-upload.jpg').convert('RGB').crop((55,676,640,1740))
    poster()
    for at in (1,5,10,15,21,24,27,30,34,40,46):frame(at).save(O/f'Preview-{at:02d}s.jpg',quality=95,subsampling=0)
    if '--video' in sys.argv:video()
    manifest={'duration':DURATION,'revision':2,'size':[W,H],'fps':FPS,'firmwareVersion':'0.1.1-beta3','firmwareBuild':'20261107.2','firmwareSha256':'6ef6f75316c4e70672c9a4e26fdb852340be28cc0242c8fcb63086506a438fb0','visuals':'Actual guide screenshot and host-rendered firmware UI; not physical device footage. Held-SEQ UI freshly captured; external editorial shortcut highlights.','audio':'Original atmospheric breakbeat score created for Rhythmer; no artist recordings','audioSourceSha256':hashlib.sha256(SCORE.read_bytes()).hexdigest(),'keyMessages':['Read the manual before playing','BREAK GEN writes genre-aware chop sequences, retaining tempo, samples and other parts','Hold SEQ on Break: F3 STRAIGHT, G3 MUTATE, A3 RANDOM','Release SEQ for BRK PERF live gestures']}
    (O/'MEDIA-PROVENANCE.json').write_text(json.dumps(manifest,indent=2)+'\n')
