#!/usr/bin/env python3
"""Dependency-free JS model/protocol tests through macOS JavaScriptCore + asset checks."""
import ctypes, hashlib, json, re, os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT/'site/src'
DIST=Path(os.environ.get('RHYTHMER_SITE_DIST',ROOT/'site/dist'))
jsc=ctypes.CDLL('/System/Library/Frameworks/JavaScriptCore.framework/JavaScriptCore')
ptr=ctypes.c_void_p
jsc.JSGlobalContextCreate.argtypes=[ptr];jsc.JSGlobalContextCreate.restype=ptr
jsc.JSStringCreateWithUTF8CString.argtypes=[ctypes.c_char_p];jsc.JSStringCreateWithUTF8CString.restype=ptr
jsc.JSEvaluateScript.argtypes=[ptr,ptr,ptr,ptr,ctypes.c_int,ctypes.POINTER(ptr)];jsc.JSEvaluateScript.restype=ptr
jsc.JSValueToStringCopy.argtypes=[ptr,ptr,ctypes.POINTER(ptr)];jsc.JSValueToStringCopy.restype=ptr
jsc.JSStringGetMaximumUTF8CStringSize.argtypes=[ptr];jsc.JSStringGetMaximumUTF8CStringSize.restype=ctypes.c_size_t
jsc.JSStringGetUTF8CString.argtypes=[ptr,ctypes.c_char_p,ctypes.c_size_t];jsc.JSStringGetUTF8CString.restype=ctypes.c_size_t
jsc.JSStringRelease.argtypes=[ptr];jsc.JSGlobalContextRelease.argtypes=[ptr]
ctx=jsc.JSGlobalContextCreate(None)
def run(code):
 s=jsc.JSStringCreateWithUTF8CString(code.encode());error=ptr()
 value=jsc.JSEvaluateScript(ctx,s,None,None,1,ctypes.byref(error));jsc.JSStringRelease(s)
 value=error.value or value
 out=jsc.JSValueToStringCopy(ctx,value,None);n=jsc.JSStringGetMaximumUTF8CStringSize(out);buf=ctypes.create_string_buffer(n);jsc.JSStringGetUTF8CString(out,buf,n);jsc.JSStringRelease(out)
 if error.value:raise AssertionError(buf.value.decode())
 return buf.value.decode()
model=(SRC/'model.mjs').read_text().replace('export ','')
midi=(SRC/'midi.mjs').read_text().replace('export ','')
app=re.sub(r'^import .*?;\n','',(SRC/'app.js').read_text(),flags=re.M)
run('async function syntaxCheck(){\n'+app+'\n}; "app syntax OK"')
for filename in ['main.js','worklet.js']:
    runtime=re.sub(r'^import .*?;\n','',(DIST/'emulator'/filename).read_text(),flags=re.M)
    run('async function runtimeSyntax(){\n'+runtime+'\n}; \"runtime syntax OK\"')
device=(SRC/'device.mjs').read_text().replace('export ','')
run(device+'\nconst markup=fm1Device(\"assets/home.png\"); if((markup.match(/data-note-key/g)||[]).length!==27)throw Error(\"FM-1 needs 27 keys\"); if((markup.match(/class=\"fm1-knob\"/g)||[]).length!==8)throw Error(\"FM-1 needs 8 knobs including MASTER\"); \"FM-1 panel geometry OK\";')
result=run(model+'\n'+midi+'''\n
const checks=[];function check(ok,label){if(!ok)throw Error(label);checks.push(label);}
let p=makeProject();check(validateProject(p).tracks.length===6,'default project round trip');
check(p.mode===6&&p.tracks.map(t=>t.model).join(',')==='DRUM,SERUM,PHASE,LOFI,ACID,BREAK','default track layout');
check(!availableModels(p,0).includes('BREAK'),'hide owned Break');
let rejected=false;try{setModel(p,0,'BREAK');}catch{rejected=true;}check(rejected,'reject second Break');
setModel(p,5,'SERUM');check(availableModels(p,0).includes('BREAK'),'release Break ownership');
setModel(p,0,'BREAK');check(!availableModels(p,5).includes('BREAK'),'move Break ownership');
setModel(p,1,'ACID');setModel(p,2,'ACID');check(p.tracks.filter(t=>t.model==='ACID').length===3,'multiple Acid allowed');
p.mode=4;check(!availableModels(p,3).includes('BREAK'),'4-track mode preserves Break ownership');
const before=JSON.stringify(p.sections[0][5]);p.mode=6;check(JSON.stringify(p.sections[0][5])===before,'mode switch retains inactive pattern');
const q=validateProject(p);q.sections[0][0][0].note=60;check(p.sections[0][0][0].note!==60,'import/save deep copy');
for(let root=0;root<12;root++)for(const scale of Object.keys(SCALES)){p.root=root;p.scale=scale;randomDraft(p,1,0,()=>.6);check(p.sections[0][1].every(s=>SCALES[scale].includes((s.note-root)%12)),'scale draft '+root+' '+scale);}
for(const mutate of [x=>x.tracks.push({}),x=>x.tracks[0].model='UNKNOWN',x=>x.tracks[5].model='BREAK',x=>x.sections[0][0][0].command='<script>',x=>x.sections[0][0][0].velocity=999,x=>x.sections[0][0].push({}),x=>x.scale='__proto__']){let bad=makeProject();bad.tracks[0].model='BREAK';bad.tracks[5].model='SERUM';mutate(bad);let threw=false;try{validateProject(bad);}catch{threw=true;}check(threw,'reject malformed draft');}
let raw=JSON.parse(JSON.stringify(makeProject()));raw.__proto_test='unwanted';check(!Object.hasOwn(validateProject(raw),'__proto_test'),'whitelist imported fields');
function infoFrame(name,n=21,tracks=6){return [240,125,70,76,1,...Array.from(name,c=>c.charCodeAt(0)),0,n,60,30,64,20,...Array.from({length:n},()=>[65,0]).flat(),tracks,247];}
check(parseInfo(infoFrame('Rhythmer dev')).isRhythmer,'Rhythmer data-engine identity');
check(parseInfo(infoFrame('Rhythmer v0.1.1-beta3')).isRhythmer,'Rhythmer release identity');
check(parseInfo(infoFrame('JIANT Rhythmer dev')).isRhythmer,'current firmware identity');
check(parseInfo(infoFrame('JIANT v0.1.1-beta3')).isRhythmer,'release build identity');
check(!parseInfo(infoFrame('JIANT v0.1.1-beta3',20)).isRhythmer,'release still rejects wrong engine count');
check(!parseInfo(infoFrame('JIANT v9.9')).isRhythmer,'reject unreviewed generic JIANT release');
check(!parseInfo(infoFrame('JIANT 1.1')).isRhythmer,'reject other firmware identity');
check(!parseInfo(infoFrame('JIANT Rhythmer dev',20)).isRhythmer,'reject wrong engine count');
check(parseInfo([240,125,70,76,1,65,247])===null,'reject truncated INFO');
JSON.stringify({passed:checks.length,checks});
''')
report=json.loads(result)
assert 'requestMIDIAccess' not in (SRC/'app.js').read_text().split('async function wireInstall')[0]
assert '.send([0xf0,0x7d,0x46,0x4c,1,0xf7])' in (SRC/'midi.mjs').read_text()
assert (SRC/'midi.mjs').read_text().count('output.send(')==1
config=json.loads((DIST/'config.json').read_text());assert isinstance(config['localOnly'],bool) and config['links']['kofi']=='https://ko-fi.com/jazzyrhythmer'
release=json.loads((DIST/'release.json').read_text())
for f in release['files']:
 p=DIST/'downloads'/f['name'];assert p.stat().st_size==f['bytes'];assert hashlib.sha256(p.read_bytes()).hexdigest()==f['sha256']
