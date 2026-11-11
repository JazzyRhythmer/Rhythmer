# Rhythmer GEN / target MIDI generation

HOME + turn SELECT opens GEN. On GEN page 1, knob 1 chooses the genre; knob 4 is now TARGET:

- ALL LOOP / ALL PARTS / ALL SONG: the existing complete arrangement generator for tracks 1–4.
- T1 through T6: replace only that track's sequence using the currently selected GEN genre; a Break target reads Tn BREAK.
- The separate BREAK target is removed. Select the track directly. If it already uses Break, GEN shows its Break controls; an EMPTY target asks SELECT SOUND.

OCT+ generates. Pressing it again with the same settings chooses another seed. SELECT still moves through the three GEN pages. GLO + G4 undoes a generation. In a partial target mode, reopening GEN retains the selected genre and target. New Project resets the scope to ALL.

## What partial generation preserves

Global tempo, swing, effects, transport, project/section/arrangement state, all other tracks, and the selected track's sound/engine/preset, root/scale, MIDI routing, loop length, division, offset, gate, FX, envelopes, both LFOs and automation remain unchanged. Generation is computed in a temporary track on the UI stack; only the 64 MIDI steps are committed in a short interrupt guard. No new audio engine, persistent audio buffer, voice allocation or per-sample generation work is added. The MIDI includes rests/ties, velocity, microtiming, accents and supported sequence modifiers.

The selected genre always comes from GEN's current setting, including RANDOM/EXPLORE. It is not taken from a stale last-song cache. Role selection reads the existing notes/register/chords and preset tag. Acid uses a bass role; Drum uses hit lanes; Break uses command notes. Final pitched notes follow the existing SCL mask. A separate polyphonic track can guide chord tones on strong notes; the reference uses the nominal track divisions and stored notes. Complex playhead transforms, transposition automation and unrelated polymetric progressions are not inferred as a full harmonic-analysis system.

The engine remains the one the user chose. Generating notes for an expensive manually chosen synth does not make its DSP inexpensive. Four-track mode also stays four-track mode: preparing MIDI on an inactive track does not activate it.

## Genre-aware Break chops

The generator uses the selected genre's drum structure, dynamics and timing to produce valid C1–C16 chop commands. This is rhythm sequencing; it does not retune the sample to SCL. Audio source A/B and the Break machine's sound/performance parameters are kept. There is no automatic A/B switch or destructive sample replacement.

- Hip-hop, Boom Bap, R&B, Soul and G-Funk: downbeat/secondary kick chops, backbeat snare chops, restrained hat/ghost events and intentional gaps. High-rate roll commands are excluded from these profiles.
- DRUNKEN HH: the established Dilla-inspired timing cells give secondary kicks a different push/pull from the delayed main snares and alternating hats. Dynamics remain varied across a repeatable two-bar cell; HUMAN scales the offsets. At 120 BPM or faster, hip-hop Break sequencing uses a half-time event grid and scaled microtiming, while preserving the master tempo and current loop length. For a two-bar slow phrase at double tempo, a 64-step loop can be used.
- Jungle / DnB / Breaks: stronger chop activity, ghost-snare answers and bounded phrase-end 1/16 rolls; high complexity allows short 1/32/down/up answers. Each roll carries a real slice first so a restart does not depend on an old playhead. Subsequent chops release the previous roll.
- ATMOS BREAKS and UK Garage retain their own rolling/two-step placement with less aggressive roll-bank use.

The sixteen source chops are evenly spaced eighth notes across two 4/4 bars. Kick/backbeat candidates assume a clean, correctly aligned loop; user uploads with different drum placement may need manual chop edits. No automatic source separation or semantic transient detection is claimed.

## Validation

The native target test applies all 59 genres to all six track slots (354 cases), checks other-track/global/automation preservation, current-genre response, SCL, chord reference, 1–64 step lengths, track-specific Break routing and empty-target behaviour, Dilla push/pull and high-tempo half-time. Whole-arrangement GEN regression remains separate. The exact firmware is also tested in the community emulator, including a playing control run and a run that operates HOME + SELECT, the genre/target knobs and OCT+; RAM comparisons verify the actual Break MIDI change and unchanged musical globals/other tracks.

Physical FM-1 listening, update transfer/recovery and long dense-session load tests are still separate checks. Research and artist references are in GEN-STYLE-DESIGN.md; these are original generative profiles, not copied artist MIDI or recordings.

During update-media capture, a remembered DRUM VOICE page was found to override the graph area of GEN/HOME. The source now prioritizes GEN/HOME before the EDIT sound list. `six_gen_navigation_test.c` verifies all three views with ASan/UBSan and is included in the normal test script. This navigation fix is newer than the preserved beta3 binary/package above: it needs a new build and exact-binary validation before installation or release.

See [independent Break tracks](MULTI-BREAK.md) for shared sample banks, per-track state and GEN controls.
