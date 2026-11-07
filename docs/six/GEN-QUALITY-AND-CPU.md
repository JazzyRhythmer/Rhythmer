# GEN quality and ensemble headroom

Revision of 0.1.1-beta3; the version label is unchanged. Package hashes identify the actual build.

## Musical choices

The existing voice-leading planner keeps common tones and penalizes large jumps and muddy low-register intervals. Ableton's [voicings lesson](https://learningmusic.ableton.com/advanced-topics/voicings.html) explains why register and inversion change a chord's character without changing its harmony. This is a design reference, not imported Ableton music or code.

Curated genres now keep factory oscillator tuning, ratios and wave settings instead of perturbing every continuous engine control. Bass selection uses bass-tagged presets; chord parts use keys/pad presets, and ordinary melodic parts exclude effect presets. The light-engine pool and one automatically assigned Serum ceiling remain. Manual browsing is unrestricted.

Seventh/ninth colour is consistent within a generated progression. Modal profiles may deliberately retain a pedal rather than forcing every seed into a progression. Foreground melodies leave alternate-bar space for an answer; G-funk hooks and abstract cycles retain their own roles. Trip-hop, lo-fi house and atmospheric breaks use sparser foreground figures. Ambient IDM has a pedal option. These are composition heuristics, not a promise to reproduce any artist.

Magenta's [GrooVAE research](https://magenta.tensorflow.org/groovae) distinguishes the event pattern from velocity and microtiming. Rhythmer uses that distinction in its existing repeated groove cells and push/pull dynamics. No trained model or Magenta dataset is embedded in the firmware. Timing is not independent random jitter on every step.

## ALL and explicit targets

ALL owns T1–T4, including its macro assignment and playhead reset. Existing T5/T6 notes, sounds and local settings are preserved. T5 MIDI must be explicitly targeted; BREAK finds its assigned track. Explicit targets keep tempo, sounds, global settings, automation and other tracks.

## CPU investigation

Read-only measurements of the owner's running pre-revision AA90 firmware showed both cores active. Acid (HT SCREAMER) averaged approximately 74–76 microseconds per 32-frame block; shared FX approximately 203–205, and Break approximately 152–153. Removing Acid reduced average render cost but still produced a roughly 12.3 ms worst half against an 11.61 ms deadline. These are observations of one changing owner session, not a controlled benchmark or evidence about the revised binary.

The shared four-stage phaser now uses integer sample processing with high-resolution coefficients. An independent double-precision reference test measures approximately 64.7 dB signal-to-error ratio across changing depth, feedback, phase and silence. Break reads adjacent packed PCM12 frames together; core-comparison PCM and boundary tests check correctness. Neither change reduces the normal shared voice budget or removes a track.

Physical sustained-load comparison and listening on the revised exact package remain necessary. Native tests and community-emulator timings do not model the FM-1's shared flash-cache contention completely.
