#!/usr/bin/env python3
"""Replay the clean website save in the actual firmware, with no device writes."""
import base64,json,os,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/six'))
g={'__name__':'audit_web_start','__file__':str(root/'tools/six/test_rhythmer_emulator.py')}
exec((root/'tools/six/test_rhythmer_emulator.py').read_text().split("if sys.argv[1:]==['--nav-flex']:")[0],g)
o=g['OUT'];saved=json.loads(Path(sys.argv[1]).read_text())
assert saved['hash']==g['report']['firmware_sha256']
f=bytearray((o/'six-nav-fixture.flash').read_bytes())
for s in saved['sectors']:f[s['at']:s['at']+4096]=base64.b64decode(s['data'])
seed=o/'web-start.flash';seed.write_bytes(f)
ram=g['run']('web-start',8500,'2500:12,2620:12^',seed,True)
d=g['report']['runs']['web-start'];assert d['peak_pcm16']>0 and not(d['late'] or d['audio_lost'] or d['fm1_mc_lost'])
counts=[]
for k in range(6):
 off=g['SYM']['trk']+k*2772+1036
 # step_t: note[4], n, vel, time, probability, followed by motion data.
 count=sum(bool(ram[off+i*20+4]) and ram[off+i*20+5]==0 for i in range(64))
 counts.append(count)
assert counts==[0,0,0,0,0,16],counts
g['report']['note_counts']=counts
(o/'web-start-validation.json').write_text(json.dumps(g['report'],indent=2))
print('Web start native playback PASS:',counts)
