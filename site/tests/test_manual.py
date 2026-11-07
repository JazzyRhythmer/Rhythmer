#!/usr/bin/env python3
"""Exercise manual navigation/search and reject stale build-note claims."""
from pathlib import Path
import json, re
R=Path(__file__).resolve().parents[2]
g={'__file__':str(R/'site/tests/test_site.py')}
exec((R/'site/tests/test_site.py').read_text().split('model=(SRC/')[0],g)
run=g['run']
src=(R/'site/src/manual.mjs').read_text().replace('export ','')
landing=(R/'site/src/landing.mjs').read_text().replace('export ','')
result=run(src+'\n'+landing+'''
const checks=[];const check=(ok,label)=>{if(!ok)throw Error(label);checks.push(label);};
const rows=manualSections();check(rows.length===16,'16 complete manual sections');
const trackHelp=rows.find(r=>r[0]==='tracks')[2];check(['six-track-warning.png','knob 1 / TRACKS','knob 2 / TRACKS','OCT+','OCT−','noise or glitches'].every(t=>trackHelp.includes(t)),'track-mode guide includes actual notice, both paths, confirmation and remedies');
const ids=rows.map(r=>r[0]);const links=renderLanding({config:{creator:'Jazzy Rhythmer',links:{},localOnly:false},release:{version:'0.1.1-beta3',sha256:'current',buildRevision:'20261107.2'},symbol:()=>'',makerNote:()=>'',ext:()=>''});
const targets=[...links.matchAll(/href="#guide\/([^"#]+)"/g)].map(m=>m[1]);
check(targets.length>=6&&targets.every(t=>ids.includes(t)),'every homepage chapter link resolves');
const markup=renderManual({release:{version:'0.1.1-beta3',product:'FM-1_748005',buildRevision:'20261107.2'}});
check(markup.includes('Build 20261107.2'),'manual identifies the maintenance build');
let scrolled='',focused='',remembered='',printed=0;
const input={value:'',addEventListener:(e,fn)=>{input[e]=fn;},focus:()=>focused='search'};
const empty={},count={};const sections=rows.map(([id,title,html])=>({dataset:{manualSection:id},textContent:title+' '+html,hidden:false,scrollIntoView:()=>scrolled=id,setAttribute:()=>{},focus:()=>focused=id}));
const nav=ids.map(id=>({dataset:{manualGo:id},addEventListener:(e,fn)=>{nav.find(n=>n.dataset.manualGo===id)[e]=fn;}}));
const top={addEventListener:(e,fn)=>{top[e]=fn;}},print={addEventListener:(e,fn)=>{print[e]=fn;}};
const document={getElementById:id=>({'manual-search':input,'manual-results':count,'manual-empty':empty,'manual-print':print}[id]),querySelectorAll:s=>s==='[data-manual-section]'?sections:s==='[data-manual-go]'?nav:[top],querySelector:()=>({scrollIntoView:()=>scrolled='contents'})};
const history={replaceState:(a,b,url)=>remembered=url};const window={print:()=>printed++};const matchMedia=()=>({matches:true});const requestAnimationFrame=fn=>fn();
wireManual('acid');check(scrolled==='acid'&&focused==='acid','direct Acid link scrolls and focuses its chapter');
input.value='zz-no-such-topic';input.input();check(sections.every(s=>s.hidden)&&empty.hidden===false,'unmatched search shows empty state');
nav.find(n=>n.dataset.manualGo==='uploads').click({preventDefault(){}});check(scrolled==='uploads'&&remembered==='#guide/uploads'&&sections.every(s=>!s.hidden),'chapter link clears search and creates shareable URL');
top.click({preventDefault(){}});check(remembered==='#guide'&&focused==='search','contents resets the chapter URL and focuses search');
input.value='zz-no-such-topic';input.input();print.click();check(printed===1&&sections.every(s=>!s.hidden),'printing includes every chapter after search');
JSON.stringify({passed:checks.length,checks});
''')
data=json.loads(result)
app=(R/'site/src/app.js').read_text()
assert "candidate.firmwareSha256===release.sha256" in app
assert 'eleven actual-firmware emulator scenarios' not in app
assert 'seven viewport host checks' not in app
print('PASS:',data['passed'],'manual interactions; current hash-bound validation; stale claims removed')
g['jsc'].JSGlobalContextRelease(g['ctx'])
