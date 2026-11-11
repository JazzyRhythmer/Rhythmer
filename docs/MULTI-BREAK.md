# Independent Break tracks

Assign BREAK to any of the six tracks. There is no single-Break restriction. Four-track mode still plays T1–T4; enable six-track mode for T5/T6.

Only the two source sample banks A/B are shared. Each Break track has its own source selection, chop sequence, playback/grain positions, pitch, filter, envelope, LFOs, repeats, freeze, feedback line and automatic mutation settings. An A/B command changes only the track carrying that command. Uploading/replacing the actual bank changes that shared audio for every track using it.

## GEN

HOME + SELECT opens GEN. Knob 4 chooses ALL LOOP / ALL PARTS / ALL SONG, then T1 through T6. The separate BREAK target and automatic owner search are removed. GEN never assigns a Break engine when targeting one track.

The selected track determines its UI. For a Break track the target reads Tn BREAK:

- GEN 1: GENRE, SEED, ENERGY, TARGET.
- GEN 2: EVERY, ACTION, MODE, AMOUNT. These are that target track's saved periodic mutation controls.
- GEN 3: DENSITY, REPEATS, SWING, HUMAN. These shape the generated chop sequence.

OCT+ replaces only the chosen track's MIDI/chop sequence, retaining its engine, preset, source, sound parameters, loop length and routing, and preserving other tracks, tempo and global effects. An empty target asks SELECT SOUND. ALL keeps manually assigned Break/Acid tracks; target those tracks explicitly to regenerate them. Preparing an inactive track does not enable it.

The project/storage format and engine IDs are unchanged. Multiple Break tracks can be saved, restored, imported and exported. Older firmware that enforces one Break may reject a project containing several; retain a backup for rollback.

## Verification

Native tests exercise six independent contexts and delay lines, shared bank pointers, cross-track source/filter/pitch/repeat isolation, release, project round trip and targeted GEN. Two/four/six Break tracks with chorus, delay and reverb produce identical PCM using one or two mixer cores. A 512,000-frame single-Break comparison across pitch/filter/feedback/roll/A-B changes is bit-identical to the previous renderer. USB resampling remains bit-identical to its original implementation.

Candidate builds and exact-image emulator measurements are in the local Multi-Break QA report. Physical FM-1 listening and long-session performance remain separate checks. More simultaneous sample DSP uses more CPU; this removes the arbitrary engine-count restriction, not the hardware's processing limit.
