#!/usr/bin/env python3
"""Execute actual SIX/JIANT FWSC in the community emulator, without physical device I/O."""
import array,base64,hashlib,json,os,re,struct,subprocess,sys,wave,zlib
from pathlib import Path
from sample_assets import sample_dir
ROOT=Path(__file__).resolve().parents[2];OUT=Path(os.environ.get('RHYTHMER_BUILD_DIR',ROOT/('rhythmer/build-ssd' if (ROOT/'rhythmer/build-ssd').exists() else 'rhythmer/build')));FW=OUT/'felucca.fwsc'
SAMPLES=sample_dir(ROOT)
EMU='/Volumes/SSD/Development/Toolchains/fm1-six-tooling/fm1-emu-0.4.0-macos-arm64/fm1-live'
SYM={m[2]:int(m[1],16)-0x1c00000 for m in re.finditer(r'^([0-9a-f]+).*?\s([A-Za-z_][A-Za-z_0-9]*)$',(OUT/'felucca.sym').read_text(),re.M)}
HALF=int(re.search(r'^#define HALF_FRAMES (\d+)',(ROOT/'rhythmer/firmware/src/core.h').read_text(),re.M)[1])
assert re.search(r'\b%08x\s+abuf$'%(HALF*16),(OUT/'felucca.sym').read_text(),re.M),'FWSC DMA buffer differs from source half-frame setting'
report={'half_frames':HALF,'sample_manifest':json.loads((SAMPLES/'break-samples.json').read_text()),'firmware_sha256':hashlib.sha256(FW.read_bytes()).hexdigest(),'physical_hardware_tested':False,'runs':{}}
def validate(report):
 assert report["firmware_sha256"]==hashlib.sha256(FW.read_bytes()).hexdigest(),"stale firmware report"
 if report.get('scope','').startswith('navigation'):
  for n,d in report['runs'].items():
   assert d['boots']==1 and d['fm1_mc_on']==1 and not(d['late'] or d['audio_lost'] or d['fm1_mc_lost']),(n,d)
  for key in ('strict','sixacid','fiveacid-break'):
   d=report['runs']['six-nav-'+key]
   assert d['peak_pcm16']>0 and d['max_us']<HALF/44100*1e6 and d['usb_capture_frames']>1000 and not(d['usb_underruns'] or d['usb_overruns'] or d['usb_missed']),(key,d)
  assert report['runs']['six-nav-boot']['six_machine_mask']==3
  print('Rhythmer focused emulator validation PASS',flush=True);return
 # Fail CI/rechecks on faults, deadline misses, missing tracks or silent playback.
 for name,d in report['runs'].items():
  assert d['boots']==1 and d['fm1_mc_on']==1, (name,d)
  assert d['late']==0 and d['audio_lost']==0 and d['fm1_mc_lost']==0, (name,d)
  if name!='six-current-boot' and not name.startswith('rhythmer-splash'):assert d['six_loop_mask']==3,(name,d)
  if name in ('six-current-break','six-current-strict'):assert d['peak_pcm16']>0,(name,d)

 assert report["runs"]["six-current-strict"]["max_us"] < HALF/44100*1e6, "audio render deadline exceeded"

 assert report["runs"]["six-current-four"]["max_us"] < HALF/44100*1e6
 assert report["runs"]["six-current-four"]["peak_pcm16"] > 0

 assert report['runs']['six-current-four']['six_machine_mask']==0
 assert report['runs']['six-current-strict']['six_machine_mask']==3

 for n in ('six-current-strict','six-current-usb48','six-current-four','six-current-triple-serum','six-current-triple-m2stat','six-current-triple-m2saw','six-current-triple-m2sid'):
  d=report['runs'][n]
  assert d['usb_capture_frames']>1000 and d['usb_peak_pcm16']>0,(n,d)
  assert d['usb_starts']>0 and not d['usb_underruns'] and not d['usb_overruns'],(n,d)
 for case,engines in [('sixacid',[16]*6),('fiveacid-break',[15]+[16]*5)]:
  d=report['runs']['six-current-'+case]
  assert d['track_engines']==engines and d['max_us']<HALF/44100*1e6 and d['peak_pcm16']>0,(case,d)
  assert d['usb_capture_frames']>1000 and not(d['usb_underruns'] or d['usb_overruns'] or d['usb_missed']),(case,d)
 assert report['runs']['six-current-triple-serum']['six_machine_mask']==3
 assert report['runs']['six-current-triple-serum']['max_us']<HALF/44100*1e6
 for model in ('m2stat','m2saw','m2sid'):
  d=report['runs']['six-current-triple-'+model];assert d['six_machine_mask']==3 and d['max_us']<HALF/44100*1e6 and d['peak_pcm16']>0
 if 'six-current-audition' in report['runs']:
  d=report['runs']['six-current-audition'];assert d['six_machine_mask']==3 and d['peak_pcm16']>0
  assert d['usb_capture_frames']>1000 and not(d['usb_underruns'] or d['usb_overruns'] or d['usb_missed'])
 for model,track in [('serum',1),('phase',2),('lofi',3)]:
  for name in ('six-current-edit-'+model,'rhythmer-note-'+model+'-hit','rhythmer-note-'+model+'-release'):
   if name in report['runs']:assert report['runs'][name].get('selected_track')==track,(name,'wrong model selected')
 print("Rhythmer emulator validation PASS",flush=True)

