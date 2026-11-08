from sample_assets import sample_dir
"""Build a local installation bundle: firmware, two loops and licensed stock recovery."""
import base64, hashlib, json, shutil, struct, zlib, zipfile
from pathlib import Path

def digest(data):return hashlib.sha256(data).hexdigest()
def build_installer(root, dist, release):
    local=release.get('localOnly',True)
    dest=dist/'installer';dest.mkdir(exist_ok=True)
    for source in [root/'site/vendor/upstream-installer',root/'site/installer']:
        for file in source.iterdir():
            if file.is_file():shutil.copy2(file,dest/file.name)
    page=dest/'index.html';html=page.read_text()
    if release.get('physicalHardwareTested'):
        html=html.replace('Development build · physical-device validation pending.','Public beta · limited creator hardware testing.').replace('Browser and native tests do not establish safe flashing, power-loss recovery or sustained operation on a real FM-1.','The creator confirmed playback and official V15 installation/return on this release. Cold power-loss recovery and sustained hardware reliability are not established.')
    if not local:
        html=html.replace('RHYTHMER / DEVELOPMENT PREVIEW','RHYTHMER / PUBLIC BETA')
        if not release.get('physicalHardwareTested') and release.get('audioBaseTest'):
            html=html.replace('Development build · physical-device validation pending.','Public beta · creator-tested audio base.').replace('This installer is for deliberate development testing.','The creator confirmed stable playback on the native Hortator audio base. Review the build notes for this maintenance revision and its exact test scope.')
        elif not release.get('physicalHardwareTested'):
            html=html.replace('Development build · physical-device validation pending.','Optimized public beta · this exact binary awaits physical FM-1 revalidation.').replace('This installer is for deliberate development testing.','The reported real-device audio-load issue is not yet confirmed resolved.')
    page.write_text(html)
    shutil.copy2(root/'site/src/midi.mjs',dest/'midi.mjs')
    transport=(dest/'fm1ota.js').read_text().replace('/fm-?1|felucca|','/rhythmer|fm-?1|felucca|')
    for before,after in [
        ('onProgress(served, r.addr);','onProgress(served, r.addr, r.len, image.length);'),
        ('(n) => step("verify", n)','(n,address,bytes,imageBytes) => step("verify",n,{address,bytes,imageBytes})'),
        ('(n) => step("write", n)','(n,address,bytes,imageBytes) => step("write",n,{address,bytes,imageBytes})')]:
        if before not in transport:raise ValueError('Progress hook anchor missing: '+before)
        transport=transport.replace(before,after)
    (dest/'transport.mjs').write_text(transport)
    shutil.copy2(root/'site/LEGAL-NOTICE.txt',dist/'LEGAL-NOTICE.txt')
    downloads=dist/'downloads';recoveryDir=downloads/'official-v15';recoveryDir.mkdir(exist_ok=True)
    vendor=root/'site/vendor/m-vave-v15';provenance=json.loads((vendor/'UPSTREAM.json').read_text())
    stock=(vendor/'FM-1_015.fwsc').read_bytes()
    assert len(stock)==699956 and digest(stock)=='db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a'
    for name,meta in provenance['files'].items():assert digest((vendor/name).read_bytes())==meta['sha256']
    for file in vendor.iterdir():
        if file.is_file():shutil.copy2(file,recoveryDir/file.name)
    rights=json.loads((root/'site/audio-rights.json').read_text());(dist/'audio-rights.json').write_text(json.dumps(rights,indent=2)+'\n')
    notice='''RHYTHMER / THIRD-PARTY AUDIO — LOCAL DEVELOPMENT BUNDLE

A / AMEN — "Amen, Brother", The Winstons (1969).
Drums: Gregory C. Coleman. Amenity credits composition to Richard L. Spencer.
Source: https://github.com/ejtb76/amenity/tree/main/assets/third-party/amen
B / THINK — "Think (About It)", Lyn Collins.
Source: user-supplied Classic Break Pack, lyncollins_think_1.wav and
lyncollins_think_2.wav. Drum intervals assembled into two complementary bars;
pitched tail/carry-in intervals excluded. Original files retained by the owner.

Both: mono PCM12 / 16 kHz / two bars at 160 BPM / 3 seconds. Prepared
with cropping, repitch/resampling, level adjustment and endpoint fades.
These recordings are NOT covered by Rhythmer's software license. Rights
remain with the respective recording and composition rights holders.
No permission for public firmware/sample redistribution or musical releases
has been documented for these copies. Credits and notices do not grant it.
Local-only status is a distribution restriction in this build process, not
a claim that personal or non-commercial use is automatically lawful.

Before public distribution: document the permissions needed for distributing
these recordings as firmware/sample assets, or replace/omit them. A license
allowing samples in a finished song may not allow a redistributable sample bank.
See audio-rights.json for source URLs, exact slot hashes and clearance status.
'''
    if not local:
        notice=notice.replace('LOCAL DEVELOPMENT BUNDLE','BUNDLED THIRD-PARTY RECORDINGS').replace('Local-only status is a distribution restriction in this build process, not\na claim that personal or non-commercial use is automatically lawful.','The project owner directs public distribution with these credits. This\ndecision is not rights-holder permission, and does not clear any user use.').replace('Before public distribution: document the permissions needed for distributing','Recording/composition permissions may be needed for distributing')
    (downloads/'THIRD-PARTY-AUDIO.txt').write_text(notice)
    loops=[];encoded=[]
    for i,(filename,label) in enumerate([('amen-160.slot','Amen'),('think-160.slot','Think')]):
        data=(sample_dir(root)/filename).read_bytes();words=struct.unpack_from('<13I',data)
        assert len(data)==81920 and data[:7]==b'FSMP\x0c\0\x01'
        assert (words[4],words[8],words[9],words[12])==(72000,0,48000,23777) and zlib.crc32(data[512:72512])==words[5]
        assert digest(data)==rights['samples'][i]['slotSHA256']
        shutil.copy2(sample_dir(root)/filename,downloads/filename)
        meta=dict(name=label,slot=rights['samples'][i]['slot'],url='../downloads/'+filename,bytes=len(data),sha256=digest(data))
        loops.append(meta);encoded.append({**meta,'data':base64.b64encode(data).decode()})
    for preview in ('think-160-preview.wav','think-160-repeat.wav'):
        if (sample_dir(root)/preview).exists():shutil.copy2(sample_dir(root)/preview,downloads/preview)
    fw=(downloads/'Rhythmer.fwsc').read_bytes()
    local=release.get('localOnly',True)
    firmware=dict(url='../downloads/Rhythmer.fwsc',product=release.get('product','FM-1_900'),sha256=digest(fw),bytes=len(fw),embeddedBreaks=True)
    recovery=dict(url='../downloads/official-v15/FM-1_015.fwsc',product='FM-1_015',sha256=digest(stock),bytes=len(stock),license='Apache-2.0',source=provenance['files']['FM-1_015.fwsc']['url'])
    package=dict(format='rhythmer-install',version=2,audioEmbedded=True,localOnly=local,publicRedistributionCleared=False,
                 firmware={**firmware,'data':base64.b64encode(fw).decode()},loops=encoded,recovery={**recovery,'data':base64.b64encode(stock).decode()},audioRights=rights,audioNotice=notice)
    data=(json.dumps(package,separators=(',',':'))+'\n').encode();bundle=downloads/'Rhythmer-with-breaks.rhythmer';bundle.write_bytes(data)
    readme='''RHYTHMER / LOCAL DEVELOPMENT INSTALLATION

Open http://127.0.0.1:8766/installer/ in desktop Chrome or Edge and press
INSTALL RHYTHMER. The page loads this exact installation bundle automatically,
identifies one FM-1, prepares a backup where supported, and requests a final
review. The integrated firmware image includes both Break banks. One firmware
transfer writes the app and fixed audio banks; no separate sample upload runs.
The installer verifies device identity and the firmware's bank-validity flags.

Rhythmer.fwsc includes the app, integrated loader and both factory Break banks.
The .rhythmer file additionally wraps recovery and provenance; it is not a format the stock updater can
flash directly. Do not feed it to M-UPGRADE. Official V15 recovery is separate
and unmodified; its Apache-2.0 license, source and notices are included.

Hardware flashing, recovery and playback remain physically untested.
Musical archives exclude Break audio; retain original audio files separately.
This bundle contains original Amen/Think recordings whose public redistribution
permission is not documented. Do not publish the bundle on that basis.
Read THIRD-PARTY-AUDIO.txt and audio-rights.json. Credits are not a license.
'''
    if not local:
        readme=readme.replace('LOCAL DEVELOPMENT INSTALLATION','PUBLIC BETA INSTALLATION').replace('Open http://127.0.0.1:8766/installer/ in desktop Chrome or Edge and press','Open the installer linked in the repository README in Chrome or Edge. Press').replace('This bundle contains original Amen/Think recordings whose public redistribution\npermission is not documented. Do not publish the bundle on that basis.','This bundle contains original Amen/Think recordings. The owner directs their\ndistribution with credits; rights-holder redistribution permission has not\nbeen documented. This is not a blanket license for users.').replace('Hardware flashing, recovery and playback remain physically untested.','Read the release-specific test record and change log. Host and emulator\nchecks passed; this exact binary is not physically tested. Earlier owner\nhardware results do not automatically apply to this package.')
        page=dest/'index.html';text=page.read_text().replace('LOCAL DEVELOPMENT','PUBLIC BETA').replace('LOCAL INSTALL BUNDLE','INSTALL BUNDLE').replace('COMPLETE LOCAL INSTALLATION','COMPLETE INSTALLATION').replace('This build and its samples remain local.','The project owner directs distribution with credits; this is not rights-holder permission.').replace('complete local installation bundle','complete installation bundle');page.write_text(text)
    if not local and release.get('audioBaseTest'):
        readme=readme.replace('Host and emulator\nchecks passed; this exact binary is not physically tested. Earlier owner\nhardware results do not automatically apply to this package.','The creator confirmed stable playback on audio-base build 20261108.6.\nThis maintenance revision updates the display and product identity copy.\nSee the exact hash-bound record for the scope of each test.')
    archive=downloads/('Rhythmer-dev-local-install.zip' if local else 'Rhythmer-'+release['version']+'-install.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(bundle,bundle.name);z.write(downloads/'Rhythmer.fwsc','firmware/Rhythmer.fwsc')
        for meta in loops:z.write(downloads/Path(meta['url']).name,'samples/'+Path(meta['url']).name)
        z.write(root/'site/LEGAL-NOTICE.txt','LEGAL-NOTICE.txt');z.writestr('README.txt',readme);z.writestr('THIRD-PARTY-AUDIO.txt',notice);z.writestr('audio-rights.json',json.dumps(rights,indent=2));z.write(root/'LICENSE','licenses/Rhythmer-GPL-3.0.txt');z.write(root/'rhythmer/firmware/hortator/MIT-Mutable-Instruments.txt','licenses/Hortator-MIT-Mutable-Instruments.txt')
        for f in recoveryDir.iterdir():z.write(f,'official-v15/'+f.name)
    archives=Path('/Volumes/SSD/Development/Archives/rhythmer');archives.mkdir(parents=True,exist_ok=True);shutil.copy2(archive,archives/archive.name)
    recoveryArchive=downloads/'FM-1-V15-recovery.zip'
    with zipfile.ZipFile(recoveryArchive,'w',zipfile.ZIP_DEFLATED) as z:
        for f in vendor.iterdir():
            if f.is_file():z.write(f,f.name)
    shutil.copy2(recoveryArchive,archives/recoveryArchive.name)
    manifest=dict(version=release['version'],channel='local-development' if local else 'public-beta',physicalHardwareTested=release.get('physicalHardwareTested',False),localOnly=local,ownerTest=release.get('ownerTest'),firmware=firmware,loops=loops,recovery=recovery,
                  recoveryArchive=dict(url='../downloads/'+recoveryArchive.name,bytes=recoveryArchive.stat().st_size,sha256=digest(recoveryArchive.read_bytes())),bundle=dict(url='../downloads/'+bundle.name,bytes=len(data),sha256=digest(data)),archive=dict(url='../downloads/'+archive.name,bytes=archive.stat().st_size,sha256=digest(archive.read_bytes())),audioRightsURL='../audio-rights.json',publicRedistributionCleared=False)
    if release.get('releaseHold'):manifest['releaseHold']=release['releaseHold']
    if release.get('buildRevision'):manifest['buildRevision']=release['buildRevision']
    (dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    fixture=(dest/'index.html').read_text().replace('src="ui.mjs"','src="ui-fixture.mjs"').replace('<title>Rhythmer / Installer</title>','<title>SIMULATION / Rhythmer Installer</title>')
    (dest/'ui-tests.html').write_text(fixture)
    # Bust the full first-party module graph on each installer change, not only the entry point.
    version=digest(b''.join(p.read_bytes() for p in sorted(dest.glob('*.mjs'))))[:12]
    import re
    for p in dest.glob('*.mjs'):
        p.write_text(re.sub(r"(from\s+['\"])(\./[^'\"]+\.(?:mjs|js))(['\"])",lambda m:m[1]+m[2]+'?v='+version+m[3],p.read_text()))
    for p in dest.glob('*.mjs'):
        p.write_text(re.sub(r"(import\(['\"])(\./[^'\"]+\.(?:mjs|js))(['\"]\))",lambda m:m[1]+m[2]+'?v='+version+m[3],p.read_text()))
    for name in ['index.html','tests.html','ui-tests.html']:
        p=dest/name;s=p.read_text().replace('href="installer.css"',f'href="installer.css?v={digest((dest/"installer.css").read_bytes())[:12]}"').replace('src="ui.mjs"',f'src="ui.mjs?v={version}"').replace('src="ui-fixture.mjs"',f'src="ui-fixture.mjs?v={version}"').replace('import("./tests.mjs")',f'import("./tests.mjs?v={version}")');p.write_text(s)
