#!/usr/bin/env python3
"""Fail-closed installer, source provenance and current release checks (no devices)."""
import hashlib,json,re,runpy,struct,zlib,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
# Reuse the existing dependency-free macOS JavaScriptCore harness and retain its regressions.
harness={'__file__':str(ROOT/'site/tests/test_site.py')};exec((ROOT/'site/tests/test_site.py').read_text().split('model=(SRC/')[0],harness);run=harness['run']
src=ROOT/'site/installer';dist=Path(os.environ.get('RHYTHMER_SITE_DIST',ROOT/'site/dist'))/'installer'
for p in src.glob('*.mjs'):
    s=re.sub(r'^import .*?;\n','',p.read_text(),flags=re.M).replace('export ','')
    s=s.replace('{validateStockPackage,sha256hex};','')
    run('async function syntax(){\n'+s+'\n}; "OK"')
vendor=ROOT/'site/vendor/upstream-installer';up=json.loads((vendor/'UPSTREAM.json').read_text())
for name,meta in up['files'].items():assert hashlib.sha256((vendor/name).read_bytes()).hexdigest()==meta['sha256']
expected=(vendor/'fm1ota.js').read_text().replace('/fm-?1|felucca|','/rhythmer|fm-?1|felucca|')
expected=expected.replace('onProgress(served, r.addr);','onProgress(served, r.addr, r.len, image.length);').replace('(n) => step("verify", n)','(n,address,bytes,imageBytes) => step("verify",n,{address,bytes,imageBytes})').replace('(n) => step("write", n)','(n,address,bytes,imageBytes) => step("write",n,{address,bytes,imageBytes})')
assert (dist/'transport.mjs').read_text()==expected
manifest=json.loads((dist/'manifest.json').read_text())
assert manifest['localOnly'] is json.loads((dist.parent/'config.json').read_text())['localOnly'] and manifest['physicalHardwareTested'] is (manifest.get('ownerTest',{}).get('firmwareSha256')==manifest['firmware']['sha256'])
for meta in [manifest['firmware'],*manifest['loops']]:
    p=dist/meta['url'];data=p.read_bytes();assert len(data)==meta['bytes'];assert hashlib.sha256(data).hexdigest()==meta['sha256']
    if p.suffix=='.slot':
        h=struct.unpack_from('<13I',data)
        assert (h[0],h[4],h[8],h[9],h[12])==(0x504d5346,72000,0,48000,23777)
        assert zlib.crc32(data[512:72512])==h[5]
official=ROOT/'site/vendor/m-vave-v15'
provenance=json.loads((official/'UPSTREAM.json').read_text())
for name,meta in provenance['files'].items():assert hashlib.sha256((official/name).read_bytes()).hexdigest()==meta['sha256']
for meta in [manifest['recovery'],manifest['recoveryArchive'],manifest['bundle'],manifest['archive']]:
    data=(dist/meta['url']).read_bytes();assert len(data)==meta['bytes'];assert hashlib.sha256(data).hexdigest()==meta['sha256']
assert manifest['recovery']['license']=='Apache-2.0'
assert (dist/'../downloads/official-v15/APACHE-2.0.txt').is_file()
assert (dist/'../downloads/official-v15/NOTICE.txt').is_file()
import base64,zipfile,sys
bundle=json.loads((dist/manifest['bundle']['url']).read_text())
assert bundle['format']=='rhythmer-install' and len(bundle['loops'])==2
for meta in [bundle['firmware'],bundle['recovery'],*bundle['loops']]:assert hashlib.sha256(base64.b64decode(meta['data'])).hexdigest()==meta['sha256']
with zipfile.ZipFile(dist/manifest['archive']['url']) as z:
    assert z.read('firmware/Rhythmer.fwsc')==(dist.parent/'downloads/Rhythmer.fwsc').read_bytes()
    assert z.read('official-v15/FM-1_015.fwsc')==(official/'FM-1_015.fwsc').read_bytes()
    for filename in ['samples/amen-160.slot','samples/think-160.slot','THIRD-PARTY-AUDIO.txt','audio-rights.json','official-v15/APACHE-2.0.txt','official-v15/NOTICE.txt']:assert filename in z.namelist()
sys.path.insert(0,str(ROOT/'tools/six'))
from release_rights import public_audio_release_blockers
assert len(public_audio_release_blockers())==2
assert bundle['publicRedistributionCleared'] is False

ui=(src/'ui.mjs').read_text();core=(src/'core.mjs').read_text()
assert 'confirmSource(up,source)' in ui and 'up.expectedFirst=source' in core
assert 'navigator.locks.request' in ui and "new BroadcastChannel('jiant-midi')" in ui
assert "validateStockPackage(new Uint8Array(await f.arrayBuffer()))" in ui
assert "sessionStorage.setItem(checkpointKey" in ui and "beforeunload" in ui
assert "showModal()" in ui and 'window.confirm' not in ui
assert 'requestMIDIAccess' not in (src/'tests.mjs').read_text()+(src/'fake.mjs').read_text()
assert "captureBackup" in core and "{complete:true}" in core
assert 'export async function uploadLoops' not in core
assert 'c.request([80,[]]' in core and 'verifyEmbeddedBanks' in core
print('PASS: installer module syntax, immutable JIANT source hashes, guarded device entry, local recovery gate, both sample hashes/CRCs, licensed stock V15, complete install archive and explicit audio public-release blockers')

assert "$('#confirm-notice').checked" in ui
assert 'id="confirm-notice"' in (src/'index.html').read_text()
assert "uploadLoops" not in ui and "onAudioProgress" not in ui and "installLoops:" not in ui
assert "verifyEmbeddedBanks(source)" in ui
assert 'id="loops"' not in (src/"index.html").read_text() and 'id="repair"' not in (src/"index.html").read_text()
assert manifest["firmware"]["embeddedBreaks"] is True
assert 'LEGAL-NOTICE.txt' in zipfile.ZipFile(dist/manifest['archive']['url']).namelist()
print('PASS: mandatory independence notice, integrated-bank install without a separate sample upload, resume choice and bundled legal notice')

# Rights acknowledgement is independent, unchecked on every review; V15 has no bundled audio.
assert 'id="confirm-audio-rights" type="checkbox"' in (src/"index.html").read_text()
assert "$('#confirm-audio-rights').checked=false" in ui
assert "$('#confirm-audio-row').hidden||$('#confirm-audio-rights').checked" in ui
assert 'id="confirm-v15" type="checkbox"' in (src/'index.html').read_text()
assert "$('#confirm-v15').checked=false" in ui
assert "$('#confirm-v15-row').hidden||$('#confirm-v15').checked" in ui
assert "$('#confirm-v15-row').hidden=mode==='stock'" in ui
print('PASS: V15 recommendation acknowledgement is required for Rhythmer, reset every review and omitted for stock recovery')
