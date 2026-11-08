#!/usr/bin/env python3
"""Original, quieter manual-tour score: warm 120 BPM broken-house, no recordings."""
from pathlib import Path
import numpy as np,math,subprocess,json,hashlib
from scipy.signal import butter,sosfilt
from scipy.io.wavfile import write
R=Path(__file__).resolve().parents[2];Q=R/'qa/manual-motion-v4-20261108';O=R/'outputs/Rhythmer-Manual-Motion-v4'
FF='/Volumes/SSD/Development/Toolchains/rhythmer-motion/imageio_ffmpeg/binaries/ffmpeg-macos-aarch64-v7.1'
SR=48000;D=64;BPM=120;beat=.5;rng=np.random.default_rng(8112026)
bus=np.zeros((SR*D,2),np.float64);per=np.zeros_like(bus)
def filt(x,f,kind='lowpass'):return sosfilt(butter(2,f,btype=kind,fs=SR,output='sos'),x,axis=0)
def add(y,t,gain=1,pan=0,dest=None):
 dest=bus if dest is None else dest;i=round(t*SR)
 if i<0:y=y[-i:];i=0
 n=min(len(y),len(dest)-i)
 if n<=0:return
 if y.ndim==1:y=np.column_stack((y*math.sqrt((1-pan)/2),y*math.sqrt((1+pan)/2)))
 dest[i:i+n]+=y[:n]*gain
def env(t,d,a=.003,r=.05):return np.minimum(t/a,1)*np.minimum((d-t)/r,1)
def kick():
 t=np.arange(int(.32*SR))/SR;ph=2*np.pi*(46*t+52*.026*(1-np.exp(-t/.026)));return np.sin(ph)*np.exp(-t*15)*env(t,.32,.001,.05)
def rim():
 t=np.arange(int(.11*SR))/SR;z=(np.sin(2*np.pi*1740*t)+.52*np.sin(2*np.pi*2310*t))*.3+filt(rng.normal(size=len(t)),[800,6200],'bandpass')*.12;return z*np.exp(-t*57)*env(t,.11,.0008,.02)
def hat():
 t=np.arange(int(.07*SR))/SR;return filt(rng.normal(size=len(t)),[4200,10500],'bandpass')*np.exp(-t*90)*env(t,.07,.001,.02)*.18
K=kick();S=rim();H=hat()
for bar in range(32):
 at=bar*2;dens=0 if bar<2 else .65 if bar<4 or 8<=bar<12 or bar>=30 else 1
 if not dens:continue
 for s in ([0,8] if bar<4 or bar>=30 else [0,7,10] if bar%4<2 else [0,6,10,15]):add(K,at+s*.125,.36*dens,dest=per)
 for s in [4,12]:add(S,at+s*.125+.008,.42*dens,.06 if s==4 else -.08,dest=per)
 for s in [2,4,6,8,10,12,14]:add(H,at+s*.125+(s%4==2)*.009,.33*dens,(-.3 if s%4 else .3),per)
 if bar>=12 and bar%4==3:
  for s in [13,15]:add(S,at+s*.125+.014,.085*dens,-.25,per)
