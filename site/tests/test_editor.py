#!/usr/bin/env python3
"""Execute the actual browser WASM codec and pinned JIANT protocol in JavaScriptCore."""
from pathlib import Path
import json,base64,re,os,time
R=Path(__file__).resolve().parents[2];DIST=Path(os.environ.get('RHYTHMER_SITE_DIST',R/'site/dist'));D=DIST/'editor'
# Reuse the dependency-free JSC evaluation helper, without executing the old draft tests.
# exec globals must be one namespace because run() resolves ctx/jsc there.
g={'__file__':str(R/'site/tests/test_site.py')};exec((R/'site/tests/test_site.py').read_text().split('model=(SRC/')[0],g)
run=g['run'];print('WASM:',run('typeof WebAssembly'),flush=True)
html=(D/'index.html').read_text();proto=html.split('/*PROTO-BEGIN*/')[1].split('/*PROTO-END*/')[0]
run(proto+'\n"protocol loaded"')
run('async function editorUISyntax(){'+html.split('<script>')[-1].split('</script>')[0]+'\n};\"editor UI syntax OK\"')
transport=re.sub(r'^import .*?;\n','',(R/'site/editor/workspace.mjs').read_text(),flags=re.M).replace('export ','')
run('async function transportSyntax(){'+transport+'\n};\"workspace transport syntax OK\"')
wasm=list((D/'workspace.wasm').read_bytes())
seed=json.loads((DIST/'downloads/Rhythmer-start.fm1save').read_text())
sectors=[{'at':s['at'],'data':list(base64.b64decode(s['data']))} for s in seed['sectors']]
run('''var wa, imports={wasi_snapshot_preview1:{}};var wm=new WebAssembly.Module(new Uint8Array('''+json.dumps(wasm)+'''));for(const im of WebAssembly.Module.imports(wm))imports[im.module][im.name]=()=>0;wa=new WebAssembly.Instance(wm,imports).exports;wa._initialize();wa.ws_init();var fl=new Uint8Array(wa.memory.buffer,wa.ws_flash(),1048576);for(const s of '''+json.dumps(sectors)+''')fl.set(s.data,s.at);wa.ws_boot();
var checks=[];function check(ok,m){if(!ok)throw Error(m);checks.push(m);}
function ask(r){const f=frame(r[0],r[1]);new Uint8Array(wa.memory.buffer,wa.ws_input(),f.length).set(f);let n=wa.ws_request(f.length);if(!n)throw Error('No C reply '+r[0]);const d=new Uint8Array(wa.memory.buffer,wa.ws_output(),n).slice();let p=0;while(p<d.length){let e=d.indexOf(247,p);const f=unframe(d.slice(p,e+1));if(f?.cmd===r[0])return f.a;p=e+1;}throw Error('Missing reply');}
var info=parse[CMD.INFO](ask(req.info()));check(info.version==='Rhythmer dev'&&info.nengines===21&&info.ntrk===6,'exact Rhythmer INFO');check(info.pcount===127&&info.pe0===119&&info.gcount===71,'actual current parameter layout');
var drumNames=parse[CMD.NAMES](ask(req.names(10))).names;check(drumNames.length===19&&['MD METAL','MD WIRE','MD TRX','MD EFM','MD PERC','MD STUDIO','MD SPACE','MD DIRT','MD LAB 1','MD LAB 2'].every(n=>drumNames.includes(n)),'all ten MD kits available through real firmware editor metadata');
var acidNames=parse[CMD.NAMES](ask(req.names(16))).names;check(acidNames.length===5&&['X0X ACID','HT SQUELCH','HT DEEPBASS','HT SCREAMER','HT SQUAREAC'].every(n=>acidNames.includes(n)),'both Acid engines and all four Hortator presets available');
for(const ei of [0,17])check(parse[CMD.NAMES](ask(req.names(ei))).names.every(n=>!n.includes('ACID')),'ordinary synth presets have unambiguous names');
ask(req.track(0));var labPreset=parse[CMD.PRESET](ask(req.preset(10,17)));check(labPreset.preset===17,'experimental MD kit loads through actual editor command');
var bankBytes=wa.ws_archive(),bankView=new DataView(wa.memory.buffer,wa.ws_archive_ptr(),bankBytes),bankPos=4,hasExtendedKit=false;
for(let i=0;i<bankView.getUint32(0,true);i++){let id=bankView.getUint32(bankPos,true),len=bankView.getUint32(bankPos+4,true);bankPos+=12;if(len===3840&&bankView.getUint8(bankPos+65))hasExtendedKit=true;bankPos+=len;}
check(hasExtendedKit,'editor archive retains extended MD factory sound selection bits');
ask(req.track(5));var st=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(st.n===0,'seed Break page is empty in real SEQ '+JSON.stringify(st));
var patch={...st,n:2,notes:[17,33,0,0],time:0,vel:111};ask(req.stepSet(0,patch));st=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(st.n===2&&st.notes[0]===17&&st.notes[1]===33,'Break performance and source B share the real step');
var saved=parse[CMD.PROJECT](ask(req.project(1,2)));check(saved.used,'section C written through actual storage');
ask(req.stepSet(0,{...patch,n:0,time:2}));ask(req.project(0,2));st=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(st.notes[0]===17&&st.notes[1]===33,'section load restores performance pattern');
check(wa.ws_project(1)===0&&wa.ws_project_current()===1,'switch to project 2');ask(req.project(1,0));check(wa.ws_project_mask(1)&1,'project 2 has its own section A');check(wa.ws_project(0)===0&&wa.ws_project_current()===0,'return project 1');ask(req.project(0,2));st=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(st.notes[0]===17,'project 1 preserved across project switch');
ask(req.track(4));check(wa.ws_acid(4,0)===0,'original X0X generator runs in data engine');var acid=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(wa.ws_acid(4,1)===0,'original X0X mutation runs');
check(wa.ws_runtime_flash()===0,'emulator export commits unsaved runtime using real A/B flash storage');
var preserved=new Uint8Array(wa.memory.buffer,wa.ws_flash(),1048576).slice();wa.ws_init();new Uint8Array(wa.memory.buffer,wa.ws_flash(),1048576).set(preserved);wa.ws_boot();ask(req.track(5));var restored=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(restored.notes[0]===17&&restored.notes[1]===33,'sample-inclusive emulator export restores current unsaved pattern');
check(new Uint8Array(wa.memory.buffer,wa.ws_flash()+0xa0000,81920).every((v,i)=>v===preserved[0xa0000+i]),'Amen sample preserved on workspace export');
check(new Uint8Array(wa.memory.buffer,wa.ws_flash()+0xb4000,81920).every((v,i)=>v===preserved[0xb4000+i]),'Think sample preserved on workspace export');
var n=wa.ws_archive();check(n>0,'native packed archive fits device capacity');var ar=new DataView(wa.memory.buffer,wa.ws_archive_ptr(),n);check(ar.getUint32(0,true)===27,'archive contains all four projects (27 objects)');
JSON.stringify({passed:checks.length,checks,info:{version:info.version,pcount:info.pcount,gcount:info.gcount,pe0:info.pe0,ntrk:info.ntrk},archiveBytes:n});
''')
result=json.loads(run('JSON.stringify({passed:checks.length,checks,info:{pcount:info.pcount,gcount:info.gcount,pe0:info.pe0,ntrk:info.ntrk},archiveBytes:n})'))
Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',result['passed'],'actual C/WASM editor and storage checks',flush=True)
# Round-trip the original JIANT backup implementation against actual C BACKUP_PUT.
import struct,zlib
run("ask(req.track(5));ask(req.stepSet(0,{n:2,notes:[29,33,0,0],time:0,flags:0,vel:111,chance:100,ratchet:1}));n=wa.ws_archive();'captured'")
raw=bytes(json.loads(run('JSON.stringify(Array.from(new Uint8Array(wa.memory.buffer,wa.ws_archive_ptr(),n)))')))
count=struct.unpack_from('<I',raw)[0];objects=[];p=4
for k in range(count):
 id,size,crc=struct.unpack_from('<III',raw,p);p+=12;body=raw[p:p+size];p+=size
 assert zlib.crc32(body)==crc
 objects.append({'id':id,'size':size,'crc':crc,'data':base64.b64encode(body).decode()})
