"""Validate the curated Pages artifact before a deployment job can publish it."""
from pathlib import Path
import hashlib,json,re,sys,struct
R=Path.cwd();v=json.loads((R/'PUBLISH.json').read_text())
draft='--draft' in sys.argv
assert v['repository'] or draft,'Connect the real GitHub owner before deployment'
if v['repository']:
 assert re.fullmatch(r'[A-Za-z0-9-]+/Rhythmer',v['repository'])
 assert v['site']==f"https://{v['repository'].split('/')[0].lower()}.github.io/Rhythmer/"
assert v['ownerDistributionDecision'] is True and v['rightsHolderClearanceDocumented'] is False
M=json.loads((R/'www/installer/manifest.json').read_text())
assert M['localOnly'] is False and M['firmware']['product']==v['product']
assert M['firmware']['sha256']==v['firmwareSha256']
for m in (M['firmware'],M['bundle'],M['archive'],M['recovery'],M['recoveryArchive'],*M['loops']):
 p=R/'www/installer'/m['url'];b=p.read_bytes();assert len(b)==m['bytes'] and hashlib.sha256(b).hexdigest()==m['sha256']
hub=json.loads((R/'fmhub.json').read_text());assert 20<=len(hub['what'])<=300
assert not set(hub)-{'what','author','kinds','play','editor','cheat','image','color'}
assert (R/hub['image']).is_file()
picture=(R/hub['image']).read_bytes();assert picture[:8]==b'\x89PNG\r\n\x1a\n' and len(picture)<=300*1024
w,h=struct.unpack('>II',picture[16:24]);assert w==h and 240<=w<=1024
assert not any(p.is_symlink() for p in (R/'www').rglob('*'))
for forbidden in ('ui-tests.html','fake.mjs','ui-fixture.mjs'):assert not (R/'www/installer'/forbidden).exists()
print('Draft preparation checks: PASS; publication disabled until the account is configured' if draft and not v['repository'] else 'Reviewed public website, package checksums and Hub metadata: PASS')
