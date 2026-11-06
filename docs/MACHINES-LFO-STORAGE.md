# Machines, dual LFO and four-project storage

GLO > SYSTEM > TRACK MODE selects 4 TRACK or 6 TRACK. Fresh settings default to six enabled tracks: T1 Drum, T2 Serum, T3 Phase, T4 LoFi, T5 Acid, T6 Break. In 4-track mode T5/T6 are inactive and hidden from ordinary track selection, with their sounds, notes, automation and mute state retained. Mode changes do not rearrange engines or unmute tracks. HOME GEN replaces only the first four tracks (T1 Drum, T2–T4 synths), preserving T5/T6 and the chosen count.

Any track can choose any available musical machine, and category-specific EDIT/SEQ follows the chosen engine. Acid is multi-instance: six independent 472-byte DSP contexts, plus per-track note/gate and generator RNG/bar state; render scratch is separate per core. Break retains one DSP instance and one shared two-loop sample bank. At most one track owns Break, even when that track is muted or disabled by 4-track mode. Its factory/user presets, favorite results and model chooser entry are hidden on other tracks until the owner changes away. Direct preset loads and duplicate-Break project imports are refused as a backstop. Existing JIANT drum tracks share the section's editable kit but retain separate hit/sequence state.

The track count is a device preference. Settings bytes 0x50/0x51 represent 4/6 tracks. Previous 0x40 (both machines off) migrates to four tracks; 0x41–0x43 migrates to six. Unset settings start at six. Project engine validity is checked independently of the current browsing filter, so valid relocated Break projects still load. The CPU-test core-count result uses the formerly unused metadata byte [15][31], separately from track mode [15][15], and legacy 1/2-core results migrate before the track preference is normalized. First-boot settings explicitly initialize 0x51.

Each track has two independent LFOs. Existing LFO / LFO DEST is LFO 1. LFO 1 PHASE retains the original TRIG/FREE phase controls. The new LFO 2 page has RATE, WAVE, PHASE (-1 = FREE) and FADE. Rates include the original 12 tempo divisions and free range; five waveforms include independent sample-and-hold RNG. LFO > MOD: knob 1 picks slot 1–4; knob 2 selects SRC including LFO 1 and LFO 2; knobs 3–4 retain destination and signed amount. All four new parameters and source selections use the existing project/preset/section/motion serialization. Engine parameter IDs are mapped by stored parameter count; old stores default the new parameters. Runtime LFO state is per track and never shared between cores.

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