def run(name,ms,keys='',flash=None,strict=False,usb_rate=44100):
 assert hashlib.sha256(FW.read_bytes()).hexdigest()==report['firmware_sha256'],'Firmware changed during validation; finish build before testing'
 env=dict(os.environ,FM1_BENCH_KEYS=keys,FM1_BENCH_PANEL=str(OUT/(name+'.ppm')),FM1_DUMPRAM=str(OUT/(name+'.ram')),FM1_BENCH_WAV=str(OUT/(name+'.pcm')))
 if strict:
  env['FM1_STRICT']='1';env['FM1_USB_WAV']=str(OUT/(name+'.usb.pcm'));env['FM1_USB_RATE']=str(usb_rate)
 cmd=[EMU,'--fwsc',str(FW),'--bench',str(ms),'--midi','none','--usb']
 if flash:cmd+=['--flash',str(flash)]
 with (OUT/(name+'.log')).open('w') as f:subprocess.run(cmd,env=env,stdout=f,stderr=subprocess.STDOUT,check=True,timeout=600)
 assert hashlib.sha256(FW.read_bytes()).hexdigest()==report['firmware_sha256'],'Firmware changed while the emulator was running; discard this run'
 log=(OUT/(name+'.log')).read_text();assert 'halt:' not in log,log
 ram=(OUT/(name+'.ram')).read_bytes();v=struct.unpack_from('<19I',ram,SYM['felucca_dbg']);d=dict(zip(['magic','halves','max_us','nested','in_audio','late','timer_irqs','ui_frames','last_us','cpu_q8','boots','stage','page','home'],v))
 for sym in ['six_loop_mask','six_machine_mask','fm1_mc_on','fm1_mc_lost']:d[sym]=ram[SYM[sym]]
 if 'mc_mask_last' in SYM:d['core1_track_mask']=ram[SYM['mc_mask_last']]&63
 layout=OUT/'six-model-layout.json'
 if layout.exists():
  offsets=json.loads(layout.read_text());d['selected_track']=ram[SYM['song']+offsets['song_sel']]
  if 'engine' in offsets:
   d['track_engines']=[ram[SYM['trk']+i*offsets['track_stride']+offsets['engine']] for i in range(6)]
 if 'uac' in SYM:
  d.update(dict(zip(['usb_starts','usb_packets','usb_frames','usb_underruns','usb_overruns','usb_missed'],struct.unpack_from('<6I',ram,SYM['uac']+40))))
 usb=OUT/(name+'.usb.pcm')
 if strict:
  data=usb.read_bytes() if usb.exists() else b'';d['usb_capture_frames']=len(data)//4;d['usb_peak_pcm16']=max(map(abs,array.array('h',data)),default=0);d['usb_rate']=usb_rate
 d['audio_lost']=struct.unpack_from('<I',ram,SYM['audio_lost'])[0]
 if 'audio_guard' in SYM:
  d['audio_guard_hits'],d['audio_guard_frames']=struct.unpack_from('<2I',ram,SYM['audio_guard']+4)
  d['adaptive_voice_cap']=ram[SYM['gov_cap']]
 pcm=(OUT/(name+'.pcm')).read_bytes();d['peak_pcm16']=max(map(abs,array.array('h',pcm)),default=0)
 with wave.open(str(OUT/(name+'.wav')),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(44100);w.writeframes(pcm)
 report['runs'][name]=d;print(name,d,flush=True);return ram
if sys.argv[1:] in (['--upgrade'],['--upgrade-ensemble']):
 subset=sys.argv[1:]==['--upgrade-ensemble']
 if subset:
  previous=json.loads((OUT/'upgrade-emulator-results.json').read_text());assert previous['firmware_sha256']==report['firmware_sha256'];report=previous
 base=OUT/('upgrade-base-'+report['firmware_sha256'][:12]+'.flash')
 run('upgrade-boot',2800,flash=base,strict=True)
 d=report['runs']['upgrade-boot'];assert d['six_loop_mask']==3 and d['peak_pcm16']==0,d
 original=base.read_bytes();flash=bytearray(original)
 def put(off,kind,data):
  h=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(data),zlib.crc32(data),0xffffffff,0xffffffff);h+=struct.pack('<I',zlib.crc32(h))
  sec=bytearray(b'\xff'*4096);sec[:32]=h;sec[256:256+len(data)]=data;flash[off:off+4096]=sec
 # Read the banks installed by the actual package, rather than injecting them.
 for k,label in enumerate(('amen-160','think-160')):
  slot=(SAMPLES/(label+'.slot')).read_bytes();off=0xa0000+k*0x14000
  assert flash[off:off+len(slot)]==slot,('embedded sample mismatch',label)
 settings=bytearray(flash[0xfc100:0xfc100+572]);settings[52+15*32+30]|=128
 put(0xfc000,0,settings) # test fixtures explicitly opt into RESTORE LAST
 seed=OUT/'upgrade-fixture.flash'
 for project in sorted(OUT.glob('upgrade-ensemble-*.project' if subset else 'upgrade-*.project')):
  data=project.read_bytes()
  for off,kind in ((0x97000,1),(0xe5000,9)):put(off,kind,data)
  seed.write_bytes(flash);name=project.stem
  run(name,6500,'2500:12,2620:12^',seed,True)
  d=report['runs'][name];assert d['peak_pcm16']>0,(name,'silent')
  assert d['max_us']<HALF/44100*1e6,(name,'render deadline')
  assert not any(d.get(k,0) for k in ('late','audio_lost','fm1_mc_lost','usb_underruns','usb_overruns','usb_missed')),(name,d)
 # Empty browser session, using the tested package and factory-installed banks.
 data=(OUT/'web-start.project').read_bytes()
 for off,kind in ((0x97000,1),(0xe5000,9)):put(off,kind,data)
 sectors=[{'at':i,'data':base64.b64encode(bytes(flash[i:i+4096])).decode()} for i in range(0,len(flash),4096) if flash[i:i+4096]!=bytes([255])*4096 and i>=0x97000]
 saved={'format':'fm1-emulator-save','version':1,'hash':report['firmware_sha256'],'name':'Rhythmer — Empty start','package':base64.b64encode(FW.read_bytes()).decode(),'sectors':sectors,'startingProject':{'revision':2,'tracksWithNotes':[],'breakSource':'A / Amen'}}
 (OUT/'Rhythmer-start.fm1save').write_text(json.dumps(saved)+'\n')
 report['scope']='next-version actual package; all requested genre, Acid, MD fixtures and embedded banks; no physical test'
 (OUT/'upgrade-emulator-results.json').write_text(json.dumps(report,indent=2)+'\n')
 print('Next-version firmware emulator PASS',flush=True);sys.exit(0)

