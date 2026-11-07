"""Build a pinned, MIT-licensed fm1-emu runtime with Rhythmer's local package and seed.
Never downloads at build time. The original upstream files and hashes stay untouched in vendor/.
"""
from pathlib import Path
import hashlib, json, shutil

def build_emulator(root, dist, firmware_hash):
    vendor=root/'site/vendor/fm1-emu'; target=dist/'emulator'; target.mkdir(exist_ok=True)
    provenance=json.loads((vendor/'UPSTREAM.json').read_text())
    for name,meta in provenance['files'].items():
        data=(vendor/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=meta['sha256']:raise ValueError('Upstream runtime changed: '+name)
        (target/name).write_bytes(data)
    shutil.copy2(vendor/'LICENSE',target/'LICENSE.txt')
    shutil.copy2(vendor/'UPSTREAM.json',target/'UPSTREAM.json')
    source=(vendor/'main.js').read_text()
    anchor='''      if (seq !== bootSeq) return false;
      await flushRunning(true);'''
    replacement='''      // Rhythmer: seed only a new session. Saved flash for this exact hash always wins.
      if (!record && !options.imported && f.id === "rhythmer") {
        const response = await fetch("../downloads/Rhythmer-start.fm1save?build=" + RHYTHMER_HASH, {cache:"no-store"});
        if (!response.ok) throw new Error(`Rhythmer session: HTTP ${response.status}`);
        record = await importSave(await response.text());
        if (record.hash !== hash || hash !== RHYTHMER_HASH) throw new Error("Rhythmer session/package hash mismatch");
        record.name = "Rhythmer";
      }
      if (seq !== bootSeq) return false;
      await flushRunning(true);'''
    if source.count(anchor)!=1:raise ValueError('Upstream boot hook changed')
    source=source.replace(anchor,replacement)
    anchor='''async function packageOf(f) {
  if (f.id === "local")'''
    replacement='''async function packageOf(f) {
  // The project landing page always opens its current published package.
  // Older package saves remain in the library under their own hashes.
  if (f.id === "rhythmer") {
    let got = packages.get(f.id);
    if (!got || await packageHash(got.bytes) !== RHYTHMER_HASH) {
      got = await fetchPackage(f, url => fetch(url, {cache:"no-store"}));
      if (await packageHash(got.bytes) !== RHYTHMER_HASH) throw new Error("Rhythmer package changed: reload the project page");
      packages.set(f.id, got);
    }
    return got;
  }
  if (f.id === "local")'''
    if source.count(anchor)!=1:raise ValueError('Upstream package-selection hook changed')
    source=source.replace(anchor,replacement)
    source='// Local Rhythmer integration; upstream MIT license in LICENSE.txt.\nconst RHYTHMER_HASH = '+json.dumps(firmware_hash)+';\n'+source
    source=source.replace('const CLOCKS = [60, 80, 100, 120, 160, 200, 240];','const CLOCKS = [120, 160, 200, 240];')
    source=source.replace('if (DEVICE === "fm1") {','if (DEVICE === "fm1" && !EMBED) {',1)
    source=source.replace('  startBtn.hidden = true;','  startBtn.hidden = true;\n  document.querySelectorAll("[data-rhy-control]").forEach(b => b.disabled = false);',1)
    source=source.replace('  frame = m;','  frame = m;\n  updateRhythmerMeter(m.outputPeak || 0);',1)
    export_old = (vendor/'main.js').read_text().split('  const url = URL.createObjectURL(new Blob([exportSave(record)]',1)[1].split('\n}));',1)[0]
    export_old = '  const url = URL.createObjectURL(new Blob([exportSave(record)]' + export_old
    if source.count(export_old)!=1: raise ValueError('Upstream export hook changed')
    source=source.replace(export_old,'  prepareRhythmerExport(record);')
    export_start='document.getElementById("export-save").addEventListener("click", action(async () => {\n  if (!running) throw new Error("start a firmware first");\n  await flushRunning();'
    export_new='document.getElementById("export-save").addEventListener("click", action(async () => {\n  if (!running) throw new Error("start a firmware first");\n  const wasPaused = ac.state === "suspended";\n  try { await flushRunning(); } finally { if (wasPaused) await ac.suspend(); }'
    if source.count(export_start)!=1: raise ValueError('Upstream export start changed')
    source=source.replace(export_start,export_new)
    source=source.replace('function draw() {','let rhyLastDraw = 0;\nfunction draw() {\n  const rhyNow = performance.now();\n  if (document.hidden || rhyNow - rhyLastDraw < 1000 / 24) { requestAnimationFrame(draw); return; }\n  rhyLastDraw = rhyNow;')
    source+='\n'+(root/'site/emulator/adapter.js').read_text()
    (target/'main.js').write_text(source)
    worklet=(vendor/'worklet.js').read_text().replace('sampleRate / 30','sampleRate / 20')
    anchor='''    out[0].set(l);
    if (out[1]) out[1].set(r);'''
    if worklet.count(anchor)!=1:raise ValueError('Upstream PCM hook changed')
    worklet=worklet.replace(anchor,anchor+'''\n    // Actual PCM output meter for the Rhythmer interface (not synthetic animation).
    let peak = this.rhythmerPeak || 0;
    for (let i = 0; i < n; i++) peak = Math.max(peak, Math.abs(l[i]), Math.abs(r[i]));
    this.rhythmerPeak = peak;''')
    worklet=worklet.replace('short: this.short, halted:', 'outputPeak: this.rhythmerPeak || 0, short: this.short, halted:')
    worklet=worklet.replace('      const took = now() - t0;', '      this.rhythmerPeak = 0;\n      const took = now() - t0;')
    (target/'worklet.js').write_text(worklet)
    html=(vendor/'index.html').read_text().replace('<title>FM-1 Emulator</title>','<title>Rhythmer — FM-1 Emulator</title>')
    html=html.replace('</head>','<link rel="stylesheet" href="rhythmer.css">\n</head>')
    html=html.replace('id="start">Start</button>','id="start">Start Rhythmer</button>')
    html=html.replace('id="export-save">Export…','id="export-save">Prepare backup')
    html=html.replace('<p class="rotate">','''<div class="rhy-controls" aria-label="Emulator controls"><button data-rhy-control data-key="12" disabled>PLAY / STOP</button><button data-rhy-control data-enc="-1" disabled>TRACK −</button><button data-rhy-control data-enc="1" disabled>TRACK +</button><button data-rhy-control data-key="8" disabled>HOME</button><button data-rhy-control data-key="6" disabled>EDIT</button><button data-rhy-control data-key="11" disabled>SEQ</button><button data-rhy-control id="rhy-audio" disabled>Pause audio</button><span class="rhy-meter"><meter id="rhy-level" min="0" max="1" value="0" aria-label="Actual audio output level"></meter><output id="rhy-db" aria-live="off">Output −∞ dBFS</output></span></div><p class="rotate">''',1)
    html=html.replace('<span class="rhy-meter">','<button data-rhy-control id="rhy-fresh" disabled>RESET / EMPTY</button><span class="rhy-meter">')
    html=html.replace('src="main.js"','src="main.js?v='+hashlib.sha256(source.encode()).hexdigest()[:12]+'"')
    (target/'index.html').write_text(html)
    shutil.copy2(root/'site/emulator/rhythmer.css',target/'rhythmer.css')
    catalog={'default':'rhythmer','firmwares':[{'id':'rhythmer','name':'Rhythmer','version':'dev','author':'Jazzy Rhythmer','license':'GPL-3.0','repo':'../#releases','package':'../downloads/Rhythmer.fwsc','about':'Six-track groovebox firmware. Local development build.'}]}
    (target/'firmwares.json').write_text(json.dumps(catalog,indent=2)+'\n')
    (target/'INTEGRATION.txt').write_text('Rhythmer integration: local-only catalog, exact-hash initial sample/project seed, existing-save preservation, accessible panel buttons, pause/resume, PCM output meter. Runtime: Joakim Eriksson, MIT. Original files: site/vendor/fm1-emu.\n')
