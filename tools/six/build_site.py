#!/usr/bin/env python3
"""Build the local-only Rhythmer website. Never registers or deploys anything."""
from pathlib import Path
import os, argparse, hashlib, json, re, shutil
from PIL import Image
from site_emulator import build_emulator
from web_start_seed import build_web_start
from site_editor import build_editor
from site_installer import build_installer
from site_navigation import apply_site_navigation
from release_rights import public_audio_release_blockers
ROOT = Path(__file__).resolve().parents[2]
SITE = ROOT / 'site'

def build():
    if not Path('/Volumes/SSD/Development').is_dir(): raise SystemExit('Configured SSD Development storage is not mounted.')
    config = json.loads(Path(os.environ.get('RHYTHMER_SITE_CONFIG',SITE/'site.config.json')).read_text())
    if config.get('localOnly') is not True:
        blockers=public_audio_release_blockers(ROOT)
        decision=json.loads((SITE/'audio-rights.json').read_text()).get('distributionDecision',{})
        if blockers and not decision.get('ownerDirectedPublicDistribution'):
            raise SystemExit('PUBLIC AUDIO RELEASE BLOCKED: '+'; '.join(blockers))
    dist = Path(os.environ.get('RHYTHMER_SITE_DIST',SITE/'dist'))
    expected=Path(os.environ.get('RHYTHMER_SITE_DIST','/Volumes/SSD/Development/BuildCache/rhythmer/site/dist')).resolve()
    if not expected.is_relative_to(Path('/Volumes/SSD/Development')):
        raise SystemExit('Site build output must stay on the configured SSD Development store.')
    expected.mkdir(parents=True,exist_ok=True)
    if not dist.exists():dist.symlink_to(expected,target_is_directory=True)
    if dist.resolve()!=expected:raise SystemExit('Refusing to generate outside configured SSD build storage; inspect the existing site/dist first.')

    for f in (SITE/'src').iterdir():
        if f.is_file(): shutil.copy2(f, dist/f.name)
    app=dist/'app.js'
    landing_version=hashlib.sha256((dist/'landing.mjs').read_bytes()).hexdigest()[:12]
    newsletter_version=hashlib.sha256((dist/'newsletter.mjs').read_bytes()).hexdigest()[:12]
    manual_version=hashlib.sha256((dist/'manual.mjs').read_bytes()).hexdigest()[:12]
    app.write_text(app.read_text().replace("'./landing.mjs'","'./landing.mjs?v="+landing_version+"'").replace("'./newsletter.mjs'","'./newsletter.mjs?v="+newsletter_version+"'").replace("'./manual.mjs'","'./manual.mjs?v="+manual_version+"'"))
    # Preserve browser cache efficiency without showing a stale UI after local rebuilds.
    document=(dist/'index.html').read_text()
    if config.get('localOnly') is False:
        document=document.replace('LOCAL PREVIEW · 2026','PUBLIC BETA · 2026')
    for asset in ['styles.css','launch.css','app.js']:
        version=hashlib.sha256((dist/asset).read_bytes()).hexdigest()[:12]
        document=document.replace('"'+asset+'"','"'+asset+'?v='+version+'"')
    (dist/'index.html').write_text(document)
    assets = dist/'assets'; assets.mkdir(exist_ok=True)
    if (SITE/'media').exists():shutil.copytree(SITE/'media',assets,dirs_exist_ok=True)
    downloads = dist/'downloads'; downloads.mkdir(exist_ok=True)
    b = Path(os.environ.get('RHYTHMER_BUILD_DIR',config.get('buildDirectory',ROOT/('rhythmer/build-ssd' if (ROOT/'rhythmer/build-ssd').exists() else 'rhythmer/build'))))
    os.environ['RHYTHMER_BUILD_DIR']=str(b)
    manifest = json.loads((b/'Rhythmer-artifacts.json').read_text())
    fw = b/'Rhythmer.fwsc'
    digest = hashlib.sha256(fw.read_bytes()).hexdigest()
    if digest != manifest['firmware_sha256']:
        raise SystemExit('Firmware does not match canonical artifact manifest.')
    shutil.copy2(b/'Rhythmer-home.png', assets/'home.png')
    if (root_break_tools:=ROOT/'qa/break-seq-tools/break-seq-menu.png').exists():
        shutil.copy2(root_break_tools, assets/'break-seq-menu.png')
    Image.open(b/'md-one.pcm.ui.ppm').save(assets/'md-mode.png')
    for name,count in [('acid',2),('break',3)]:
        image = Image.open(b/f'Rhythmer-{name}-tabs.png')
        w,h = image.size
        for i in range(count): image.crop((w*i//count,0,w*(i+1)//count,h)).save(assets/f'{name}-{i+1}.png')
    for name in ['Rhythmer.fwsc','Rhythmer-six-track-test.fm1save']:
        shutil.copy2(b/name, downloads/name)
    # Preserve the published print edition during a fresh firmware-site build.
    manual_name = 'Rhythmer-Manual-v'+config.get('version','0.1.1-beta3')+'.pdf'
    for candidate in (ROOT/'docs/manual'/manual_name, ROOT/'output/pdf'/manual_name):
        if candidate.is_file():
            shutil.copy2(candidate, downloads/manual_name)
            break
    build_web_start(ROOT,b,downloads,digest)
    shutil.copy2(ROOT/'LICENSE', assets/'LICENSE.txt')
    shutil.copy2(ROOT/'rhythmer/firmware/hortator/MIT-Mutable-Instruments.txt', assets/'Hortator-MIT-Mutable-Instruments.txt')
    (assets/'favicon.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64"><rect width="64" height="64" fill="#111"/><path d="M12 50V14h11v36m10 0V14h8v36m10 0V14h3v36" stroke="#eee" stroke-width="4"/></svg>')
    files = []
    for f in downloads.iterdir():
        if f.is_file(): files.append({'name':f.name,'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
    release = {'version':config.get('version','Rhythmer dev'),'product':config.get('product','FM-1_900'),'sha256':digest,'physicalHardwareTested':config.get('ownerTest',{}).get('firmwareSha256')==digest,'ownerTest':config.get('ownerTest'),'localOnly':config.get('localOnly',True),'files':files,'sampleNotice':'Includes third-party break recordings; recording/composition rights are separate from the code license. Redistribution clearance is not documented.'}
    release['releaseHold']=config.get('releaseHold')
    release['buildRevision']=config.get('buildRevision')
    release['audioBaseTest']=config.get('audioBaseTest')
    release['releaseCaution']=config.get('releaseCaution')
    (dist/'release.json').write_text(json.dumps(release,indent=2)+'\n')
    validation_file=dist/'RELEASE-VALIDATION.json'
    validation_file.unlink(missing_ok=True)
    for candidate in (b/'RELEASE-VALIDATION.json',ROOT/'RELEASE-VALIDATION.json'):
        if candidate.is_file() and json.loads(candidate.read_text()).get('firmwareSha256')==digest:
            shutil.copy2(candidate,validation_file)
            break
    (dist/'config.json').write_text(json.dumps({k:v for k,v in config.items() if k!='buildDirectory'},ensure_ascii=False,indent=2)+'\n')
    presets = {}
    for filename,symbol,model in [('eng_serum.c','SERUM','SERUM'),('eng_phase.c','PHASE','PHASE'),('eng_lofi.c','LOFI','LOFI'),('eng_mono2k.c','M2STAT','FM+STAT'),('eng_mono2k.c','M2SAW','SWAV-SAW'),('eng_mono2k.c','M2SID','SID-6581')]:
        text = (ROOT/'rhythmer/firmware/src'/filename).read_text()
        block = re.search(r'static const preset_t '+symbol+r'_PRESETS\[\]\s*=\s*\{(.*?)\n\};',text,re.S)
        if not block: raise SystemExit('Missing presets: '+symbol)
        presets[model] = re.findall(r'\{\s*"([^"]+)"',block.group(1))
    (dist/'presets.json').write_text(json.dumps(presets,indent=2)+'\n')
    build_emulator(ROOT, dist, digest)
    if config.get('localOnly') is False:
        p=dist/'emulator/firmwares.json';catalog=json.loads(p.read_text())
        catalog['firmwares'][0].update(version=release['version'],about='Hands-on groovebox firmware for the FM-1. Public beta candidate; see release-specific test scope.')
        p.write_text(json.dumps(catalog,indent=2)+'\n')
    build_editor(ROOT, dist)
    build_installer(ROOT, dist, release)
    apply_site_navigation(ROOT, dist)
    # The unified installer adds licensed recovery assets and a local-only audio bundle.
    release['files']=[{'name':str(f.relative_to(downloads)),'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in sorted(downloads.rglob('*')) if f.is_file()]
    release['installation']='One integrated firmware image includes Amen/Think; no separate sample upload. Official V15 recovery included under Apache-2.0.'
    release['audioPublicRedistributionCleared']=False
    (dist/'release.json').write_text(json.dumps(release,indent=2)+'\n')
    print('LOCAL BUILD:',dist.resolve())
    print('Firmware SHA256:',digest)
    print('Preset counts:',{k:len(v) for k,v in presets.items()})
    print('No deployment, registration or firmware write performed.')

if __name__=='__main__': build()