if sys.argv[1:]==['--nav-flex']:
 import time
 # A virgin file matters: old base dumps contain track-count preferences.
 base=OUT/('nav-'+report['firmware_sha256'][:8]+'-'+str(time.time_ns())+'.flash')
 run('six-nav-boot',2200,flash=base)
 assert report['runs']['six-nav-boot']['six_machine_mask']==3,'fresh settings must enable six tracks'
 flash=bytearray(base.read_bytes());original=bytes(flash)
 for k,label in enumerate(('amen-160','think-160')):
  data=(SAMPLES/(label+'.slot')).read_bytes();off=0xa0000+k*0x14000;flash[off:off+len(data)]=data
 def inject(off,kind,data):
  h=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(data),zlib.crc32(data),0xffffffff,0xffffffff);h+=struct.pack('<I',zlib.crc32(h))
  sec=bytearray([255])*4096;sec[:32]=h;sec[256:256+len(data)]=data;flash[off:off+4096]=sec
 settings=bytearray(572);struct.pack_into('<IIII',settings,0,0x50455234,0,0,0);struct.pack_into('<I',settings,16,0x50414e35)
 settings[20:34]=bytes([2,3,4,5,6,7,8,9,10,11,12,13,0,1]);settings[34:41]=bytes([0,1,6,2,3,4,5]);settings[41:48]=bytes([1]*7)
 settings[52+15*32+15]=0x51;settings[52+15*32+17]=4;settings[52+15*32+28]=128;settings[52+15*32+30]=128
 inject(0xfc000,0,settings)
 seed=OUT/'six-nav-fixture.flash'
 def fixture(name):
  data=(OUT/name).read_bytes()
  for off,kind in ((0x97000,1),(0xe5000,9)):inject(off,kind,data)
  seed.write_bytes(flash)
 for case,project,engines in [('strict','six-demo.project',[10,17,2,3,16,15]),('sixacid','six-sixacid-stress.project',[16]*6),('fiveacid-break','six-fiveacid-break-stress.project',[15]+[16]*5)]:
  fixture(project);run('six-nav-'+case,8000 if case=='strict' else 6000,'2500:12,2620:12^',seed,True)
  d=report['runs']['six-nav-'+case];assert d['track_engines']==engines and d['max_us']<HALF/44100*1e6 and d['peak_pcm16']>0,(case,d)
  assert d['usb_capture_frames']>1000 and not(d['usb_underruns'] or d['usb_overruns'] or d['usb_missed']),(case,d)
 fixture('six-audition.project')
 sectors=[{'at':i,'data':base64.b64encode(bytes(flash[i:i+4096])).decode()} for i in range(0,len(flash),4096) if flash[i:i+4096]!=original[i:i+4096]]
 saved={'format':'fm1-emulator-save','version':1,'hash':report['firmware_sha256'],'name':'Rhythmer - six track test','package':base64.b64encode(FW.read_bytes()).decode(),'sectors':sectors}
 (OUT/'Rhythmer-six-track-test.fm1save').write_text(json.dumps(saved))
 # Real firmware page IDs are stable because no PAGES entries were inserted.
 titles=re.findall(r'^\s*\{"([^"]+)",\s*FAM_', (ROOT/'rhythmer/firmware/src/params.c').read_text().split('static const page_t PAGES[] = {',1)[1],re.M)
 for kind,track,custom in [('acid',4,['303 GEN 1','303 GEN 2']),('break',5,['BRK PERF','BRK SEQ','BRK RETURN'])]:
  turns=','.join(f'{2400+i*250}:e1:1' for i in range(track))
  start=3700 if track==4 else 4000
  for page,title in enumerate(custom):
   fixture('six-audition.project');keys=turns
   for j in range(page+2):keys+=f',{start+j*400}:6,{start+j*400+120}:6^'
   name='six-nav-'+kind+'-'+str(page+1)
   run(name,start+(page+1)*400+450,keys,seed)
   d=report['runs'][name];assert d['selected_track']==track and d['page']==titles.index(title),(name,title,d)
 # The ordinary demo retains explicit 4-track mode; the test fixture defaults to six.
 fixture('six-mono.project');settings[52+15*32+15]=0x50;inject(0xfc000,0,settings)
 saved['name']='Rhythmer - four track demo';saved['sectors']=[{'at':i,'data':base64.b64encode(bytes(flash[i:i+4096])).decode()} for i in range(0,len(flash),4096) if flash[i:i+4096]!=original[i:i+4096]]
 (OUT/'Rhythmer-demo.fm1save').write_text(json.dumps(saved))
 for name,d in report['runs'].items():
  assert d['boots']==1 and d['fm1_mc_on']==1 and not(d['late'] or d['audio_lost'] or d['fm1_mc_lost']),(name,d)
 report['scope']='navigation and flexible-machine focused validation; earlier DSP sweep is archived separately'
 (OUT/'six-current-results.json').write_text(json.dumps(report,indent=2))
 print('Rhythmer machine navigation and flexible-track emulator validation PASS',flush=True);sys.exit(0)