presets=json.loads((DIST/'presets.json').read_text());assert {k:len(v) for k,v in presets.items()}=={'SERUM':8,'PHASE':16,'LOFI':17,'FM+STAT':5,'SWAV-SAW':6,'SID-6581':6}
for name in ['home.png','acid-1.png','acid-2.png','break-1.png','break-2.png','break-3.png','LICENSE.txt','favicon.svg']:assert (DIST/'assets'/name).is_file()
report['static_checks']='assets, canonical hashes, all 58 selected synth preset names, explicit local/public config, INFO-only MIDI sender'
(Path('/Volumes/SSD/Development/TestResults/rhythmer/installer')/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS:',report['passed'],'JavaScript checks + static package checks')
jsc.JSGlobalContextRelease(ctx)

import base64
seed=json.loads((DIST/'downloads/Rhythmer-start.fm1save').read_text())
assert seed['startingProject']['tracksWithNotes']==[] and seed['startingProject']['unassignedTracks']==[5,6]
assert hashlib.sha256(base64.b64decode(seed['package'])).hexdigest()==release['sha256']
sectors={s['at']:base64.b64decode(s['data']) for s in seed['sectors']}
# Banks may be embedded in flash.bin rather than redundant save overrides.
import importlib.util,struct
spec=importlib.util.spec_from_file_location('site_pkg',ROOT/'rhythmer/tools/fm1pkg_make.py');pkg=importlib.util.module_from_spec(spec);spec.loader.exec_module(pkg)
raw=base64.b64decode(seed['package']);logical=b''.join(raw[i*48:i*48+47] for i in range(20))+raw[960:]
h=bytearray(logical[:64]);pkg.enc(h,0,64);flash=None
for i in range(struct.unpack_from('<H',h,8)[0]):
 e=bytearray(logical[64+i*80:144+i*80]);pkg.enc(e,0,80)
 if struct.unpack_from('<H',e)[0]==0:
  at,n=struct.unpack_from('<II',e,8);flash=logical[at:at+n]
assert flash is not None
nor=bytearray(b'\xff'*0x100000);nor[:len(flash)]=flash
for at,data in sectors.items():nor[at:at+len(data)]=data
for base,name in [(0xA0000,'amen-160'),(0xB4000,'think-160')]:
 assert nor[base:base+0x14000]==(DIST/'downloads'/(name+'.slot')).read_bytes(),(hex(base),'seed bank differs from packaged audio')
upstream=json.loads((ROOT/'site/vendor/fm1-emu/UPSTREAM.json').read_text())
for name,meta in upstream['files'].items():assert hashlib.sha256((ROOT/'site/vendor/fm1-emu'/name).read_bytes()).hexdigest()==meta['sha256']
assert (DIST/'emulator/fm1_web.wasm').read_bytes()[:8]==b'\0asm\x01\0\0\0'
assert json.loads((DIST/'emulator/firmwares.json').read_text())['default']=='rhythmer'
print('PASS: runtime syntax, FM-1 27-key/8-knob geometry, pinned runtime hashes, both sample banks and exact seed firmware')
