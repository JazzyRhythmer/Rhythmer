#!/usr/bin/env python3
"""Package marketing drafts without deploying firmware or publishing posts."""
from pathlib import Path
import hashlib,json,shutil,zipfile,html
R=Path(__file__).resolve().parents[2];O=R/'outputs/Rhythmer-update-2026-11-07-v2';Q=R/'qa/update-media-20261107'
COPY={
'Reddit.txt':'''TITLE
Rhythmer update preview: individual drum sounds, target GEN, genre-aware break chops, and your own samples on the FM-1

POST
I've been working on the next Rhythmer update. Rhythmer is my hands-on custom FM-1 firmware, built on JIANT, bringing together the synths, drums, acid generation and break manipulation I wanted in one place.

This time, the focus is on changing one part of a groove without losing the parts that already work.

• Swap one drum, keep the kit. Pick a lane in EDIT > SOUND > SELECT, then browse the sound list and replace just that sound. The update includes all 55 Hortator MD sounds, 19 kits and two experimental MD kits within that total.

• More directions for GEN. There are now 59 styles, including ten new profiles: Bounce House, 90s House, G-Funk, Nocturnal HH, Trip Hop, LoFi House, Abstract IDM, Ambient IDM, Atmos Breaks and Drunken HH. Existing R&B, Neo Soul, Jazzy Chords, Boom Bap, Jungle, Liquid DnB, UK Garage and Future Bass profiles have also been refined.

• Generate only the track you want. Choose T1–T6 MIDI as the GEN target. It uses your current genre choice while keeping your sounds, tempo, scale, effects, automation and the other tracks. Press generate again for another variation.

• Generate the break chops. Select BREAK as the target and GEN finds your Break track, wherever you put it. Hip-hop gets space and dynamics, Drunken HH gets intentional push/pull timing, and Jungle gets more active chops and bounded answer rolls.

• Bring your own breaks. The web editor can prepare your own two-bar loops: choose audio, enter the source BPM, select eight beats, convert, compare the original crop with the actual device-format preview, then confirm and save to an A/B slot. The two audio slots are shared across projects. This is classic repitch, so export at 160 BPM first if you want to keep the original pitch.

There are also four Hortator Acid presets alongside the original X0X sound, corrected closed hats in conventional kits, a louder level-matched Think break, and an empty starting sequence.

This is a preview of the next development beta, not an announcement that this exact build is already public. Host/emulator checks are complete; physical FM-1 validation of this candidate is still pending. Dense combinations can still push the hardware, so CPU headroom matters.

Project / current public version:
https://jazzyrhythmer.github.io/Rhythmer/
Source and upstream credits:
https://github.com/JazzyRhythmer/Rhythmer

Which would you try first: a custom drum kit, a new genre, or your own break?

Video note: actual host-rendered Rhythmer UI and a local web-editor capture, with an original atmospheric breakbeat score. The soundtrack is not a direct FM-1 audio demo.
''',
'Instagram.txt':'''RHYTHMER — THE NEXT UPDATE
Build your kit. Keep your groove.

01 / Swap one drum sound without replacing the whole kit.
02 / Explore 59 GEN styles, including ten new profiles.
03 / Generate MIDI for one track. Keep the rest.
04 / Generate genre-aware Break chops — from loose hip-hop to rolling jungle.
05 / Bring your own breaks through the web editor.

Your loop → source BPM → two bars → convert → preview → A/B slot.

Also in this update: all 55 Hortator MD sounds, 19 kits including two experimental MD kits, four additional Hortator Acid presets, corrected closed hats and a level-matched Think break.

Development-beta preview. Exact-build physical FM-1 testing is still pending; dense patches still need CPU headroom. This preview is not yet the public installer build.

By Jazzy Rhythmer. Built on JIANT, with credit to the upstream projects that made this possible.

Project link in bio:
jazzyrhythmer.github.io/Rhythmer

Original score / actual host-rendered UI. The music is not a direct hardware sound demo.

#Rhythmer #FM1 #Groovebox #Breakbeat #GenerativeMusic #Jungle #HardwareMusic
''',
'YouTube.txt':'''TITLE
Rhythmer: Build Your Kit, Generate Your Next Groove | FM-1 Update Preview

DESCRIPTION
The next Rhythmer update starts with your drum kit — change one sound, keep the rest — then opens up new ways to generate and reshape your music.

Rhythmer is a hands-on groovebox firmware for the M-VAVE FM-1, built on JIANT by Jazzy Rhythmer.

00:00 Rhythmer v0.1.1-beta3 / firmware update intro
00:04 Individual drum sound selection
00:14 Ten new GEN genres / 59 styles in total
00:24 Target GEN / regenerate one track's MIDI
00:34 Break GEN / genre-aware chop sequencing
00:44 MD sounds, Acid and performance
00:52 Bring your own breaks / web-editor upload
01:04 Explore Rhythmer

WHAT IS NEW
• Individual lane sound selection, alongside all 55 Hortator MD sounds and 19 kits (including two experimental kits).
• Ten new GEN profiles: Bounce House, 90s House, G-Funk, Nocturnal HH, Trip Hop, LoFi House, Abstract IDM, Ambient IDM, Atmos Breaks and Drunken HH.
• A T1–T6 MIDI target that preserves tempo, sound, scale, effects, automation and other tracks.
• A BREAK target that finds the Break machine and makes genre-aware chop sequences.
• Your own two-bar loops in the web editor: source BPM, crop, conversion, original/device-format previews and confirmation before saving.
• Four Hortator Acid presets alongside the original X0X sound, corrected closed hats, level-matched Think and empty starting sequences.

Project / current public version:
https://jazzyrhythmer.github.io/Rhythmer/
Source / credits:
https://github.com/JazzyRhythmer/Rhythmer
Creator:
https://jazzyrhythmer.github.io/
Support:
https://ko-fi.com/jazzyrhythmer

STATUS
This video previews an unreleased development-beta candidate. Host/emulator checks have passed; physical FM-1 installation, listening and recovery checks for this exact candidate are still pending. The public installer may contain an earlier version. Dense patches can still exceed the hardware's CPU budget.

SAMPLE UPLOAD NOTE
Use audio you have permission to use. Choose a clean, correctly aligned two-bar 4/4 loop and enter its known source BPM. The editor produces 160 BPM / 3 seconds / mono / 16 kHz / packed 12-bit audio. Classic repitch changes pitch; pre-export at 160 BPM to preserve pitch. The A/B audio slots are global, shared by all projects. Musical backups exclude audio; SAVE FOR PLAY includes both samples.

VISUALS / MUSIC
Actual host-rendered firmware UI, plus a real local web-editor capture. These are not physical FM-1 camera recordings. Original atmospheric breakbeat score and sound design: Rhythmer / Afterimage. No third-party music, artist MIDI, Amen or Think recordings are used in this video's soundtrack. The music is not a direct firmware audio demonstration.

Independent project; not affiliated with or endorsed by M-VAVE. Back up your music, follow the installation guide and use the official V15 route when switching firmware. Keep power and USB connected throughout an update.

#Rhythmer #FM1 #Breakbeat

PINNED COMMENT
This is a preview of the next development beta, not a claim that this exact version is already in the public installer. Which new workflow should get a hands-on tutorial first: individual drums, target GEN, Break GEN or sample uploads?
''',
'Reels.txt':'''ON-SCREEN ORDER / 52 SECONDS
00–04  RHYTHMER v0.1.1-beta3 / FIRMWARE UPDATE
04–12  CHANGE ONE SOUND. KEEP THE KIT.
12–20  NEW GENRES. NEW DIRECTIONS. / TEN NEW GENRE NAMES
20–28  ONE TRACK. NEW POSSIBILITIES.
28–36  GENERATE THE BREAKS.
36–48  BRING YOUR OWN BREAKS.
48–52  RHYTHMER / YOUR NEXT GROOVE.

CAPTION
Swap one drum. Generate one track. Find a new groove.

The next Rhythmer update adds new GEN styles, target MIDI generation, genre-aware Break chops and your own two-bar samples through the web editor.

Development-beta preview / exact-build hardware testing pending.
Project link in bio. Built on JIANT.

#Rhythmer #FM1 #Groovebox #Breakbeat #GenerativeMusic
''',
'Release-checklist.txt':'''PUBLICATION STATUS
All files in this pack are PREVIEW drafts. No post or video was published by this workflow.
Candidate: 0.1.1-beta3 / FM-1_748005.
SHA-256: 9c6731ebbb19c0d83c427664f1a00c8dd149ce005b86b7ef5fa959798a2725ec
Exact candidate publicly deployed: false. Physical hardware validation: false.

BEFORE CHANGING “PREVIEW” TO A RELEASE ANNOUNCEMENT
1. Confirm installation from official V15, audio playback at realistic load, sample upload/readback and V15 return on the exact candidate.
2. Rebuild with the source UI navigation fix in QA-NOTES.txt and validate the new binary. The prior candidate package was preserved.
3. Publish the checked candidate and confirm the installer serves that hash.
4. Regenerate artwork/video labels and revise the status paragraphs. Do not only change the social caption while leaving preview cards inconsistent.
5. Use final published version and precise hardware results; do not claim all overloads are solved.

Suggested upload order: YouTube long video first; use its final URL in Reddit; Instagram carousel and Reels last with the project link in bio. This file is guidance only, not permission to publish.
'''
}
def package():
 copies=O/'Post-Copy';copies.mkdir(exist_ok=True)
 for name,content in COPY.items():(copies/name).write_text(content)
 source=O/'source';source.mkdir(exist_ok=True)
 for name in ['render_update_media.py','score_update_media.py','capture_update_media.c','package_update_media.py','prepare_update_intro.py']:shutil.copy2(R/'tools/six'/name,source/name)
 shutil.copy2(R/'rhythmer/tests/six_gen_navigation_test.c',source/'six_gen_navigation_test.c')
 shutil.copy2(Q/'gen-navigation.log',source/'gen-navigation.log')
 assets=source/'assets';assets.mkdir(exist_ok=True)
 for p in (Q/'native').glob('*.webp'):shutil.copy2(p,assets/p.name)
 for p in (R/'site/media/motion').glob('*.webp'):shutil.copy2(p,assets/p.name)
 shutil.copy2(Q/'editor-upload.jpg',assets/'editor-upload.jpg')
 shutil.copy2(R/'qa/editor-gen-20261107/project-bank.png',assets/'project-bank.png')
 for name in ['InterTight[wght].ttf','IBMPlexMono-SemiBold.ttf','OFL.txt','OFL-IBMPlexMono.txt']:shutil.copy2(R/'rhythmer/assets/fonts'/name,assets/name)
 shutil.copy2(R/'rhythmer/LICENSE',source/'LICENSE-UI-GPL3.txt')
 (O/'QA-NOTES.txt').write_text('''VISUAL / STATUS NOTES
Screen exports are integer nearest-neighbour enlargements of the actual 240x240 firmware UI; higher output dimensions do not add hardware detail.
Portrait video is reflowed with complete LCD panels. Web-editor shots use intentional close-ups of the actual crop/conversion/preview controls.
The uploader screenshot uses a newly composed original 135 BPM two-bar drum loop, converted locally to 160 BPM. No hardware write was made for this capture.
The background score is original, not a demo of these presets or a recording of an existing artist.

CAPTURE FINDING / SOURCE FIXED, NEW BINARY PENDING
Entering GEN while the remembered underlying page was DRUM VOICE could show the drum sound list in GEN's graph area. six_graph_surface() now prioritizes GEN and HOME over the remembered EDIT page. The new ASan/UBSan navigation regression passes for GEN, HOME and the original Drum sound list. It was added to the normal test script. Media captures show the actual intended GEN UI. The existing validated candidate binary/package was preserved: rebuild with this source fix and revalidate the new binary before a release or installation. No new physical verification is claimed.
''')
 (O/'README.txt').write_text('''RHYTHMER / NEXT UPDATE MEDIA PACK

Start with index.html for the gallery.
YouTube: Rhythmer-Update-YouTube-1920x1080-68s.mp4
Reels: Rhythmer-Update-Reels-1080x1920-52s.mp4
Thumbnail upload: YouTube-Thumbnail-3840x2160-upload.jpg (1280x720)
Instagram carousel: Instagram-01 through 06 *-upload.jpg (1080x1350).
Post-Copy contains ready-to-edit English Reddit, Instagram, YouTube and Reels drafts.
Screens contains 18 actual firmware LCD views exported at 1920x1920.
Major-Screens-3200px.png is the large contact sheet. Storyboards show the timelines.
PNG/JPG poster masters: 3840x2160 and 2160x2700.

Priority: 4-second version/update intro > individual drum sound replacement > ten named new genres > target GEN > Break GEN > bring your own Breaks. The original score PCM is preserved after the new intro. Landscape is 68 seconds; portrait is 52 seconds.
Real host-rendered firmware UI and a real local web-editor screenshot; no device photo or invented interface. Original 135 BPM atmospheric breakbeat, sub, suspended-ninth pads, dub echoes and gentle UI SFX. No existing artist recordings or MIDI. Music is separate from the illustrated firmware audio.

Status: unreleased development-beta preview. Exact candidate hardware verification remains pending. No firmware deployment or social publication was performed. Read Release-checklist.txt and QA-NOTES.txt before a public release announcement.

Editable Python/C source and source assets are provided. The render source currently uses the fm1-six repository paths and existing SSD toolchain; it is not a standalone app. UI capture code retains GPL-3.0 and upstream font notices. Rhythmer is built on JIANT, with upstream acknowledgements retained in the project.
''')
 imgs=sorted(p for p in O.glob('*-upload.jpg'))
 sections=''.join(f'<figure><a href="{html.escape(p.name)}"><img loading="lazy" src="{html.escape(p.name)}"></a><figcaption>{html.escape(p.stem)}</figcaption></figure>' for p in imgs)
 vids=''.join(f'<section><h2>{label}</h2><video controls preload="metadata" poster="{poster}" src="{name}"></video><p><a download href="{name}">Download MP4 ↓</a></p></section>' for label,poster,name in [('YouTube / 68 seconds','Frame-YouTube-02s.png','Rhythmer-Update-YouTube-1920x1080-68s.mp4'),('Reels / 52 seconds','Frame-Reels-02s.png','Rhythmer-Update-Reels-1080x1920-52s.mp4')])
 (O/'index.html').write_text(f'''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Rhythmer / Update Media</title><style>body{{background:#0a0c0c;color:#eeeede;font-family:Arial,sans-serif;margin:4vw auto;max-width:1400px;padding:24px}}h1{{font-size:clamp(40px,8vw,100px);letter-spacing:-.04em}}p{{line-height:1.6;color:#afb8af}}a{{color:inherit}}.grid{{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:28px}}figure{{margin:0}}img{{width:100%;height:auto}}figcaption{{font:12px monospace;padding:12px 0}}video{{max-height:80vh;max-width:100%;background:black}}section{{margin:65px 0;border-top:1px solid #373d37;padding-top:20px}}</style><p>R / OS · JAZZY RHYTHMER · BUILT ON JIANT</p><h1>BUILD YOUR KIT.<br>KEEP YOUR GROOVE.</h1><p>Update preview. Individual drums first, then new genres, target GEN, Break GEN and your own samples.<br>Development beta / exact-build hardware validation pending. No new public release was made by this media task.</p><p><a download href="Rhythmer-Update-Media-Pack.zip">FULL MEDIA PACK ↓</a> · <a download href="Rhythmer-Update-Images.zip">IMAGES ONLY ↓</a></p>{vids}<h2>Posters / Instagram carousel</h2><div class="grid">{sections}</div><section><h2>Actual UI / 18 views</h2><a href="Major-Screens-3200px.png"><img src="Major-Screens-3200px.jpg"></a></section><section><h2>English post drafts</h2>{''.join(f'<p><a href="Post-Copy/{name}">{name}</a></p>' for name in COPY)}<p><a href="Rhythmer-Update-Score-With-Intro.wav">Original score / WAV</a> · <a href="README.txt">Read me</a> · <a href="QA-NOTES.txt">QA and release notes</a></p></section></html>''')
 with zipfile.ZipFile(O/'Rhythmer-Update-Images.zip','w',zipfile.ZIP_DEFLATED,compresslevel=5) as z:
  for p in sorted(O.rglob('*')):
   if p.is_file() and p.suffix.lower() in ('.png','.jpg') and 'source' not in p.relative_to(O).parts:z.write(p,p.relative_to(O))
 records=[{'file':str(p.relative_to(O)),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(O.rglob('*')) if p.is_file() and p.suffix!='.zip' and p.name!='manifest.json']
 (O/'manifest.json').write_text(json.dumps({'releaseStatus':'unreleased development preview','published':False,'physicalValidation':False,'files':records},indent=2)+'\n')
 z=O/'Rhythmer-Update-Media-Pack.zip'
 with zipfile.ZipFile(z,'w',zipfile.ZIP_DEFLATED,compresslevel=5) as f:
  for p in sorted(O.rglob('*')):
   if p.is_file() and p!=z:f.write(p,p.relative_to(O))
 print(z, z.stat().st_size,'bytes',flush=True)
if __name__=='__main__':package()