if sys.argv[1:]==['--verify-only']:
 report=json.loads((OUT/'six-current-results.json').read_text());validate(report);sys.exit(0)
if sys.argv[1:]==['--garden-views']:
 report=json.loads((OUT/'six-current-results.json').read_text());validate(report)
 saved=json.loads((OUT/'Rhythmer-six-track-test.fm1save').read_text());assert saved['hash']==report['firmware_sha256']
 base=OUT/('six-base-'+report['firmware_sha256'][:8]+'.flash')
 if not base.exists():base=OUT/'six-current-base.flash'
 flash=bytearray(base.read_bytes())
 for s in saved['sectors']:flash[s['at']:s['at']+4096]=base64.b64decode(s['data'])
 seed=OUT/'six-current-garden-views.flash'
 for label,track in [('serum',1),('phase',2),('lofi',3)]:
  turns=','.join(f'{2500+i*350}:e1:1' for i in range(track))
  for suffix,ms in [('hit',4200),('release',5500)]:
   seed.write_bytes(flash);n='rhythmer-note-'+label+'-'+suffix
   run(n,ms,turns+',4000:n0,4500:n0^',seed);assert report['runs'][n]['selected_track']==track,(n,report['runs'][n])
  seed.write_bytes(flash);n='six-current-edit-'+label
  run(n,6000,turns+',3600:6,3720:6^,4200:12,4320:12^',seed);assert report['runs'][n]['selected_track']==track
 (OUT/'six-current-results.json').write_text(json.dumps(report,indent=2));validate(report);sys.exit(0)