backup={'format':'felucca-backup','version':1,'firmware':'Rhythmer dev','created':'2026-11-06T00:00:00Z','objects':objects}
base64js='''var b64chars='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';function atob(s){let n=0,b=0,out='';for(const c of s){if(c==='=')break;const x=b64chars.indexOf(c);if(x<0)throw Error('base64');b=(b<<6)|x;n+=6;if(n>=8){n-=8;out+=String.fromCharCode((b>>>n)&255);}}return out;}'''
run(base64js+(D/'fm1backup.js').read_text().replace('export ','')+'\nvar archiveFile='+json.dumps(backup)+";readBackup(archiveFile);wa.ws_init();wa.ws_boot();var restoreDone=false,restoreError=null;restoreBackup(ask,archiveFile).then(()=>restoreDone=true,e=>restoreError=String(e));'restoring';")
status=json.loads(run("JSON.stringify({done:restoreDone,error:restoreError})"));assert status['done'] and not status['error'],status
run("var releaseArchive={...archiveFile,firmware:'Rhythmer v0.1.1-beta3'};check(readBackup(releaseArchive).objects.length===27,'release-labelled musical archive accepted with exact object layout');for(const legacyName of ['JIANT Rhythmer dev','JIANT v0.1.1-beta3'])check(readBackup({...archiveFile,firmware:legacyName}).objects.length===27,'legacy Rhythmer archive remains compatible '+legacyName);var foreignArchive={...archiveFile,firmware:'JIANT v9.9'};var foreignRejected=false;try{readBackup(foreignArchive);}catch{foreignRejected=true;}check(foreignRejected,'unreviewed firmware archive still rejected');")
run("ask(req.track(5));var back=parse[CMD.STEP_GET](ask(req.stepGet(0)));check(back.notes[0]===29&&back.notes[1]===33,'JIANT musical archive restores unsaved Break performance using real C');check(wa.ws_project_mask(1)&1,'archive preserves second project bank');'restored';")
result=json.loads(run('JSON.stringify({passed:checks.length,checks,info:{pcount:info.pcount,gcount:info.gcount,pe0:info.pe0,ntrk:info.ntrk},archiveBytes:n})'))
Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',result['passed'],'actual C/WASM and JIANT archive restore checks',flush=True)

