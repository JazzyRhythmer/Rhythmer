## Multi-Break · build 20261111.3

Assign Break to any active track, including several tracks at once. A/B sample audio is shared; each track independently keeps source selection, chops, playback, pitch, filter, envelope, LFOs, repeats, freeze and feedback.

GEN targets T1–T6 directly. Select a track that already uses Break to see Tn BREAK and its Break-specific controls. The separate BREAK target and automatic owner search are removed. Target generation changes only that track’s MIDI/chops, keeping its sound, source, tempo, effects and other tracks. ALL keeps manually assigned Acid/Break tracks. Four-track mode still plays T1–T4; enable six-track mode for T5/T6.

Both factory Break banks remain included. Replacing A/B changes that shared audio across all projects. Keep original audio and .slot banks; musical backups do not extract device sample audio. Older firmware can reject projects containing multiple Break tracks, so keep an older compatible backup for rollback.

No engines, presets, FX or USB Host features were removed. Per-track state reuses unused synth storage; feedback lines use lossless 24-bit storage; mirrored USB FIR coefficients are shared. Native isolation/project/GEN tests pass. One/two-core PCM matches for 2/4/6 Break tracks plus chorus, delay and reverb. Exact-package emulator tests show no late/audio-lost/core errors; physical-device endurance testing remains separate.
