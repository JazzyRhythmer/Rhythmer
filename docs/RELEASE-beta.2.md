# Rhythmer 0.1.0-beta.2 — optimized beta

Built on JIANT’s sequencing and system foundation, with X0X-derived Acid generation, Amenity-derived break manipulation and a new monochrome interface.

[Website / browser demo](https://jazzyrhythmer.github.io/Rhythmer/) · [Install / official V15 return](https://jazzyrhythmer.github.io/Rhythmer/installer/) · [36-second opening film](https://jazzyrhythmer.github.io/Rhythmer/#opening)

**This optimized binary has not yet been physically revalidated.** A previous build produced audible artifacts and exceeded the audio deadline on a real FM-1. The changes below passed host and emulator checks; resolution of that hardware report is not yet confirmed. This is a public prerelease, released at the owner’s request.

## Changes

- GEN keeps SERUM, PHASE, LOFI, ANALOG, WHEEL and SID-6581. It automatically assigns at most one SERUM, including any existing SERUM on untouched tracks. SWAV-SAW, FM+STAT and FM6 are excluded from automatic GEN assignment. Manual model selection remains available; existing songs are not rewritten.
- Sustained high CPU or recurring missed audio deadlines produces a short `CPU HIGH: REDUCE INSTRUMENTS` notice. It does not interrupt transport, open a modal, or overwrite higher-priority messages. Debounce and a 30-second cooldown limit interruptions.
- Break playback avoids unnecessary remainder calculations and bypasses its performance filter when fully open. The track filter, ENV and both LFO routes remain active. Re-enabling the performance filter fades into fresh filter state.
- SWAV-SAW high-pass arithmetic uses an equivalent cheaper integer calculation. Focused comparisons preserve the original result.
- SEQ piano-roll register follows the current notes after track/bar changes and note edits.
- New browser/editor sessions have empty T1–T5 and only T6’s two-bar Amen A sequence. Existing saves are kept. Export a backup before choosing `RESET / BREAK ONLY` to remove an old demo session.
- The homepage includes all twelve machine animations, the opening film and native startup artwork.

The normal twelve shared-voice ceiling and existing overload relief are unchanged. No additional global voice reduction was introduced. Both prepared two-bar samples are included: Amen A and the corrected drum-only Think B. Project storage, sample banks and update-loader binary remain unchanged.

## Validation scope

Ten current native firmware emulator scenarios passed, including six-track playback, multiple Acid machines, Acid/Break navigation and Break-only website startup. These observed no late/lost audio blocks, worker loss or USB under/over/missed packets. GEN, notice timing/priority, all Garden/MONO2K presets, SEQ visibility and Break filter/ENV/dual-LFO host checks passed. 129,536 integer comparisons and an OPEN-filter PCM comparison passed. Website model/package checks (130) and six firmware-cache regression cases passed.

The captured song’s native emulator maximum changed only from 9,930 to 9,880 µs. This is not evidence of a comparable real-device CPU improvement. Physical CPU and audible-artifact revalidation remain pending. See `RELEASE-VALIDATION.json` and `HARDWARE-TEST-STATUS.json`; the previous owner hardware result applies to beta.1, not this binary.

Product: `FM-1_748001`.
Firmware SHA-256: `976d7cc2d3698a458424bdd83b0eb070ce8ce628a9059b41386ad0dd64ad97a6`.

## Installation and credits

Back up music and keep uninterrupted power/USB until verification finishes. Official V15 is the recommended starting point for a first installation. Use V15 between Rhythmer and another custom firmware; direct foreign-custom transitions are unsupported. A matching Rhythmer can update directly after backup. Cold power-loss recovery is not established; a separately interrupted unit remains unrecovered.

GPL-3.0-only software with retained component notices. Thanks to Felucca, JIANT, X0X, Amenity, Garden-FM, MONO2K and Hortator. Independent of M-VAVE and the upstream authors.

Amen: The Winstons, “Amen, Brother”, drums by Gregory C. Coleman, via Amenity. Think: Lyn Collins, “Think (About It)”, owner-supplied drum intervals assembled into two bars. The owner directs their credited inclusion; recording/composition redistribution permission is not documented. Recordings are separate from the code license and can be replaced in the editor. See `THIRD-PARTY-AUDIO.txt`, `audio-rights.json` and `LEGAL-NOTICE.txt`.

The attached 36-second 1080p opening shows host-rendered firmware UI with an edited soundtrack, not physical-device audio.
