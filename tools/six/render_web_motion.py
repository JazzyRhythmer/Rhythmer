#!/usr/bin/env python3
"""Encode 240px host firmware captures as GIF, lossless WebP and still posters."""
from pathlib import Path
import hashlib,json,sys
from PIL import Image,ImageDraw
q=Path(sys.argv[1]);manifest=[];tiles=[]
for p in sorted(q.glob('*.rgb')):
 raw=p.read_bytes();assert len(raw)==48*240*240*3
 frames=[Image.frombytes('RGB',(240,240),raw[i*172800:(i+1)*172800]) for i in range(48)]
 unique=len({hashlib.sha256(f.tobytes()).digest() for f in frames})
 print(p.stem,unique,flush=True);assert unique>1,p.name
 palette=Image.new('P',(1,1));palette.putpalette([v for i in range(256) for v in (i,i,i)])
 gs=[f.quantize(palette=palette,dither=Image.Dither.NONE) for f in frames]
 gs[0].save(q/(p.stem+'.gif'),save_all=True,append_images=gs[1:],duration=80,loop=0,optimize=True,disposal=2)
 frames[0].save(q/(p.stem+'.png'))
 frames[0].save(q/(p.stem+'.webp'),save_all=True,append_images=frames[1:],duration=80,loop=0,lossless=True,method=4)
 tile=Image.new('RGB',(260,282),'#151515');tile.paste(frames[8],(10,30));ImageDraw.Draw(tile).text((10,10),p.stem,fill='white');tiles.append(tile)
 manifest.append({'id':p.stem,'frames':48,'uniqueFrames':unique,'durationMs':3840,'capture':'Host-rendered unchanged firmware UI with note/envelope and preset input; not physical FM-1 footage','gifBytes':(q/(p.stem+'.gif')).stat().st_size,'gifSha256':hashlib.sha256((q/(p.stem+'.gif')).read_bytes()).hexdigest()})
board=Image.new('RGB',(1040,1128),'#151515')
for i,tile in enumerate(tiles):board.paste(tile,(i%4*260,i//4*282))
board.save(q/'contact-sheet.png');(q/'motion.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS',len(manifest),'moving firmware UI clips; GIF total',sum(x['gifBytes'] for x in manifest),'bytes')