if sys.argv[1:]==['--mono-views']:
 report=json.loads((OUT/'six-current-results.json').read_text());validate(report)
 saved=json.loads((OUT/'Rhythmer-six-track-test.fm1save').read_text());assert saved['hash']==report['firmware_sha256']
 base=OUT/('six-base-'+report['firmware_sha256'][:8]+'.flash')
 if not base.exists():base=OUT/'six-current-base.flash'
 flash=bytearray(base.read_bytes())
 for sec in saved['sectors']:flash[sec['at']:sec['at']+4096]=base64.b64decode(sec['data'])
 initial=bytearray(flash);seed=OUT/'six-current-mono-views.flash'
 def inject(data):
  for off,kind in ((0x97000,1),(0xE5000,9)):
   h=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(data),zlib.crc32(data),0xffffffff,0xffffffff);h+=struct.pack('<I',zlib.crc32(h))
   sec=bytearray([255])*4096;sec[:32]=h;sec[256:256+len(data)]=data;flash[off:off+4096]=sec
 for label,engine in [('m2stat',18),('m2saw',19),('m2sid',20)]:
  flash=bytearray(initial);inject((OUT/('six-'+label+'-stress.project')).read_bytes());seed.write_bytes(flash)
  name='six-current-edit-'+label;run(name,6500,'2500:e1:1,3400:6,3520:6^,4200:12,4320:12^',seed)
  assert report['runs'][name]['selected_track']==1 and report['runs'][name]['track_engines'][1:4]==[engine]*3,(name,report['runs'][name])
 flash=bytearray(initial);seed.write_bytes(flash)
 acid_turns=','.join(f'{2500+i*350}:e1:1' for i in range(4))
 run('six-current-acid-scl',6500,acid_turns+',4000:6,4120:6^,4500:e0:1,4800:e0:1,5400:1,5520:1^',seed)
 assert report['runs']['six-current-acid-scl']['selected_track']==4
 flash=bytearray(initial);seed.write_bytes(flash)
 turns=','.join(f'{2500+i*350}:e1:1' for i in range(5))
 run('six-current-brkstep',6000,turns+',4300:11,4420:11^,4700:e3:13,5000:e3:13,5300:e4:3',seed)
 assert report['runs']['six-current-brkstep']['selected_track']==5
 offsets=json.loads((OUT/'six-model-layout.json').read_text());ram=(OUT/'six-current-brkstep.ram').read_bytes()
 assert ram[SYM['trk']+5*offsets['track_stride']+offsets['step']+offsets['step_note']]==26,'BRK STEP did not write UP'
 flash=bytearray(initial);seed.write_bytes(flash)
 run('six-current-brkstep-keys',6500,turns+',4300:11,4420:11^,4700:13,4820:13^,5300:n7,5500:n7^',seed)
 ram=(OUT/'six-current-brkstep-keys.ram').read_bytes()
 assert ram[SYM['trk']+5*offsets['track_stride']+offsets['step']+offsets['step_note']]==4,'BRK STEP stopped keys did not write chop 5'

 flash=bytearray(initial);seed.write_bytes(flash)
 run('six-current-brkplay-rec',9500,turns+',4300:11,4420:11^,4900:e0:1,5500:12,5620:12^,6000:13,6120:13^,6600:n24,7100:n22,7220:n22^,7600:n20,7720:n20^,8300:n24^',seed)
 ram=(OUT/'six-current-brkplay-rec.ram').read_bytes();at=SYM['trk']+5*offsets['track_stride']+offsets['step']
 events=[ram[at+i*offsets['step_stride']+offsets['step_note']] for i in range(16)]
 assert any(16<=v<32 for v in events),('no recorded performance',events)
 (OUT/'six-current-results.json').write_text(json.dumps(report,indent=2));validate(report);sys.exit(0)
