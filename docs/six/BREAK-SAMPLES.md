# Local break samples

A: **Amen**, from Amenity `assets/third-party/amen/amen.wav`. The Winstons, “Amen, Brother” (1969); drums Gregory C. Coleman, composition Richard L. Spencer. Original Amenity audio notice remains in its reference checkout. Its supplied 48 kHz / stereo / 24-bit loop is already 144,000 frames, two bars at 160 BPM.

B: **Think drums**, Lyn Collins, “Think (About It)”. Current edit uses the owner's `lyncollins_think_1.wav` (93,761 stereo PCM16 frames at 44.1 kHz) and `lyncollins_think_2.wav` (92,698 frames). `lyncollins_think_2wav.wav` is byte-identical to file 2. Each supplied loop is treated as one four-beat bar. File 1's last beat has a pitched tail; file 2's first beat has pitched carry-in. Those intervals are excluded. The drum-only assembly is file 1 beats 1–3 + file 2 beat 4, followed by file 1 beat 1 + file 2 beats 2–4. This preserves two complementary drum phrases without source-separation processing.

Each source bar is anti-alias repitched to exactly 24,000 samples (1.5 s); assembled output is exactly 48,000 samples / two bars at 160 BPM. All four joins have 0.5 ms attack / 3 ms tail microfades without changing length, with zero quantized sample jumps at 1.125, 1.5, 1.875 and 3.0 seconds. Kick/snare pitch naturally remains; musical bass/horn/voice intervals identified in the source spectrogram are excluded. Physical-device listening is a separate check. The previous Rhythm Lab part1 edit is retained only under historical `build/six`. The current bank is `build/six-ssd/think-160.slot` (SSD BuildCache).


[Reason Studios' Jungle 101](https://www.reasonstudios.com/news/post/jungle-101-the-beat) introduces Think immediately after Amen among foundational jungle breaks. This is the rationale for slot B, not a claim of an independently measured global ranking.

Both loops: mono, DC removed, Kaiser polyphase anti-alias resampling, exactly 48,000 frames / 3.000 seconds / 8 beats at 160 BPM, -1 dBFS peak, packed signed PCM12 at 16 kHz. Endpoint fades are 0.5 ms at the start and 3 ms at the end; loop length does not change. Both quantized endpoints are zero, eliminating a discrete sample jump at the repeat seam. This mathematical seam check does not replace listening on the physical FM-1.

Each payload is 72,000 bytes in an 81,920-byte flash slot. Total payload 144,000 bytes. Existing two-voice A/B crossfade reads flash directly; these sample changes add no runtime RAM buffer or processing stage.

## Reproduce and inspect

`tools/six/prepare_think_drums.py FIRST_WAV SECOND_WAV --out /Volumes/SSD/Development/BuildCache/rhythmer/samples` (dedicated NumPy/SciPy environment). Amen is copied byte-for-byte from the historical prepared bank. `build/six-ssd/break-samples.json` records input and packed-slot SHA-256, crop, rates, gain, seam, RMS and quantization results. `amen-160-repeat.wav` and `think-160-repeat.wav` are 12-second previews of four repetitions of the actual quantized sample.

`test_rhythmer_emulator.py` inserts `amen-160.slot` at A0000 and `think-160.slot` at B4000, and creates `rhythmer/build-ssd/Rhythmer-six-track-test.fm1save`. Format/upload tests continue to use separate synthetic `original-loop-*` fixtures, so rerunning those tests does not replace the real break slots.

These original recordings have separate rights belonging to their respective rights holders; the project's GPL does not grant rights to the recordings. They are local user-requested demonstration assets, not a royalty-free sample pack. No public release or GitHub publication has been performed.

## Installer and user replacements

The current local installation container and ZIP include both packed banks and automatically install them after firmware verification on a fresh device. Upgrades preserve current A/B by default. The browser SAMPLES tab also accepts user audio or a validated converted `.slot`, previews quantized output, saves local banks and supports explicitly confirmed single-bank USB upload. See WEB-INSTALLER.md.

The original-recording clearance status is tracked in `site/audio-rights.json`; `tools/six/release_rights.py` blocks a public audio release while documentation is absent. Amenity's notice and Rhythm Lab's availability do not grant us a public firmware/sample redistribution license. Free/non-commercial/local labels are not treated as proof of lawful use. Attribution is retained, and release must use properly cleared material or omit/replace it.