write(O/'Quiet-Circuit-Drum-Loop-120BPM-2bars.wav',SR,(per[8*SR:12*SR]*32767).astype(np.int16))
# Soft electric-key chords, original E-major progression and inversions.
chords=[[52,59,63,66,71],[49,56,59,63,68],[45,52,56,59,64],[47,54,57,63,68]]
for bar in range(0,32,2):
 notes=chords[(bar//2)%4];at=bar*2;d=4.25;t=np.arange(round(d*SR))/SR;st=[]
 for side in range(2):
  y=np.zeros(len(t))
  for j,n in enumerate(notes):
   f=440*2**((n-69)/12)*(1+(-1 if side else 1)*.0007)
   ph=2*np.pi*f*t+.28*np.sin(2*np.pi*f*2*t)*np.exp(-t*2.2)
   y+=(np.sin(ph)+.16*np.sin(2*ph))*np.exp(-t*(.8+j*.07))
  st.append(filt(y,2300)*env(t,d,.025,.5)*.048)
 add(np.column_stack(st),at)
 # Two subdued offbeat answers rather than repeated block chords.
 if 8<=bar<30:
  add(np.column_stack(st)[:int(1.2*SR)],at+2.75,.24)
for bar in range(4,31):
 root=[40,37,33,35][(bar//2)%4]
 for pos,off,d,gain in [(0,0,.64,.27),(1.5,0,.3,.19),(3.25,7,.24,.13)]:
  t=np.arange(round(d*SR))/SR;f=440*2**((root+off-69)/12);y=(np.sin(2*np.pi*f*t)+.08*np.sin(2*np.pi*f*2*t))*env(t,d,.012,.07)*np.exp(-t*.8)
  add(y,bar*2+pos*.5,gain)
# Sparse bell/pluck motif, absent during the CPU text to keep attention on the guide.
for bar in range(12,30):
 for j,pos in enumerate([.75,2.5] if bar%2==0 else [1.75]):
  note=chords[(bar//2)%4][3+j%2]+12;d=1.4;t=np.arange(round(d*SR))/SR;f=440*2**((note-69)/12)
  y=(np.sin(2*np.pi*f*t)+.12*np.sin(2*np.pi*f*3*t))*np.exp(-t*5.5)*env(t,d,.006,.2)
  at=bar*2+pos*.5;pan=(-1 if bar%2 else 1)*.3;add(y,at,.052,pan)
  add(y,at+.375,.018,-pan);add(y,at+.75,.008,pan)
# Gentle space from the music only, not the transient instructions.
for delay,g in [(.21,.13),(.43,.08),(.67,.045)]:
 n=round(delay*SR);bus[n:]+=bus[:-n,::-1]*g
bus+=per
# Short wood-like UI ticks, not pitched notification beeps.
for at in [14.88,22.24,24.96,26.96,29.16,31.68,34.2,36.8,39.44,42.08,48.72,49.44,50.16,51.36,54.0,55.3,56.5,57.7]:
 t=np.arange(int(.025*SR))/SR;z=filt(rng.normal(size=len(t)),[900,4500],'bandpass')*np.exp(-t*180)*env(t,.025,.001,.008);add(z,at,.035)
fade=np.minimum(np.arange(len(bus))/SR/.7,1)*np.minimum((D-np.arange(len(bus))/SR)/1.3,1);bus*=fade[:,None]
peak=np.max(np.abs(bus));bus*=.75/max(peak,.75);write(Q/'manual-score-mix.wav',SR,bus.astype(np.float32))
cmd=[FF,'-hide_banner','-i',str(Q/'manual-score-mix.wav'),'-af','loudnorm=I=-17:TP=-1.5:LRA=8:print_format=json','-f','null','-']
p=subprocess.run(cmd,stderr=subprocess.PIPE,stdout=subprocess.DEVNULL,text=True,check=True);a=p.stderr[p.stderr.rfind('{'):];stats=json.JSONDecoder().raw_decode(a)[0]
f='loudnorm=I=-17:TP=-1.5:LRA=8:measured_I={input_i}:measured_TP={input_tp}:measured_LRA={input_lra}:measured_thresh={input_thresh}:offset={target_offset}:linear=true'.format(**stats)
score=O/'Rhythmer-Manual-Quiet-Circuit-120BPM.wav';subprocess.run([FF,'-v','error','-y','-i',str(Q/'manual-score-mix.wav'),'-af',f,'-ar',str(SR),'-c:a','pcm_s24le',str(score)],check=True)
(Q/'score-validation.json').write_text(json.dumps({'title':'Quiet Circuit','bpm':BPM,'duration':D,'originalComposition':True,'thirdPartyRecordings':False,'description':'Warm electric keys, restrained broken-house percussion, rounded bass and sparse plucks; gentle action ticks. A new composition, distinct from the update-video score.','sha256':hashlib.sha256(score.read_bytes()).hexdigest(),'normalizationTargetLUFS':-17,'truePeakCeilingDBTP':-1.5,'preNormalization':stats},indent=2)+'\n')
print('New original manual score ready:',score,flush=True)
