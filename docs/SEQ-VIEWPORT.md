# SEQ note visibility / beta.2

Rhythmer bypassed the legacy graph cache function, which also called the piano-roll register follower. As a result the graph could show the wrong octave. The renderer now follows the current bar before caching and includes the viewport register in its redraw signature.

Opening SEQ, switching tracks/bars and editing notes now show the relevant register. An unusually wide range still follows the selected step; the display has a finite number of pitch rows. Notes, playback, the storage layout, audio engines and update loader are unchanged.

Seven host assertions pass through the actual frame/render path, including low Acid, high synth, MIDI 0/127, stopped edits and Break commands. Two actual-firmware screen cases and nine navigation/load cases passed in the community emulator. The exact beta.2 binary has not been physically tested. Beta.1 owner tests remain identified by their older hash.

The source regression is rhythmer/tests/six_seq_viewport_test.c. RELEASE-VALIDATION.json binds the tested results and active source hashes to the package.
