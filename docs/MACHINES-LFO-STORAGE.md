# Machines, dual LFO and four-project storage

GLO > SYSTEM > TRACK MODE selects 4 TRACK or 6 TRACK. Fresh settings default to six enabled tracks: T1 Drum, T2 Serum, T3 Phase, T4 LoFi, T5 Acid, T6 Break. In 4-track mode T5/T6 are inactive and hidden from ordinary track selection, with their sounds, notes, automation and mute state retained. Mode changes do not rearrange engines or unmute tracks. HOME GEN replaces only the first four tracks (T1 Drum, T2–T4 synths), preserving T5/T6 and the chosen count.

Any track can choose any available musical machine, and category-specific EDIT/SEQ follows the chosen engine. Acid is multi-instance: six independent 472-byte DSP contexts, plus per-track note/gate and generator RNG/bar state; render scratch is separate per core. Break retains one DSP instance and one shared two-loop sample bank. At most one track owns Break, even when that track is muted or disabled by 4-track mode. Its factory/user presets, favorite results and model chooser entry are hidden on other tracks until the owner changes away. Direct preset loads and duplicate-Break project imports are refused as a backstop. Existing JIANT drum tracks share the section's editable kit but retain separate hit/sequence state.

The track count is a device preference. Settings bytes 0x50/0x51 represent 4/6 tracks. Previous 0x40 (both machines off) migrates to four tracks; 0x41–0x43 migrates to six. Unset settings start at six. Project engine validity is checked independently of the current browsing filter, so valid relocated Break projects still load. The CPU-test core-count result uses the formerly unused metadata byte [15][31], separately from track mode [15][15], and legacy 1/2-core results migrate before the track preference is normalized. First-boot settings explicitly initialize 0x51.

Each track has two independent LFOs, with separate phase, waveform, fade and sample-and-hold RNG. The LFO button cycles MOD → LFO 1 → LFO 2 → MSEQ. Both LFO setting tabs have RATE, WAVE, MODE (TRIG/FREE), FADE and a separate PHASE page reached with SELECT. FREE is stored as phase -1; its remembered TRIG phase is per source and per track. Switching one source to FREE restarts only that source's fade. Both LFO 1 DEST and LFO 2 DEST have independent PITCH, FILTER, SHAPE and AMP depths, in addition to all four MOD slots. SRC display order is OFF, LFO 1, LFO 2, ENV and the remaining sources; stored IDs remain unchanged (LFO2 = 15). MOD knob 1 selects slot 1–4, knob 2 selects source including LFO 1/2, knobs 3–4 select destination and signed amount. The graph and its phase page always display the selected source. RATE supports 12 tempo divisions and the original free range; five waveforms include independent sample-and-hold RNG.

All parameters and source choices retain the existing project/preset/section/motion serialization and parameter IDs. No extra oscillator is created. Nonzero LFO2 direct depths add per-block modulation offsets through the existing modulation renderer; all-zero depths retain its fast path. Project and preset layouts keep their parameter counts, record sizes and flash map. A host test now covers all six tracks, mode/phase/fade isolation, all four source slots, project round trip and repeated-button/SELECT navigation. Actual Break modulation and exact-package dual-LFO workloads are checked separately. Physical-device confirmation of this maintenance candidate remains pending.

Four complete projects, each A–D sections, are available in SAVE > PROJECT. Each section is six tracks plus sounds, sequence, automation and shared state, subject to the original 3840-byte packed-section limit. Each save retains an A/B commit-last sector pair with CRC and sequence selection; interrupted writes leave the last committed copy valid.

Flash map (all offsets chip-relative):

| Region | Purpose |
| --- | --- |
| 97000–9EFFF | Project 1, A–D, dual copies |
| 9F000 / FE000 | User FM6 patches, unchanged |
| A0000–B3FFF | Amen 2-bar PCM12 sample |
| B4000–C7FFF | Think 2-bar PCM12 sample |
| C8000–D7FFF | Projects 2–3, A–D, dual copies |
| D8000–D9FFF | Project index |
| DC000–DFFFF | User presets |
| E0000–E4FFF | Update staging, unchanged |
| E5000–E6FFF | Autosave, unchanged |
| E7000–E7FFF | FM6 voice bank, unchanged |
| F0000–F7FFF | Project 4, A–D, dual copies |
| FC000–FDFFF | Device settings, unchanged |

The fourth project uses an explicitly bounded SIX-only region in the stock firmware's USR/preset area, unused by SIX's factory sounds, user presets, loader and staging. It is not compatible with stock firmware's own patch data in that area; switching firmware requires the normal separate backup/restore workflow. No physical flash has been performed. Current SIX factory sound counts and both break samples are retained. The original three-project index rows/scenes migrate in memory to the fourth-project layout without rewriting sample sectors.

Audio buffering: 512-frame halves, 8192-byte DMA buffer overall (512 frames × stereo 32-bit × two halves), low-buffer allocation. Storage scratch moves into available normal RAM; USB stereo reuses a 2048-frame portion of a shared 20 KiB ring. Both 8- and 10-channel stem modes use 1024 frame power-of-two capacities, fixing index wrap overlap in 10-channel mode. Build keeps the original 8 KiB pool reserve, linker bounds and code reach checks.


LFO 2 DEST compatibility (maintenance candidate 20261107.6): obsolete split-HUMAN slots 65/68 and unused DIGITAL slots 72/80 hold encoded direct depths. PITCH/FILTER/SHAPE store depth +64; AMP stores depth -64 (127 remains the historical inactive default). Factory defaults are unchanged. FUNC writes an optional final `L2` + track-mask extension (3 bytes); user presets use reserved packed byte 142 = 0x4c. Untagged old projects/presets cannot activate these reclaimed values; DIGITAL is converted before its original operator level is discarded. Active tagged tracks bypass only the obsolete split-HUMAN merge; the current unified HUMAN parameter is retained. Direct depth edits and preset loads do not change MOD slots. Four direct destinations keep the original signed ranges and full amplitude resolution. Existing STEP locks/motion IDs remain below 128. The editor advertises INFO tag 0x32/version 1 and uses atomic GET/SET scope 2, IDs 0..3, to avoid partial initialization during four-knob edits. Raw parameter DUMP, sound files and archives retain the encoded values.
