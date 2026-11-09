# Native Acid and engine efficiency — 20261108.6

Version: 0.1.1-beta3. Product identity: FM-1_748005. Private test candidate.
Firmware SHA-256: `9962e1b78d357ce8fbe9b1b88cfccec0ecc2e87c649c8752af684102bdd4ae82`.

## Acid: one native synth, one MIDI generator

The active Acid engine is now `rhythmer/firmware/src/eng_acid.c`, backed directly by Hortator's oscillator, ladder filter, envelope and glide. Four factory sounds are available: HT SQUELCH, HT DEEPBASS, HT SCREAMER and HT SQUAREAC. SAW/SQR are the only waveform choices.

The X0X bass303 audio DSP, floating-point helpers, parameter layer, audio-model union, duplicate note/gate caches and old multi-machine pattern wrapper have been removed from the active firmware and all active build paths. The retained TB-3PO NEW/MUTATE algorithm is a MIDI-only command in `acid_generate.c`. It does not run inside the audio callback. AUTO defaults to OFF; an explicitly enabled interval runs once at the requested bar boundary. The main-loop poll also exits immediately when the bar has not changed.

Hortator pitch increments and fixed 32-frame envelope-decay factors are cached in existing runtime fields. The render loop and coefficient routine use the existing bounded RAM-code section. The cached kernel matches its frozen pre-cache reference bit for bit across 48,000 blocks, including waveform/control extremes, pitch glide and release. This verifies the optimization, not equivalence to the intentionally removed X0X sound.

The Acid generator displays and edits the same ROOT/SCALE as SCL. CHR is the shared default. There is no private minor fallback, second key state or explicit-scale-choice marker. Existing projects retain their stored SCL settings; changing SCL does not rewrite existing MIDI until generation is requested.

## GEN routing

Only the existing, explicitly named `SR_ACID` sound recipe selects native Hortator Acid in ALL LOOP, ALL PARTS or ALL SONG. This recipe already appears in Acid House and Breaks; EXPLORE/mutant recipes can use it when explicitly selected by their existing recipe rules. Acid is not added to ordinary bass, lead, chord or texture sound pools. Break stays excluded from automatic instrument assignment.

A one-byte transaction mask keeps a newly assigned Acid track inside the current generation pipeline, so its MIDI, mix, harmony and A–D sections are completed. The mask is cleared afterward; pre-existing Acid/Break parts remain preserved by ALL. Individual TARGET MIDI on an Acid track uses the genre-driven mono-bass role, preserves its sound/tempo/other tracks, and follows SCL. The dedicated ACID NEW/MUTATE interface remains separate.

Target routing passed 354 genre/track combinations (59 genres × all six positions), with 2,616 single-note events. LOOP/PARTS/SONG assignment and stored sections passed. Forced Acid requests through ordinary bass, chord and lead recipe paths are rejected by the normal sound-pool fence.

## Older projects

Engine ID 16 and the project/wire layout remain unchanged. Old Rhythmer X0X Acid tracks reopen with native Hortator sound. Legacy factory sound indices and wave selectors migrate to the four native presets/two waveforms; note, accent, slide, tie and SCL data remain. Five legacy factory fixtures, square-wave migration and round trips are covered by tests. Unrelated standalone X0X project formats are rejected safely rather than imported as an unknown layout.

The root prototype/reference tree and required attribution notices are preserved. They are not compiled into the active Rhythmer image and consume no device flash/RAM/CPU.

## Other retained efficiency changes

- DMA half-buffer is restored to 512 frames, about 11.6 ms at 44.1 kHz, matching the earlier public release. This restores scheduling margin, not DSP capacity. Total physical round-trip latency was not measured here. Existing governor timing constants retain their original wall-clock durations.
- Mutually exclusive FM6 and MD runtimes reuse typed, per-track storage. Real crossfade/engine-switch tests verify the old renderer finishes before that storage changes owner. Eight drum lanes per track and 48 MD runtime slots remain; stack and guard reserves are not reduced.
- Drum protection is a Rhythmer addition: approximately 40 ms of the new attack is protected from voice stealing/overload shedding. Afterward, tails participate in the original voice-management logic. Same-lane retriggers and hat choke still work. There is no separate MD-only cap.
- Break avoids repeated unchanged-tempo divisions and duplicate independent clock handling. Exact silence skips sample work while clocks, sequence position and required state continue. Pending triggers, live rolls, filter/comb tails and unfinished envelopes prevent an unsafe shortcut. Stop/release then wake-on-trigger is covered by host tests, and standalone idle/wake PCM and trigger timing match the reference.
- Fresh settings use four tracks. T5/T6 are EMPTY, with no allocated instrument voices or instrument rendering. Acid/Break can be placed on any eligible track; neither has a fixed T5/T6 role. Stored songs/settings remain preserved when loading them.

No new predictive audio omission, output fade, time-stretch, automatic FX degradation or aggressive adaptive-polyphony policy is introduced. The restored JIANT governor and its original fallback remain, together with the new-engine core scheduling and the limited drum-attack exception.

## Capacity

Compared with the frozen 20261108.5 candidate:

| Measure | 20261108.5 | 20261108.6 |
|---|---:|---:|
| Application bytes | 583,600 | 569,700 |
| Data + BSS bytes | 96,584 | 93,992 |
| Free application slot bytes | 736 | 14,636 |
| Free main data region bytes | 1,720 | 4,312 |
| RAM code bytes | 23,504 | 23,104 |
| Pool/stack guard spare bytes | 8,192 | 8,192 |

This frees 13,900 application bytes and 2,592 data bytes. The fixed-layout FWSC download is still 828,061 bytes. Static capacity alone does not reduce CPU; the omitted runtime work and cached calculations do.

## Performance evidence and limits

Actual candidate FWSC, community emulator 0.4.0, strict mode, 320 MHz, two cores, 24 emulated seconds:

| Fixture | Previous reference | Native Acid candidate |
|---|---|---|
| Five Acid + Break + 44.1 kHz USB | 20261108.5: max half 5,341 µs; final CPU window 41.4% | max half 5,301 µs; final CPU window 29.7% |
| Captured six-track arrangement (no Acid) | Public 20261108.2: max half 6,446 µs; final CPU window 46.5% | max half 6,258 µs; final CPU window 45.7% |

Both candidate runs had zero audio deadline misses, lost buffers and core loss; the five-Acid USB run also reported no USB underruns. CPU percentages are final firmware measurement windows, not whole-run averages. These are emulator comparisons, not promised real-device percentages. The native Acid sound is intentionally different from X0X, so equal MIDI does not imply equal waveform.

A fresh actual-binary boot verified both embedded banks (mask 3), default engines DRUM/WAVETABLE/PHASE/LOFI/EMPTY/EMPTY, four-track mode, two cores and silent empty sequences. Corrected Think and Amen bank hashes remain unchanged. Browser installation simulation passed 49 checks without accessing a physical FM-1.

Host regression, exact-generator, editor, uploader and manual results are recorded in the hash-bound candidate validation report. The user is testing this exact candidate separately. No automatic physical firmware write or public deployment was performed in this work.
