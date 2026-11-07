# Rhythmer

![Rhythmer six-track workspace](docs/launch/card.png)

**Six assignable tracks for the M-VAVE FM-1.** Synthesized drums, nine musical synth models, 303-style acid generation and playable breakbeats, with one sequencer and a browser editor.

Free, open-source firmware by [Jazzy Rhythmer](https://www.youtube.com/@jazzyrhythmer). Optional support: [Ko-fi](https://ko-fi.com/jazzyrhythmer).

**Built on [JIANT](https://github.com/juanjiant-bit/JIANT).** Rhythmer keeps its sequencing, project management and system foundation, with a new monochrome interface and additional machines. The native Acid synth comes from Hortator, paired with X0X-derived TB-3PO MIDI generation; Break manipulation comes from Amenity. Original authors and licenses remain credited.

## Watch Rhythmer

[![Watch the Rhythmer OS opening — Acid Generator and Breakbeat Manipulator](site/media/video/rhythmer-opening-poster.jpg)](https://jazzyrhythmer.github.io/Rhythmer/#opening)

[Watch the 36-second opening with sound](https://jazzyrhythmer.github.io/Rhythmer/#opening) · [Download the Amen mix MP4](https://github.com/JazzyRhythmer/Rhythmer/releases/download/v0.1.0-beta.2/Rhythmer-OS-Opening-36s-1080p-Amen.mp4). Original opening sound plus an Amen Break section; sample credits below. The film shows host-rendered firmware UI, not physical-device footage.

## Start here

[Play](https://jazzyrhythmer.github.io/Rhythmer/#play) · [Detailed manual](https://jazzyrhythmer.github.io/Rhythmer/#guide) · [Web editor](https://jazzyrhythmer.github.io/Rhythmer/editor/) · [Install / official V15 return](https://jazzyrhythmer.github.io/Rhythmer/installer/) · [Releases](https://github.com/JazzyRhythmer/Rhythmer/releases)

**Use official V15 when moving from another custom firmware to Rhythmer, or from Rhythmer to another custom firmware.** Direct foreign-custom transitions are unsupported. For a first install, official V15 is the recommended starting point. Back up the music you want to keep before changing firmware. The installer requires a final V15 recommendation acknowledgement, backup/risk acknowledgements and a separate audio-rights acknowledgement. Use Chrome or Edge and keep USB/power connected until verification finishes. A matching Rhythmer can be updated directly after backup.

## Acid generation and Break performance

| Acid / NEW + MUTATE | Break / chops + repeats |
| --- | --- |
| ![Acid generation](docs/motion/acid-generate.gif) | ![Break performance](docs/motion/break-perform.gif) |
| ACID track → EDIT → ACID tab → 303 GEN 1 / 2. OCT+ generates a new line; OCT− mutates it. SCL supplies the root and scale. [Acid guide](docs/ACID.md) | BREAK track → EDIT → BREAK tab. Play slices and repeat/up/down commands; SEQ edits the actual events. Hold SEQ for STRAIGHT / MUTATE / RANDOM. [Break guide](docs/BREAK.md) |

## The machines in motion

Host-rendered firmware UI driven by note, envelope and preset input; silent previews, not physical-device footage. FM6 cycles patches to show its operator networks. [Try the actual firmware with sound](https://jazzyrhythmer.github.io/Rhythmer/#play).

| | | |
| --- | --- | --- |
| **DRUM**<br><img src="docs/motion/drum.gif" alt="DRUM firmware UI animation" width="240"><br>Synthesized percussion | **SERUM**<br><img src="docs/motion/serum.gif" alt="SERUM firmware UI animation" width="240"><br>Wavetable morphing | **PHASE**<br><img src="docs/motion/phase.gif" alt="PHASE firmware UI animation" width="240"><br>Phase distortion |
| **LOFI**<br><img src="docs/motion/lofi.gif" alt="LOFI firmware UI animation" width="240"><br>Digital textures | **ANALOG**<br><img src="docs/motion/analog.gif" alt="ANALOG firmware UI animation" width="240"><br>Classic waveforms | **FM6**<br><img src="docs/motion/fm6.gif" alt="FM6 firmware UI animation" width="240"><br>Six-operator patch networks |
| **WHEEL**<br><img src="docs/motion/wheel.gif" alt="WHEEL firmware UI animation" width="240"><br>Harmonic organ layers | **FM+STAT**<br><img src="docs/motion/fmstat.gif" alt="FM+STAT firmware UI animation" width="240"><br>Compact FM | **SWAV-SAW**<br><img src="docs/motion/swavsaw.gif" alt="SWAV-SAW firmware UI animation" width="240"><br>Layered saws |
| **SID-6581**<br><img src="docs/motion/sid.gif" alt="SID-6581 firmware UI animation" width="240"><br>Chip-style synthesis | **ACID**<br><img src="docs/motion/acid.gif" alt="ACID firmware UI animation" width="240"><br>303-style bass | **BREAK**<br><img src="docs/motion/break.gif" alt="BREAK firmware UI animation" width="240"><br>Sequenced chops |

## Machines and sequencing

- Six assignable tracks; fresh settings start with four active tracks: DRUM / SERUM / PHASE / LOFI. T5/T6 start EMPTY. Confirm six-track mode with OCT+ to fill empty T5/T6 with ACID / BREAK; existing sounds and notes are kept and a Break on another track is never duplicated.
- Synths: Serum, Phase, LoFi, Analog, FM6, Wheel, FM+STAT, SWAV-SAW and SID-6581. All 41 selected Garden model presets remain; MONO2K-derived models add 17 starting sounds.
- Drums include the complete 55-preset Hortator MD bank (57 articulations / 21 kernels), 19 kits including two experimental MD LAB kits, and individual lane sound replacement. Drum attacks are protected for about 40 ms; tails then use the ordinary shared voice/CPU policy, without a separate MD-only cap.
- Multiple Acid tracks; one Break machine per project, assignable anywhere. Machine-specific EDIT/SEQ pages follow the selected machine.
- Acid NEW/MUTATE always follows the root/scale in SCL; the same shared SCL default is CHR.
- Break: 16 chops, 16 performance/repeat commands, source A/B and return commands in the real sequence. Hold SEQ for STRAIGHT / MUTATE / RANDOM, tail fills, beat shuffle, reverse order, shifts and A/B sequencing. Saved periodic mutation supports ORBIT/DRIFT.
- Break follows the normal track filter, ADSR and both LFOs. Fresh Break tracks select A / Amen with an empty two-bar sequence. Hold SEQ + F3 for a straight loop; saved edits remain intact.
- Four projects, A–D sections, two LFOs per track, modulation slots, shared distortion/phaser/delay/reverb and performance effects. Four-/six-track modes preserve inactive tracks.
- Web editor for actual sounds/patterns, musical archive import/export and user-replaceable two-bar A/B loops. Conversion to 16 kHz mono packed PCM12 happens in the browser. Musical archives do not contain audio: retain your original samples separately.

## Release and compatibility

The current beta is **v0.1.1-beta3 / FM-1_748005**. This maintenance revision keeps the version label; use the installer SHA-256 to identify the exact binary. The package includes its update loader and both prepared A/B banks. Installing restores those factory loops. Project/settings protection applies only to the documented layout and does not make foreign firmware projects compatible.

New in this release: 59 GEN genres; targeted MIDI and genre-aware BREAK GEN; all 55 Hortator MD presets and per-lane drum replacement; four Hortator Acid sounds; revised Think level matching; guided user-sample conversion/upload; empty initial sequences; and a detailed English manual. ALL fills eligible active slots in four-/six-track mode and keeps pre-existing Acid/Break parts. Only the explicitly named SR_ACID recipe assigns native Acid for its mono bass part; ordinary bass, lead and chord pools exclude it, and Break is excluded from automatic assignment. Target MIDI remains available at every track position. Curated genres keep preset tuning, use role-appropriate sounds and leave melodic space. CPU work replaces sample-rate phaser float math, streamlines packed Break reads and removes unused legacy Break processing. The normal voice budget, clocks, synchronization and memory guards remain.

The owner installed build **20261108.6** and reported stable, very good playback without the earlier buffering issue. Build **20261108.7** keeps that audio DSP, buffer, voice management and core scheduling; its firmware change only adds default instruments on confirmed six-track activation. Automated checks and the exact hashes/test scopes are recorded in `RELEASE-VALIDATION.json`. Busy patterns can still exceed hardware capacity: mute tracks, reduce simultaneous notes or simplify effects when necessary. Back up before installation.

Use official V15 between different custom firmware families. A previous unit became unresponsive after power was interrupted during an update; its recovery remains unconfirmed. Cold power-loss recovery and UC25HQ80 hardware validation are not established.

The restored JIANT shared voice/CPU policy and new-engine dual-core scheduling remain. Drum attacks have a short protection window and older tails participate normally; there is no separate MD-only cap. The 512-frame half-buffer restores approximately 11.6 ms of scheduling margin at 44.1 kHz. No guarantee of unlimited polyphony is made.

## Samples, credits and license

Source is GPL-3.0-only; original component licenses and notices remain. The active firmware is in `rhythmer/`. See `docs/THIRD_PARTY.md`, `rhythmer/LICENSING.md` and the component license directories.

Amen: The Winstons, *Amen, Brother*, Gregory C. Coleman on drums; source via Amenity. Think: Lyn Collins, *Think (About It)*; a two-bar drum edit from owner-supplied files. Each prepared loop is two bars at 160 BPM, mono 16 kHz / PCM12. **These recordings are separate from the code license.** The owner directs their inclusion with credits; recording/composition redistribution permission has not been documented. Credits, installation acknowledgements, free access and this repository do not grant a blanket sample license. Replace either loop through the web editor or obtain permission appropriate to your use. See `audio-rights.json` and `THIRD-PARTY-AUDIO.txt`.

Built from community work including Felucca, JIANT, X0X, Amenity, Garden-FM, MONO2K and Hortator. Neither those authors nor M-VAVE or Elektron are claimed to endorse Rhythmer. Machinedrum and other referenced product names belong to their owners.

Independent community firmware, supplied without warranty to the extent permitted by law. Firmware changes can affect data or device operation. Mandatory legal rights and non-excludable liability remain unaffected. Read `LEGAL-NOTICE.txt`.
