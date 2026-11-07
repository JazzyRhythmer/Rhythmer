#!/usr/bin/env python3
"""Progress semantics independent of real MIDI or wall-clock estimates."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[2]
g={'__file__':str(ROOT/'site/tests/test_site.py')};exec((ROOT/'site/tests/test_site.py').read_text().split('model=(SRC/')[0],g);run=g['run']
code=(ROOT/'site/installer/progress.mjs').read_text().replace('export ','')
result=run(code+'''
let clock=0;const p=new InstallProgress(()=>clock),checks=[];function check(v,m){if(!v)throw Error(m);checks.push(m);}
p.begin();check(p.snapshot().percent===0&&p.snapshot().active==='prepare','begin at 0 with no fake time-based advancement');
p.enter('device');p.enter('backup');p.bytes('backup',500,1000);check(p.snapshot().bytes===500,'actual backup byte progress');p.enter('review');const waiting=p.snapshot().percent;
clock+=60000;check(p.snapshot().percent===waiting&&p.snapshot().elapsed===60000,'waiting increases elapsed time, never percentage');
p.enter('stage');p.packet('stage',{address:0,bytes:64,imageBytes:1000},1);check(p.steps.find(s=>s.id==='stage').fraction===0,'sparse loader staging stays indeterminate');
p.enter('loader');p.enter('write');p.packet('write',{address:0,bytes:512,imageBytes:1000},1);p.packet('write',{address:0,bytes:512,imageBytes:1000},2);
check(p.snapshot().bytes===512,'repeated request does not double-count package bytes');p.packet('write',{address:256,bytes:512,imageBytes:1000},3);check(p.snapshot().bytes===768,'overlapping request ranges merged accurately');
p.packet('write',{address:768,bytes:232,imageBytes:1000},4);check(p.steps.find(s=>s.id==='write').fraction<1,'all image bytes served still waits for device finish');
p.enter('reboot');p.enter('identity');check(p.steps.every(s=>s.id!=='A'&&s.id!=='B'),'no separate Break addition stages');p.enter('final');check(p.snapshot().percent<100,'100 percent withheld before integrated package verification');p.finish('Verified');check(p.snapshot().percent===100&&p.snapshot().status==='complete','100 percent only after explicit verified completion');
clock+=5000;const elapsed=p.snapshot().elapsed;clock+=5000;check(p.snapshot().elapsed===elapsed,'completed elapsed time freezes');
p.begin();p.enter('write');p.packet('write',{address:0,bytes:10,imageBytes:1000},1);p.fail('USB lost');const stopped=p.snapshot();clock+=1000;p.packet('write',{address:10,bytes:200,imageBytes:1000},2);check(p.snapshot().percent===stopped.percent&&p.snapshot().status==='failed','late callback cannot advance failed operation');check(stopped.active==='write'&&stopped.detail==='USB lost','failure keeps failed stage and reason');
p.begin();p.enter('review');p.cancel();check(p.snapshot().status==='cancelled'&&p.snapshot().percent<100,'cancel never looks like completion');
p.begin({mode:'stock'});check(!p.steps.some(s=>s.id==='A'||s.id==='B'),'stock recovery omits sample stages');
p.begin();check(p.steps.length===10&&p.steps.some(s=>s.id==='write'),'one integrated firmware process has ten steps');
p.begin({resume:true,loops:false});check(!p.steps.some(s=>s.id==='stage'||s.id==='A'||s.id==='B'),'resume excludes already-staged loader and preserved audio');
p.enter('backup');p.skip('backup');p.enter('review');check(p.steps.find(s=>s.id==='backup').status==='skipped','unsupported backup visibly marked not available');
check(duration(65000)==='01:05','elapsed display format');JSON.stringify({passed:checks.length,checks,physicalHardwareTested:false});
''')
folder=Path('/Volumes/SSD/Development/TestResults/rhythmer/installer');folder.mkdir(parents=True,exist_ok=True);(folder/'progress-model-tests.json').write_text(json.dumps(json.loads(result),indent=2)+'\n');print('PASS:',json.loads(result)['passed'],'progress model checks')