if sys.argv[1:]==['--audition']:
 report=json.loads((OUT/'six-current-results.json').read_text());validate(report)
 saved=json.loads((OUT/'Rhythmer-six-track-test.fm1save').read_text());assert saved['hash']==report['firmware_sha256']
 base=OUT/('six-base-'+report['firmware_sha256'][:8]+'.flash')
 if not base.exists():base=OUT/'six-current-base.flash'
 flash=bytearray(base.read_bytes())
 for s in saved['sectors']:flash[s['at']:s['at']+4096]=base64.b64decode(s['data'])
 payload=(OUT/'six-audition.project').read_bytes();assert len(payload)<=3840
 for off,kind in ((0x97000,1),(0xE5000,9)):
  h=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(payload),zlib.crc32(payload),0xffffffff,0xffffffff);h+=struct.pack('<I',zlib.crc32(h))
  sector=bytearray([255])*4096;sector[:32]=h;sector[256:256+len(payload)]=payload;flash[off:off+4096]=sector
  entry=next(s for s in saved['sectors'] if s['at']==off);entry['data']=base64.b64encode(sector).decode()
 seed=OUT/'six-current-audition.flash';seed.write_bytes(flash)
 run('six-current-audition',8000,'1200:12,1320:12^',seed,True)
 validate(report)
 (OUT/'Rhythmer-six-track-test.fm1save').write_text(json.dumps(saved))
 (OUT/'six-current-results.json').write_text(json.dumps(report,indent=2));sys.exit(0)
base=OUT/'six-current-base.flash'
if base.exists():base=OUT/('six-base-'+report['firmware_sha256'][:8]+'.flash')
run('six-current-boot',2500,flash=base)
run('rhythmer-splash',1200,flash=base)
run('rhythmer-splash-b',1400,flash=base)
run('rhythmer-splash-c',1600,flash=base)
flash=bytearray(base.read_bytes());original=bytes(flash)
for k in range(2):
 slot=(SAMPLES/(('amen-160' if k==0 else 'think-160')+'.slot')).read_bytes();off=0xA0000+k*0x14000;flash[off:off+len(slot)]=slot
