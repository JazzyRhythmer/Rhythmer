#!/usr/bin/env python3
"""Refresh first-party page content only; never rebuild installation assets."""
from pathlib import Path
import argparse, hashlib, json, shutil

R = Path(__file__).resolve().parents[2]
ap = argparse.ArgumentParser()
ap.add_argument('--dist', required=True, type=Path)
args = ap.parse_args()
dist = args.dist.resolve()
assert dist.is_relative_to(Path('/Volumes/SSD/Development'))
assert (dist / 'installer/manifest.json').is_file()
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
protected = [p for directory in ('installer', 'downloads', 'emulator', 'editor')
             for p in (dist / directory).rglob('*') if p.is_file()]
before = {str(p.relative_to(dist)): sha(p) for p in protected}
for p in (R / 'site/src').iterdir():
    if p.is_file():
        shutil.copy2(p, dist / p.name)
app = dist / 'app.js'
text = app.read_text()
for name in ('landing.mjs', 'manual.mjs', 'newsletter.mjs'):
    text = text.replace("'./" + name + "'", "'./" + name + '?v=' + sha(dist / name)[:12] + "'")
app.write_text(text)
index = dist / 'index.html'
text = index.read_text().replace('LOCAL PREVIEW · 2026', 'PUBLIC BETA · 2026')
for name in ('styles.css', 'launch.css', 'app.js'):
    text = text.replace('"' + name + '"', '"' + name + '?v=' + sha(dist / name)[:12] + '"')
index.write_text(text)
after = {str(p.relative_to(dist)): sha(p) for p in protected}
assert before == after, 'Protected installer, firmware or runtime asset changed'
print(json.dumps({'pageFilesRefreshed': True, 'protectedFilesUnchanged': len(before),
                  'firmwareSha256': json.loads((dist / 'release.json').read_text())['sha256']}, indent=2))
