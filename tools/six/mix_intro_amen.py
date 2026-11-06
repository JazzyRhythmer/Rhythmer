#!/usr/bin/env python3
"""Add a tempo-matched Amen section without re-encoding the opening picture."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess
import numpy as np
import imageio_ffmpeg

ap=argparse.ArgumentParser()
ap.add_argument('--source',type=Path,required=True)
ap.add_argument('--amen',type=Path,required=True)
ap.add_argument('--output',type=Path,required=True)
ap.add_argument('--length',type=int,default=18)
ap.add_argument('--scene-start',type=float,default=6.65)
ap.add_argument('--bars',type=int,default=2)
a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
ff=imageio_ffmpeg.get_ffmpeg_exe();sr=48000
def decode(path,channels):
    raw=subprocess.run([ff,'-v','error','-i',str(path),'-ar',str(sr),'-ac',str(channels),'-f','f32le','-'],capture_output=True,check=True).stdout
    return np.frombuffer(raw,dtype='<f4').reshape(-1,channels).copy()
base=decode(a.source/'Rhythmer-OS-Boot-Sound.wav',2)
amen=decode(a.amen,1)[:,0]
assert len(base)==a.length*sr and len(amen)==3*sr and 1<=a.bars<=8
start=round(a.scene_start*sr);end=start+round((a.bars*1.5+.75)*sr);restore=end+round(.05*sr)
assert restore<len(base)
# Complete bars at 160 BPM; a two-beat sliced fill leads into the logo.
beat=round(.375*sr);eighth=beat//2
fill=np.concatenate([amen[6*beat+i*eighth:6*beat+(i+1)*eighth] for i in [0,0,2,3]])
for at in range(0,len(fill),eighth):
    n=round(.0015*sr);fill[at:at+n]*=np.linspace(0,1,n);fill[at+eighth-n:at+eighth]*=np.linspace(1,0,n)
straight=np.tile(amen,(a.bars+1)//2)[:round(a.bars*1.5*sr)]
break_audio=np.concatenate([straight,fill]);assert len(break_audio)==end-start
break_audio*=.67/max(float(np.max(np.abs(break_audio))),1e-9)
break_audio[:240]*=np.linspace(0,1,240);break_audio[-1920:]*=np.linspace(1,0,1920)
duck=np.ones(len(base),dtype=np.float32)
duck[start:start+960]=np.linspace(1,.22,960)
duck[start+960:end]=.22;duck[end:restore]=np.linspace(.22,1,restore-end)
mix=base*duck[:,None];mix[start:end]+=break_audio[:,None]/np.sqrt(2)
assert np.array_equal(mix[:start],base[:start]) and np.array_equal(mix[restore:],base[restore:])
assert float(np.max(np.abs(mix)))<.99
sound=a.output/'Rhythmer-OS-Boot-Sound-Amen.wav'
subprocess.run([ff,'-v','error','-y','-f','f32le','-ar',str(sr),'-ac','2','-i','-','-c:a','pcm_s24le',str(sound)],input=mix.astype('<f4').tobytes(),check=True)
silent=a.output/f'Rhythmer-OS-Opening-{a.length}s-1080p-Silent.mp4'
if (a.source/silent.name).resolve()!=silent.resolve():shutil.copy2(a.source/silent.name,silent)
video=a.output/f'Rhythmer-OS-Opening-{a.length}s-1080p-Amen.mp4'
subprocess.run([ff,'-v','error','-y','-i',str(silent),'-i',str(sound),'-map','0:v:0','-map','1:a:0','-c:v','copy','-c:a','aac','-b:a','256k','-ar',str(sr),'-shortest','-movflags','+faststart','-metadata','title=Rhythmer OS Opening - Amen Mix','-metadata','artist=Jazzy Rhythmer',str(video)],check=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
(a.output/'audio-edit.json').write_text(json.dumps({'durationSeconds':a.length,'amenStart':a.scene_start,'amenEnd':end/sr,'originalSoundFullyRestored':restore/sr,'tempo':160,'structure':f'{a.bars} full bars, then a two-beat sliced fill with a short fade into the logo','picture':'Input picture stream copied without re-encoding','originalAudioUnchangedOutsideEdit':True,'amenSource':str(a.amen),'amenSHA256':sha(a.amen),'sampleCredit':'Amen, Brother - The Winstons (1969). Drums: Gregory C. Coleman. Prepared Amenity-source firmware loop.','videoSHA256':sha(video)},indent=2)+'\n')
print('Amen opening ready:',video,flush=True)