frameHelper=(R/'site/editor/workspace.mjs').read_text().split("const KEY=")[0]
frameHelper=re.sub(r'^import .*?;\n','',frameHelper,flags=re.M).replace('export ','')
run(frameHelper+"var two=splitEditorFrames(new Uint8Array([240,125,70,76,24,247,240,125,70,76,67,2,0,0,247]));check(two.length===2&&two[0][4]===24&&two[1][4]===67,'queued RELOAD push and archive reply are separate MIDI messages');check(splitEditorFrames(new Uint8Array([0,240,125,70])).length===0,'partial MIDI frames are not dispatched');")
result=json.loads(run('JSON.stringify({passed:checks.length,checks,info:{pcount:info.pcount,gcount:info.gcount,pe0:info.pe0,ntrk:info.ntrk},archiveBytes:n})'))
Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',result['passed'],'C/WASM, archive restore and MIDI transport checks',flush=True)

run("check(replyMatches(80,[],[83,73,88,51,0,3]),'Break upload query matches SIX3 protocol');check(!replyMatches(80,[1,0],[2,0,0,3]),'stale upload operation acknowledgement rejected');check(!replyMatches(80,[2,1],[2,0,0,3]),'wrong audio slot acknowledgement rejected');'OK'")
result=json.loads(run('JSON.stringify({passed:checks.length,checks})'));Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n');print('PASS:',result['passed'],'C/WASM, archives, MIDI and Break-upload reply checks')

