## Multi-Break · build 20261111.3

Assign Break to any active track, including several tracks at once. A/B sample audio is shared; each track independently keeps source selection, chops, playback, pitch, filter, envelope, LFOs, repeats, freeze and feedback.

GEN targets T1–T6 directly. Select a track that already uses Break to see Tn BREAK and its Break-specific controls. The separate BREAK target and automatic owner search are removed. Target generation changes only that track’s MIDI/chops, keeping its sound, source, tempo, effects and other tracks. ALL keeps manually assigned Acid/Break tracks. Four-track mode still plays T1–T4; enable six-track mode for T5/T6.

Both factory Break banks remain included. Replacing A/B changes that shared audio across all projects. Keep original audio and .slot banks; musical backups do not extract device sample audio. Older firmware can reject projects containing multiple Break tracks, so keep an older compatible backup for rollback.

No engines, presets, FX or USB Host features were removed. Per-track state reuses unused synth storage; feedback lines use lossless 24-bit storage; mirrored USB FIR coefficients are shared. Native isolation/project/GEN tests pass. One/two-core PCM matches for 2/4/6 Break tracks plus chorus, delay and reverb. Exact-package emulator tests show no late/audio-lost/core errors; physical-device endurance testing remains separate.


# Changelog

## 2026-11-11 · Sidechain guide

- Documented trigger sources, drum-lane selection, shared DUCK/AMT, and bidirectional Kick Link between RHYTHMER and JAZZY. [Read the guide](https://jazzyrhythmer.github.io/Rhythmer/#guide/sidechain).

## 0.1.2-dev1 — 2026-11-09 · build 20261109.18

- FMINI-1 (43 presets) and OberVave (32 presets) replace PHASE, LOFI and WHEEL. Analog, Wavetable, FM6 and all three MONO2K machines remain. Projects using a retired engine load the affected track with Analog.
- Twelve LXR-X kits use Rhythmer's Drum-X synthesis, alongside the full Hortator MD bank. Browse and swap individual lane sounds; factory kick detection follows the selected sound.
- Five generators in SEQ for drums, synths and Break chops. Full GEN has more role-specific sound choices, including FMINI basses where they suit the genre.
- EDIT / SOUND / PRESETS scopes browsing to the selected engine; the physical PRESETS knob keeps global browsing outside that page. SAVE shows SONG, PROJECT, SOUND, with confirmed project and section clearing.
- USB MIDI Host and duck envelope controls: choose local, USB-audio or MIDI triggers and set attack, release and amount. MIDI velocity does not change the amount.
- Opened the Acid factory cutoff to 80 for Squelch, Deepbass and SquareAc, and set Screamer to 50. Audited factory synth patches for audibility.
- Updated the installer, exact-image browser emulator, web editor, engine animations and English manual.

## Maintenance build 20261108.2 — 0.1.1-beta3

- Restore public 20261107.2 audio rendering, FX and voice governor; remove rejected experimental output protection.
- Start in four-track mode, preserving inactive T5/T6 music. Six-track mode needs an explicit OCT+ CPU-notice confirmation; OCT− cancels.
- Independent LFO2 destinations and adjacent LFO sources; retained GEN bass selection and level improvements.
- Publish matching web editor, 16-section manual and illustrated track-mode instructions.
- Host, emulator, editor and loader checks passed; the owner confirmed physical-device listening.

## 0.1.0-beta.2 - 2026-11-07

- Fix SEQ piano-roll register following and stopped-view redraws. Current notes remain visible when changing tracks/bars or editing pitches.
- Publish a 36-second Rhythmer OS film with all 12 model UI animations, simultaneous Acid/Break pages and an Amen section.
- Keep both embedded Break banks, DSP, project format and update loader unchanged.
- Distinguish this candidate’s host/emulator checks from the earlier beta.1 owner hardware report.

## 0.1.0-beta.1

Initial public beta. See its release notes and exact-package hardware report on the v0.1.0-beta.1 tag.