payload=(OUT/'six-demo.project').read_bytes()
def record(off,kind,data=None):
 global payload
 if data is not None:old=payload;payload=data
 h=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(payload),zlib.crc32(payload),0xffffffff,0xffffffff);h+=struct.pack('<I',zlib.crc32(h));flash[off:off+4096]=bytes([255])*4096;flash[off:off+32]=h;flash[off+256:off+256+len(payload)]=payload
 if data is not None:payload=old
record(0x97000,1);record(0xE5000,9)
# PER4: 16-byte header + 32-byte panel + 4-byte hold setting, then favorites.
settings=bytearray(572);struct.pack_into('<IIII',settings,0,0x50455234,0,0,0)
struct.pack_into('<I',settings,16,0x50414e35)
settings[20:34]=bytes([2,3,4,5,6,7,8,9,10,11,12,13,0,1]);settings[34:41]=bytes([0,1,6,2,3,4,5]);settings[41:48]=bytes([1]*7)
settings[52+15*32+17]=4;settings[52+15*32+28]=128;settings[52+15*32+30]=128
record(0xFC000,0,settings)
seed=OUT/'six-current-demo.flash';seed.write_bytes(flash)
# UI generator, then performance view. ALGO increments are spread across scans.
run('six-current-home',3000,flash=seed)
seed.write_bytes(flash)
run('six-current-gen',4500,'1200:6,1320:6^,1700:e0:1,2000:e0:1,2500:1,2620:1^,3100:0,3220:0^',seed)
seed.write_bytes(flash)
run('six-current-gen-animation',3300,'1200:6,1320:6^,1700:e0:1,2000:e0:1,3100:1,3220:1^',seed)
seed.write_bytes(flash)
run('six-current-gen-animation-b',3500,'1200:6,1320:6^,1700:e0:1,2000:e0:1,3100:1,3220:1^',seed)
seed.write_bytes(flash)
run('six-current-gen-animation-c',3800,'1200:6,1320:6^,1700:e0:1,2000:e0:1,3100:1,3220:1^',seed)
seed.write_bytes(flash)
run('six-current-gen-scale',4000,'1200:6,1320:6^,1700:e0:1,2000:e0:1,2300:e0:1',seed)
seed.write_bytes(flash)
run('six-current-break',6500,'1200:e1:1,1500:6,1620:6^,1900:e0:1,2200:e0:1,2500:12,2620:12^,3000:n26,4100:n26^,4400:13,4520:13^,4800:n22,4920:n22^,5100:n20,5220:n20^,5400:n25,5520:n25^,5800:13,5920:13^',seed)
seed.write_bytes(flash)
run('six-current-return',6000,'1200:e1:1,1500:6,1620:6^,1900:e0:1,2200:e0:1,2500:e2:70,2700:e3:80,3000:e0:1,3300:e0:1,4000:1,4120:1^',seed)
seed.write_bytes(flash)
run('six-current-break-up',5000,'1200:e1:1,1500:6,1620:6^,1900:e0:1,2200:e0:1,2500:12,2620:12^,3300:n22,3420:n22^,3800:n24',seed)
# Rhythmer animation sites: HOME grid/focus, each synth EDIT, LFO2 and original FX key map.
seed.write_bytes(flash)
run('six-current-playing-home',4500,'1200:12,1320:12^',seed)
seed.write_bytes(flash)
run('rhythmer-home-b',4700,'1200:12,1320:12^',seed)
seed.write_bytes(flash)
run('rhythmer-home-c',4900,'1200:12,1320:12^',seed)
seed.write_bytes(flash)
run('six-current-focus',4500,'1200:8,1320:8^,1600:12,1720:12^',seed)
# Model-specific EDIT and note captures run after audition creation, with input
# scheduled beyond startup and selected-track assertions (--garden-views).
seed.write_bytes(flash)
run('six-current-fx-guide',5000,'1200:12,1320:12^,1800:2,2600:n0',seed)
seed.write_bytes(flash)
run('six-current-lfo2',4200,'1200:5,1320:5^,2100:e0:-2,2700:e2:20,3000:e3:2',seed)
seed.write_bytes(flash)
run('six-current-machines',4600,'1200:7,1320:7^,1700:e0:1,1950:e0:1,2200:e0:1,2450:e0:1,2700:e0:1,3100:e2:-3,3400:e2:1,3700:e2:1,4000:e2:1',seed)
# All populated tracks with all four sends, USB audio and live break rolls; timed LCD/cache model.
seed.write_bytes(flash)
run('six-current-strict',24000,'1200:12,1320:12^,1800:e1:1,2100:6,2220:6^,2500:e0:1,2800:e0:1,3000:e0:1,3200:e2:-1,3400:e0:-1,3600:e2:80,3700:e3:96,3800:e4:90,4200:n23,5200:n23^,7000:n22,7120:n22^,8000:n26,9500:n26^,10000:n24,11000:n25,11120:n25^,13000:n24^,14000:n22,14120:n22^,17000:n26,18000:n20,18120:n20^,19000:n26^,22000:8,22120:8^',seed,True)
seed.write_bytes(flash)
run('six-current-usb48',8000,'1200:12,1320:12^',seed,True,48000)
stress=bytearray(flash)
record(0x97000,1,(OUT/'six-serum-stress.project').read_bytes());record(0xE5000,9,(OUT/'six-serum-stress.project').read_bytes())
seed.write_bytes(flash);run('six-current-triple-serum',12000,'1200:12,1320:12^',seed,True)
flash=stress
for model in ('m2stat','m2saw','m2sid'):
 snapshot=bytearray(flash)
 record(0x97000,1,(OUT/('six-'+model+'-stress.project')).read_bytes());record(0xE5000,9,(OUT/('six-'+model+'-stress.project')).read_bytes())
 seed.write_bytes(flash);run('six-current-triple-'+model,10000,'2500:12,2620:12^',seed,True)
 flash=snapshot