# Explicit section saves and all four projects use the real storage codec.
run("for(let k=0;k<4;k++){check(wa.ws_project(k)===0,'open project '+(k+1));check(wa.ws_section_save(k)===0,'save section '+String.fromCharCode(65+k)+' in project '+(k+1));check(wa.ws_section_current()===k,'current section follows explicit save');check(wa.ws_project_mask(k)&(1<<k),'project section mask follows actual storage');}check(wa.ws_section_save(4)!==0,'invalid section rejected');check(wa.ws_project(4)!==0,'invalid fifth project rejected');'OK'")
# Test the shipped JS autosave transport with the actual C WASM module.
# Browser APIs below are in-memory fixtures: no browser data or hardware I/O.
run("var mockStore=new Map(),quotaFail=false,saveMessages=[],eventHandlers={};function btoa(str){let out='';for(let i=0;i<str.length;i+=3){let a=str.charCodeAt(i),b=str.charCodeAt(i+1),c=str.charCodeAt(i+2),x=(a<<16)|((b||0)<<8)|(c||0);out+=b64chars[x>>>18]+b64chars[(x>>>12)&63]+(i+1<str.length?b64chars[(x>>>6)&63]:'=')+(i+2<str.length?b64chars[x&63]:'=');}return out;}var localStorage={getItem:k=>mockStore.get(k)||null,setItem:(k,v)=>{if(quotaFail)throw Error('Storage quota test');mockStore.set(k,v);}},window={addEventListener:(k,f)=>eventHandlers[k]=f,removeEventListener:k=>delete eventHandlers[k]},navigator={locks:{request:async(n,o,cb)=>cb({name:n})}},performance={now:()=>10000},crypto={getRandomValues:a=>a.fill(31)},setTimeout=()=>1,clearTimeout=()=>{};WebAssembly.compileStreaming=async()=>wm;WebAssembly.instantiate=async(m,i)=>new WebAssembly.Instance(m,i);var wsSeed="+json.dumps({k:v for k,v in seed.items() if k!='package'})+";var fetch=async url=>({json:async()=>String(url).includes('release.json')?{sha256:wsSeed.hash}:wsSeed});var workspaceTestError=null,workspaceTestDone=false;async function testTransport(){"+transport+"\nvar w=await createWorkspace(m=>saveMessages.push(m));check(w.current()===0,'transport boots a real four-project workspace');w.generate(4,false);check(w.status().dirty,'unsaved edit is reported');check(w.flush()&&!w.status().dirty&&mockStore.has('rhythmer-workspace-v2'),'explicit flush persists current edits before leaving');w.saveSection(0);await w.project(1,0);check(w.current()===1&&(w.mask(0)&1),'save and switch keeps outgoing project A');w.generate(4,true);quotaFail=true;var stopped=false;try{await w.project(2,0);}catch{stopped=true;}check(stopped&&w.current()===1&&w.status().dirty&&!!w.status().saveError,'quota failure keeps current project and reports unsaved edits');quotaFail=false;check(w.flush()&&!w.status().dirty,'saving recovers after quota becomes available');w.close();workspaceTestDone=true;}testTransport().catch(e=>workspaceTestError=String(e));'running'")
for attempt in range(250):
 state=json.loads(run('JSON.stringify({done:workspaceTestDone,error:workspaceTestError})'))
 if state['done'] or state['error']:break
 time.sleep(.02)
