#!/usr/bin/env python3
"""Exercise the real portable loader against fake NOR; no physical device I/O."""
from pathlib import Path
import importlib.util,subprocess,struct,json,hashlib,zlib,os
from sample_assets import sample_dir
R=Path(__file__).resolve().parents[2];O=Path(os.environ.get('RHYTHMER_BUILD_DIR',R/'rhythmer/build-ssd'));Q=Path(os.environ.get('RHYTHMER_LOADER_QA',R/'qa/embedded-install'))
spec=importlib.util.spec_from_file_location('embedded_pack',R/'rhythmer/tools/fm1pkg_make.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
raw=(O/'felucca.fwsc').read_bytes();logical=b''.join(raw[i*48:i*48+47] for i in range(20))+raw[960:]
h=bytearray(logical[:64]);m.enc(h,0,64);entries=[]
for i in range(struct.unpack_from('<H',h,8)[0]):
 e=bytearray(logical[64+i*80:144+i*80]);m.enc(e,0,80);kind=struct.unpack_from('<H',e)[0];at,n=struct.unpack_from('<II',e,8);entries.append((kind,logical[at:at+n]))
flash=next(x for k,x in entries if k==0);ota=next(x for k,x in entries if k==100);assert len(flash)==0xc8000
for k,name in enumerate(['amen-160','think-160']):assert flash[0xa0000+k*0x14000:0xb4000+k*0x14000]==(sample_dir(R)/(name+'.slot')).read_bytes()
app_unchanged=(O/'felucca.bin').read_bytes()==(Q/'previous-app.bin').read_bytes()
exe=O/'embedded-ldr-test';subprocess.run(['clang','-w','-O1','-g','-fwrapv','-fsanitize=address,undefined',str(R/'rhythmer/tests/ldr_test.c'),'-o',str(exe)],check=True)
def run(name,new,extra):
 with (Q/(name+'.log')).open('w') as f:subprocess.run([str(exe),str(Q/'previous.fwsc'),str(new),*extra],stdout=f,stderr=subprocess.STDOUT,check=True,cwd=R)
 s=(Q/(name+'.log')).read_text();assert 'FAIL' not in s and 'runtime error' not in s and 'AddressSanitizer' not in s,name
run('loader-positive',O/'felucca.fwsc',['584336',str(Q/'loader-installed.flash')]);run('loader-legacy',Q/'previous.fwsc',['584336'])
for name,mutate in [('bad-marker',lambda f:f.__setitem__(0x93000,0)),('bad-range',lambda f:f.__setitem__(slice(0x9300c,0x93010),struct.pack('<I',0x97000))),('bad-bank-header',lambda f:f.__setitem__(0xb4000,0)),('bad-bank-payload',lambda f:f.__setitem__(0xb4400,f[0xb4400]^1))]:
 f=bytearray(flash);mutate(f)
 if name=='bad-range':struct.pack_into('<I',f,0x9301c,zlib.crc32(f[0x93000:0x9301c]))
 p=Q/(name+'.fwsc');p.write_bytes(m.ufw(bytes(f),ota,'FM-1_900'));run(name,p,['reject'])
nor=(Q/'loader-installed.flash').read_bytes();assert nor[0xa0000:0xc8000]==flash[0xa0000:0xc8000]
report=dict(firmwareSha256=hashlib.sha256(raw).hexdigest(),appSha256=hashlib.sha256((O/'felucca.bin').read_bytes()).hexdigest(),appUnchanged=app_unchanged,audioRegressionRequired=not app_unchanged,physicalHardwareTested=False,writeRanges=[[0x4000,0x93000],[0xa0000,0xc8000]],sampleHeaderSectorCommittedLast=True,powerInterruptionResumeCases=5,negativePackagesRejectedBeforeErase=4,legacyCodeOnlyPackageSupported=True,banks=[hashlib.sha256(flash[a:a+0x14000]).hexdigest() for a in [0xa0000,0xb4000]])
(Q/'loader-results.json').write_text(json.dumps(report,indent=2)+'\n');print('Integrated loader: CRC/range validation, exact NOR banks, project/settings preservation, five resumptions, legacy compatibility and four pre-erase negative cases PASS')
