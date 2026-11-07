#!/usr/bin/env python3
"""Exact-image live GEN while playing, virtual USB only; no physical I/O."""
import os,sys,json,struct,zlib,hashlib
from pathlib import Path
R=Path(__file__).resolve().parents[2]
src=(R/'tools/six/test_rhythmer_emulator.py').read_text().split("if sys.argv[1:] in")[0]
exec(compile(src,'test_rhythmer_emulator.py','exec'))
reuse=sys.argv[1:]==['--half-only']
if reuse:
 report=json.loads((OUT/'target-emulator-results.json').read_text());assert report['firmware_sha256']==hashlib.sha256(FW.read_bytes()).hexdigest()
base=OUT/'target-base.flash'
if not reuse:run('target-boot',2800,flash=base,strict=True)
original=base.read_bytes();flash=bytearray(original)
def put(off,kind,data):
 head=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(data),zlib.crc32(data),0xffffffff,0xffffffff);head+=struct.pack('<I',zlib.crc32(head));sector=bytearray(b'\xff'*4096);sector[:32]=head;sector[256:256+len(data)]=data;flash[off:off+4096]=sector
settings=bytearray(flash[0xfc100:0xfc100+572]);settings[52+15*32+30]|=128;put(0xfc000,0,settings)
def fixture(path):
 data=Path(path).read_bytes()
 for off,kind in ((0x97000,1),(0xe5000,9)):put(off,kind,data)
 seed=OUT/'target-fixture.flash';seed.write_bytes(flash);return seed
for f in sorted(OUT.glob('upgrade-target-*.project')):
 if reuse and f.name!='upgrade-target-drunken-half.project':continue
 seed=fixture(f);run(f.stem,9000,'2500:12,2620:12^',seed,True)
if not reuse:
 # Same live fixture, same controls. Only the second run actually presses GEN.
 seed=fixture(OUT/'upgrade-target-drunken-break.project');order=json.loads((ROOT/'docs/six/GEN-GENRE-ORDER.json').read_text())['stableRowIds'];pos=order.index(58)+2
 keys=['2500:12','2620:12^','3100:8','3150:e0:1','3280:8^']
 keys += [f'{4100+i*150}:e2:1' for i in range(pos)]
 keys += [f'{4100+i*150}:e5:1' for i in range(9)]
 at=4100+pos*150+500
 prefix=','.join(keys)
 a=run('target-live-control',at+2500,prefix,fixture(OUT/'upgrade-target-drunken-break.project'),True)
 b=run('target-live-generate',at+2500,prefix+f',{at}:1,{at+120}:1^',fixture(OUT/'upgrade-target-drunken-break.project'),True)
 assert a[SYM['gp']+16]==b[SYM['gp']+16]==7,'physical knob 4 did not select BREAK'
 assert a[SYM['gp']]==b[SYM['gp']]==59,'physical genre knob did not select DRUNKEN HH'
 layout=json.loads((OUT/'six-model-layout.json').read_text());stride=layout['track_stride'];p=layout['p'];st=layout['step'];ss=layout['step_stride']
 assert a[SYM['song']:SYM['song']+142]==b[SYM['song']:SYM['song']+142],'global musical parameters changed'
 for k in range(6):
  off=SYM['trk']+k*stride
  assert a[off:off+254]==b[off:off+254],('sound parameters changed',k)
  if k!=5:assert a[off+st:off+st+ss*64]==b[off+st:off+st+ss*64],('other sequence changed',k)
 off=SYM['trk']+5*stride+st;assert a[off:off+ss*64]!=b[off:off+ss*64],'Break MIDI did not change'
for name,d in report['runs'].items():
 assert d['boots']==1 and d['fm1_mc_on']==1 and not any(d.get(k,0) for k in ['late','audio_lost','fm1_mc_lost','usb_underruns','usb_overruns','usb_missed']),(name,d)
 assert d['max_us']<HALF/44100*1e6,(name,d)
report['scope']='partial GEN controls and real-time generation while six-track transport runs; exact FWSC; no physical test'
(OUT/'target-emulator-results.json').write_text(json.dumps(report,indent=2)+'\n')
print('LIVE TARGET GEN: PASS',flush=True)
