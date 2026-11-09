# Changelog

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