for case in ('sixacid','fiveacid-break'):
 snapshot=bytearray(flash)
 record(0x97000,1,(OUT/('six-'+case+'-stress.project')).read_bytes());record(0xE5000,9,(OUT/('six-'+case+'-stress.project')).read_bytes())
 seed.write_bytes(flash);run('six-current-'+case,12000,'2500:12,2620:12^',seed,True)
 flash=snapshot
# Review fixture: all six machines populated/enabled, stopped on HOME. Import is
# local to the emulator and starts a distinct save for this firmware hash.
settings[52+15*32+15]=0x51
record(0xFC000,0,settings)
six_sectors=[{'at':i,'data':base64.b64encode(bytes(flash[i:i+4096])).decode()} for i in range(0,len(flash),4096) if flash[i:i+4096]!=original[i:i+4096]]
(OUT/'Rhythmer-six-track-test.fm1save').write_text(json.dumps({'format':'fm1-emulator-save','version':1,'hash':report['firmware_sha256'],'name':'Rhythmer - six track test','package':base64.b64encode(FW.read_bytes()).decode(),'sectors':six_sectors}))
# User-facing initial project: four generated tracks, empty reserved acid/break.
payload=(OUT/'six-mono.project').read_bytes()
record(0x97000,1);record(0xE5000,9)
settings[52+15*32+15]=0x50
record(0xFC000,0,settings)
seed.write_bytes(flash)
run('six-current-four',8000,'1200:12,1320:12^',seed,True)
seed.write_bytes(flash)
run('six-current-layout',3500,flash=seed)
# A save image is a versioned emulator artifact, never a device flash dump to install.
sectors=[{'at':i,'data':base64.b64encode(bytes(flash[i:i+4096])).decode()} for i in range(0,len(flash),4096) if flash[i:i+4096]!=original[i:i+4096]]
(OUT/'Rhythmer-demo.fm1save').write_text(json.dumps({'format':'fm1-emulator-save','version':1,'hash':report['firmware_sha256'],'name':'Rhythmer - six track groovebox','package':base64.b64encode(FW.read_bytes()).decode(),'sectors':sectors}))
(OUT/'six-current-results.json').write_text(json.dumps(report,indent=2))

validate(report)
if (OUT/'six-audition.project').exists():
 subprocess.run([sys.executable,__file__,'--audition'],check=True)
 subprocess.run([sys.executable,__file__,'--garden-views'],check=True)
