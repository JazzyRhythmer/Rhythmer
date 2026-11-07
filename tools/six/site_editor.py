"""Adapt the pinned GPL JIANT editor without modifying the reference sources."""
from pathlib import Path
import os,re,shutil,hashlib,json,html

def build_editor(root,dist):
    site=root/'site';vendor=site/'vendor/upstream-editor';out=dist/'editor';out.mkdir(exist_ok=True)
    s=(vendor/'editor.html').read_text()
    release=json.loads((dist/'release.json').read_text()) if (dist/'release.json').exists() else {}
    def change(a,b):
        nonlocal s
        if a not in s:raise ValueError('JIANT adaptation anchor missing: '+a[:80])
        s=s.replace(a,b)
    change('<html lang="ja">','<html lang="en">')
    s=re.sub(r'<link[^>]+(?:fonts.googleapis|fonts.gstatic)[^>]*>','',s)
    s=re.sub(r'<p class="kofi".*?</p>','',s,flags=re.S)
    s=s.replace('JIANT FM by Juan Jiant. GPL-3.0.','Rhythmer editor · Adapted from JIANT by Juan Jiant and Felucca by Leo Kuroshita. GPL-3.0.')
    s=s.replace('../installer/','../#install').replace('JIANT installer','Rhythmer installation').replace('The sound changed on the device: read again','Data refreshed')
    s=s.replace('[EXPECT.gcount, 32, 36, 37, 66, 67, 69, 71]','[71]').replace('[[120, 112], [99, 91], [91, 83], [89, 81], [69, 61], [57, 49]]','[[127, 119]]')
    s=re.sub(r'<h1 class="jiant-wordmark">.*?</h1>','',s)
    s=re.sub(r'<title>.*?</title>','<title>Rhythmer — Web Editor</title>',s)
    change('let lang = (navigator.language || "en").toLowerCase().startsWith("ja") ? "ja" : "en";','let lang = "en";')
    change('const MOCK = QS.get("mock") === "1";','const MOCK = false;\nconst WORKSPACE = QS.get("target") !== "usb";\nlet workspace = null;')
    s=s.replace('if (!MOCK && !navigator.requestMIDIAccess)','if (!WORKSPACE && !navigator.requestMIDIAccess)')
    change('if (MOCK) { mock = makeMockDevice({ legacy: QS.get("legacy") === "1" }); access = mock.access; }','if (WORKSPACE) { const w = await import("./workspace.mjs"); workspace = await w.createWorkspace(m => {$("workspace-status").textContent=m;}); access = workspace.access; }')
    change('if (!/^(?:JIANT|FELUCCA)\\b/i.test(info.version || "")) throw new Error("Not a JIANT editor device");','if (!["Rhythmer dev","Rhythmer v0.1.1-beta3","JIANT Rhythmer dev","JIANT v0.1.1-beta3"].includes(info.version) || ![21,22].includes(info.nengines) || info.ntrk !== 6 || info.pcount !== 127 || info.gcount !== 71 || info.pe0 !== 119) throw new Error("This editor requires the matching Rhythmer firmware");')
    change('function replyMatches(cmd, args, a) {','function replyMatches(cmd, args, a) { if(cmd===80)return args.length?a[0]===args[0]&&a[1]===args[1]:a.length===6&&String.fromCharCode(...a.slice(0,5))===\"SIX3\\0\";')
    change('function engineOrder(names) {','function engineOrder(names) { return [10,17,2,3,18,19,20,0,12,7,16,15,21].filter(i => names[i] && names[i] !== "-"); }\nfunction upstreamEngineOrder(names) {')
    change('["LFO", 0, [9, 10, 11, 12]]','["LFO 1", 0, [9, 10, 11, 12]], ["LFO 2", 0, [pe0-4,pe0-3,pe0-2,pe0-1]]')
    change('["LFO 2", 0, [pe0-4,pe0-3,pe0-2,pe0-1]]','["LFO 2", 0, [pe0-4,pe0-3,pe0-2,pe0-1]], ["LFO 2 DEST", 2, [0,1,2,3]]')
    change('function paramRow(s, id) {','function paramRow(s, id) { if(s===2)return lfo2DestRow(id);')
    change('o.lyric = ty >= 0','o.lfo2Dest = ty >= 3 && trailer[ty-3]===0x32 && trailer[ty-2]===1 && trailer[ty-1]===1;\n    o.lyric = ty >= 0')
    change('function enumShown(d) {','function enumShown(d) { if(d.names?.[1]==="LFO 1"&&d.names?.[15]==="LFO 2")return [0,1,15,2,3,4,5,6,7,8,9,10,11,12,13,14];')
    s=s.replace('["LFO DEST", 0,','["LFO 1 DEST", 0,')
    s=s.replace('["CHORUS", 1,','["PHASER", 1,')
    # Actual command tokens, rather than MIDI pitch names, for the singleton Break track.
    change('const stepNotes = (s) => s.notes.slice(0, s.n).map(noteName).join(" ");','const breakLabel = n => n<16?`C${n+1}`:n<32?`R${n-15}`:n===32?"A":n===33?"B":n===34?"RET":String(n);\nconst isBreak = () => !!dev && dev.dump?.engine===15;\nconst breakParse = text => { const words=String(text).trim().toUpperCase().split(/[ ,]+/).filter(Boolean); if(words.length>4)return null; const ns=words.map(w=>/^C([1-9]|1[0-6])$/.test(w)?+w.slice(1)-1:/^R([1-9]|1[0-6])$/.test(w)?+w.slice(1)+15:w==="A"?32:w==="B"?33:w==="RET"?34:/^([0-9]|[12][0-9]|3[0-4])$/.test(w)?+w:NaN); return ns.some(Number.isNaN)?null:ns; };\nconst stepNotes = s => s.notes.slice(0,s.n).map(isBreak()?breakLabel:noteName).join(" ");')
    change('const notes = parseNotes(c.notes.value), hits = parseHits(c.hits.value);','const notes = isBreak()?breakParse(c.notes.value):parseNotes(c.notes.value), hits = parseHits(c.hits.value);')
    change('c.notes.value = stepNotes(s);','c.notes.value = stepNotes(s); c.notes.setAttribute("aria-label",`${isBreak()?"Break command":"Notes"} ${+tr.dataset.i+1}`); c.notes.placeholder=isBreak()?"C1 / R1 / A / B / RET":"—";')
    change('for (let i = 0; i < 8 && pe0 + i < d.info.pcount; i++) d.pdesc[pe0 + i] = await getDescOrNull(0, pe0 + i, d);','for (const id of [83,84,85,86,87,88,89,90,...Array.from({length:8},(_,i)=>pe0+i)]) d.pdesc[id] = await getDescOrNull(0,id,d);')
    change('const send = () => writeStep(i, tr);','let noteTimer; const send = () => {clearTimeout(noteTimer);writeStep(i,tr);}; notes.addEventListener("input",()=>{clearTimeout(noteTimer);noteTimer=setTimeout(send,180);});')
    change('dev.steps[i] = parse[CMD.STEP_SET](await rq(req.stepSet(i, s)));','dev.steps[i] = parse[CMD.STEP_SET](await rq(req.stepSet(i, s))); renderGrid(); tr.dataset.committed=stepNotes(dev.steps[i]);')
    s=s.replace('$("noteshelp").textContent = t("notesHelp");','$("noteshelp").textContent = isBreak()?"C1–C16 chops · R1–R16 repeats/up-down · A / B samples · RET return. NOTE / TIE / REST control playback; up to four commands per step.":t("notesHelp");')
    change('await restoreBackup((r, o) => sessionRequest(d, r, o), archive, backupProgress);','await restoreBackup((r,o)=>{if(r[0]===CMD.BACKUP_PUT&&r[1][0]===2)selfLoad();return sessionRequest(d,r,o);},archive,backupProgress);')
    # Current data ABI has four projects; old JIANT archives must not be silently truncated.
    s=s.replace('JIANT-backup','Rhythmer-backup')
    s=s.replace('$("version").textContent = d.info.version;', '$("version").textContent = d.info.version==="JIANT Rhythmer dev"?"Rhythmer dev":d.info.version.replace(/^JIANT /,"Rhythmer ");')
    s=s.replace('/felucca|jiant|skiff|fm[ _-]*1|','/rhythmer|felucca|jiant|skiff|fm[ _-]*1|')
    s=s.replace('backupDone: "Backup saved"','backupDone: "Backup ready — use DOWNLOAD above"')
    s=s.replace('backupTitle: "Complete backup"','backupTitle: "Musical archive"').replace('backupHelp: "Save the current music, A–D, sound banks and settings to this computer."','backupHelp: "Save all four projects, A–D sections, sounds and settings. Break sample audio is separate; use PLAY’s .fm1save for sample-inclusive backups."')
    s=s.replace('return lib.filter((p) => (!q', 'return lib.filter((p) => (p.engineName!=="BREAK" || dev?.dump?.engine===15) && (!q')
    s=s.replace('const rows = devicePresetRows(d.info, d.names, p);','const rows = devicePresetRows(d.info, d.names, p).filter(r=>r.engine!==15 || d.dump?.engine===15);')
    s=s.replace('projects: "Projects"','projects: "Sections A–D"')
    # Adaptations inside the original UI closure can use its real protocol/state functions.
    change('applyLang();\nshowTab(tab);\nlibInit();','applyLang();\nshowTab(tab);\nlibInit();\n'+(site/'editor/adapter.js').read_text())
    change('const TABS = ["sound", "fm6", "sequencer", "tracks", "library", "projects", "settings"];','const TABS = ["sound", "fm6", "sequencer", "tracks", "library", "projects", "samples", "settings"];')
    s=s.replace('<button role="tab" data-tab="settings"','<button role="tab" data-tab="samples">SAMPLES</button><button role="tab" data-tab="settings"',1)
    s=s.replace('<section class="panel" id="p-settings"','<section class="panel" id="p-samples" data-tab="samples" hidden></section><section class="panel" id="p-settings"',1)
    s=s.replace('</head>','<link rel="stylesheet" href="rhythmer.css"></head>')
    s=s.replace('<main>','<main><section id="rhythmer-mode"><a href="../#editor" target="_top" class="rhy-title">RHYTHMER / EDITOR</a><div class="bar"><a id="mode-browser" href="?target=browser">BROWSER WORKSPACE</a><a id="mode-usb" href="?target=usb">USB FM-1</a></div><p id="workspace-status" role="status"></p><p id="mode-note"></p></section>',1)
    codec=Path(os.environ.get("RHYTHMER_BUILD_DIR",root/"rhythmer/build-ssd"))/"workspace.wasm"
    editor_version=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((site/'editor').iterdir()) if p.is_file())+(codec.read_bytes() if codec.exists() else b'')).hexdigest()[:12]
    for entry in ['workspace.mjs','break-samples.mjs']:
        s=s.replace('./'+entry,'./'+entry+'?v='+editor_version)
    s=s.replace('href="rhythmer.css"','href="rhythmer.css?v='+editor_version+'"')
    version=html.escape(release.get('version','development'))
    s=s.replace('class="rhy-title">RHYTHMER / EDITOR</a>',f'class="rhy-title">RHYTHMER / EDITOR</a><span class="rhy-build">{version}</span>')
    (out/'index.html').write_text(s)
    b=(vendor/'fm1backup.js').read_text().replace('length: 32','length: 16')
    # Do not accept foreign ABI backups or allow unknown objects to be skipped during restore.
    b=b.replace('if (!file || file.format !== "felucca-backup" || file.version !== 1 || !ids)','if (!file || file.format !== "felucca-backup" || file.version !== 1 || !ids || ids.length !== 27 || !["Rhythmer dev","Rhythmer v0.1.1-beta3","JIANT Rhythmer dev","JIANT v0.1.1-beta3"].includes(file.firmware))')
    b=b.replace('if (a[2] === 1) { done += o.size; onProgress(done, total); continue; }','if (a[2] === 1) throw new Error("Rhythmer archive object rejected; restore stopped");')
    (out/'fm1backup.js').write_text(b)
    for name in ['fukiai.ttf','FUKIAI-LICENSE.txt','LICENSE.txt','UPSTREAM.json']:
        shutil.copy2(vendor/name,out/name)
    for f in (site/'editor').iterdir():
        if f.is_file():shutil.copy2(f,out/f.name)

    for p in out.glob('*.mjs'):
        text=p.read_text()
        text=re.sub(r"(from\s+['\"])(\./[^'\"]+\.(?:mjs|js))(['\"])",lambda m:m[1]+m[2]+'?v='+editor_version+m[3],text)
        text=text.replace("'./break-convert-worker.mjs'","'./break-convert-worker.mjs?v="+editor_version+"'")
        p.write_text(text)

    codec=Path(os.environ.get("RHYTHMER_BUILD_DIR",root/"rhythmer/build-ssd"))/"workspace.wasm"
    if codec.exists():shutil.copy2(codec,out/"workspace.wasm")
