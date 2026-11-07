"""Make a clean browser starting save without changing firmware/test fixtures."""
import base64, hashlib, json, os, struct, subprocess, zlib

def build_web_start(root, build, downloads, firmware_hash):
    tool=build/'web_start_project'
    sources=[root/'tools/six/web_start_project.c',*[root/('rhythmer/firmware/six/'+n+'.c') for n in ('jungle','tb3po','acid_generate')]]
    subprocess.run(['clang','-w','-O1','-fwrapv','-DJUNGLE_AUDIO_CODE=', '-I'+str(build/'gen'),'-I'+str(root/'rhythmer/firmware/src'),*map(str,sources),'-lm','-o',str(tool)],check=True)
    project=build/'web-start.project';subprocess.run([str(tool),str(project)],check=True)
    saved=json.loads((build/'Rhythmer-six-track-test.fm1save').read_text())
    assert saved['hash']==firmware_hash==hashlib.sha256(base64.b64decode(saved['package'])).hexdigest()
    data=project.read_bytes();assert len(data)<=3840
    # Keep settings and both sample banks. Replace every project/section sector,
    # so another section cannot revive the old demonstration song.
    projects=0
    for sector in saved['sectors']:
        old=base64.b64decode(sector['data'])
        if struct.unpack_from('<I',old)[0]!=0x33584953:continue
        kind=struct.unpack_from('<H',old,4)[0]
        if kind not in (1,2,3,4,9,*range(11,23)):continue
        header=struct.pack('<IHHIIIII',0x33584953,kind,0,1,len(data),zlib.crc32(data),0xffffffff,0xffffffff)
        header+=struct.pack('<I',zlib.crc32(header))
        raw=bytearray(b'\xff'*4096);raw[:32]=header;raw[256:256+len(data)]=data
        sector['data']=base64.b64encode(raw).decode();projects+=1
    assert projects>=2
    saved['name']='Rhythmer — Empty start'
    saved['startingProject']={'revision':3,'tempo':160,'tracksWithNotes':[],'unassignedTracks':[5,6]}
    (downloads/'Rhythmer-start.fm1save').write_text(json.dumps(saved)+'\n')