assert state['done'] and not state['error'],state
result=json.loads(run('JSON.stringify({passed:checks.length,checks})'))
Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',result['passed'],'actual four-project C storage, archive restore and autosave failure/recovery checks')

# Both sources and every slot survive the actual editor's section-save path.
run("wa.ws_init();wa.ws_boot();for(let k=0;k<6;k++){ask(req.track(k));for(const [id,v] of [[9,20+k],[10,1],[11,k],[12,5+k],[115,80+k],[116,3],[117,30+k],[118,10+k]])ask(req.set(0,id,v));for(let j=0;j<4;j++){ask(req.set(0,49+3*j,j%2?15:1));ask(req.set(0,50+3*j,1));ask(req.set(0,51+3*j,12+j));}}check(parse[CMD.PROJECT](ask(req.project(1,0))).used,'six-track dual-LFO section saved through editor');for(let k=0;k<6;k++){ask(req.track(k));ask(req.set(0,9,0));ask(req.set(0,115,0));}ask(req.project(0,0));for(let k=0;k<6;k++){ask(req.track(k));const p=parse[CMD.DUMP](ask(req.dump()),info).p;check(p[9]===20+k&&p[115]===80+k&&p[116]===3&&p[117]===30+k&&p[118]===10+k&&[0,1,2,3].every(j=>p[49+3*j]===(j%2?15:1)&&p[51+3*j]===12+j),'independent LFO 1/2 and all four sources restored on T'+(k+1));}'OK'")
assert 'TWO LFOS / PER TRACK' in html and '["LFO 1 DEST", 0,' in html
result=json.loads(run('JSON.stringify({passed:checks.length,checks})'))
Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',result['passed'],'including six-track dual-LFO editor save/restore checks')

run("function enumShown(d) {"+html.split("function enumShown(d) {",1)[1].split("\n}",1)[0]+"\n}; true;")
run("check(info.lfo2Dest,'matching firmware advertises atomic LFO2 DEST');wa.ws_init();wa.ws_boot();for(let k=0;k<6;k++){ask(req.track(k));for(let j=0;j<4;j++)check(parse[CMD.GET](ask(req.get(2,j))).value===0,'default LFO2 DEST is zero T'+(k+1)+' / '+j);for(const [id,v] of [[0,-30+k],[1,24+k],[2,-12+k],[3,90+k]])check(parse[CMD.SET](ask(req.set(2,id,v))).value===v,'atomic direct depth set T'+(k+1)+' / '+id);const p=parse[CMD.DUMP](ask(req.dump()),info).p;check([0,1,2,3].every(j=>p[49+3*j]===0),'direct depths preserve all MOD slots T'+(k+1));}check(parse[CMD.PROJECT](ask(req.project(1,0))).used,'six-track LFO2 DEST section saved');for(let k=0;k<6;k++){ask(req.track(k));ask(req.set(2,0,0));}ask(req.project(0,0));for(let k=0;k<6;k++){ask(req.track(k));const want=[-30+k,24+k,-12+k,90+k];check(want.every((v,j)=>parse[CMD.GET](ask(req.get(2,j))).value===v),'all four direct depths restored T'+(k+1));}var d2Source=parse[CMD.DESC](ask(req.desc(0,49)));check(enumShown(d2Source).slice(0,4).map(n=>d2Source.names[n]).join(',')==='OFF,LFO 1,LFO 2,ENV','browser SRC order keeps adjacent LFOs with stable wire IDs');'OK'")
assert '["LFO 2 DEST", 2, [0,1,2,3]]' in html
result=json.loads(run('JSON.stringify({passed:checks.length,checks})'))
Path(os.environ.get('RHYTHMER_EDITOR_RESULTS','/Volumes/SSD/Development/TestResults/rhythmer/installer/editor-results.json')).write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',result['passed'],'including atomic dedicated LFO2 DEST, six-track save/restore and SRC ordering')
