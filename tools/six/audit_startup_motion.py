#!/usr/bin/env python3
"""Exact-image startup frames, unchanged fresh defaults, and 4/6-track playback."""
from pathlib import Path
import hashlib,json,struct,zlib
from PIL import Image
R=Path(__file__).resolve().parents[2]
p=R/'tools/six/test_rhythmer_emulator.py'
g={'__file__':str(p)}
exec(p.read_text().split("if sys.argv[1:] in (['--upgrade'],['--upgrade-ensemble']):")[0],g)
O=g['OUT'];SYM=g['SYM'];run=g['run']
token=g['report']['firmware_sha256'][:12]
base=O/f'startup-fresh-{token}.flash'
ram=run('startup-fresh',3000,flash=base,strict=True)
assert ram[SYM['trk_live']]==4
assert g['report']['runs']['startup-fresh']['track_engines']==[10,17,2,3,21,21]
assert g['report']['runs']['startup-fresh']['six_loop_mask']==3
original=base.read_bytes()
for i,name in enumerate(['amen-160','think-160']):
 data=(g['SAMPLES']/(name+'.slot')).read_bytes()
 assert original[0xa0000+i*0x14000:0xb4000+i*0x14000]==data
for ms in (480,1200,1800):
 run(f'startup-frame-{ms}',ms,flash=O/f'startup-frame-{ms}-{token}.flash')
 Image.open(O/f'startup-frame-{ms}.ppm').crop((370,42,850,522)).save(O/f'startup-native-{ms}.png')
def put(f,off,kind,data):
 h=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(data),zlib.crc32(data),0xffffffff,0xffffffff)
 h+=struct.pack('<I',zlib.crc32(h));s=bytearray(b'\xff'*4096);s[:32]=h;s[256:256+len(data)]=data;f[off:off+4096]=s
fixture=R/'rhythmer/tests/fixtures/startup-md-default.project'
data=fixture.read_bytes()
for live in (4,6):
 f=bytearray(original);cfg=bytearray(f[0xfc100:0xfc100+572]);cfg[52+15*32+15]=0x60+(live==6);cfg[52+15*32+30]|=128;put(f,0xfc000,0,cfg)
 for at,kind in ((0x97000,1),(0xe5000,9)):put(f,at,kind,data)
 seed=O/f'startup-play-{live}-{token}.flash';seed.write_bytes(f)
 ram=run(f'startup-play-{live}',6500,'2500:12,2620:12^',seed,True)
 d=g['report']['runs'][f'startup-play-{live}']
 assert ram[SYM['trk_live']]==live
 assert d['peak_pcm16']>0 and d['usb_capture_frames']>1000
for name,d in g['report']['runs'].items():
 assert d['boots']==1 and d['fm1_mc_on']==1,name
 assert not any(d.get(k,0) for k in ['late','audio_lost','fm1_mc_lost','usb_underruns','usb_overruns','usb_missed']),name
 assert d['max_us']<g['HALF']/44100*1e6,(name,d)
g['report']['scope']='Actual-FWSC startup frames, virgin four-track/EMPTY defaults and embedded banks, four-/six-track controlled MD ensemble playback with USB; no physical device writes.'
(O/'startup-native-results.json').write_text(json.dumps(g['report'],indent=2)+'\n')
print('Startup display, unchanged defaults and four/six playback PASS')
