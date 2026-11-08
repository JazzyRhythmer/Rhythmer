# Rhythmer videos — controls in motion

Media-only revision; firmware stays v0.1.1-beta3 / build 20261108.9, SHA-256 `8b9d4af568135e76465ca73d20c9e544753babc1dcac60f7a0f76ebd6fa2feb1`. No firmware or sample-bank bytes changed.

- Manual: 64 seconds, 1920×1080 and 1080×1920, 30 fps. Current firmware actions replace static display examples: four-track playback, the actual six-track notice and confirmation, Acid NEW/MUTATE, genre Break generation, held-SEQ commands and the resulting playing steps, individual drum selection and targeted GEN. Actual browser search and sample conversion are also shown.
- The new original score, Quiet Circuit, is warm 120 BPM broken-house with soft keys, bass, restrained percussion and short control ticks. It uses no third-party recordings. Both manual exports measure −17.01 LUFS / −2.14 dBTP after AAC encoding.
- Update: YouTube 68 seconds and Reels 52 seconds. ONE TRACK shows TARGET T2 → T3 → T4, not seed changes. GENERATE THE BREAKS shows BOOM BAP, JUNGLE and DRUNKEN HH selections followed by different sequencer results. A fixed seed verifies the style difference. The original update AAC soundtrack is bit-identical.

All four files passed complete video/audio decode and 41 comparisons against the expected encoded scenes. Actual changing LCD capture regions are checked independently of editorial progress graphics. These are host-rendered firmware walkthroughs, not camera footage of physical hardware. The soundtrack is editorial music, not a recording of the example pattern.

Evidence: `www/assets/video/rhythmer-motion-tours-v4-validation.json`. Source: `tools/six/capture_manual_motion.c`, `score_manual_motion.py`, `render_motion_tours_v4.py` and `audit_motion_tours_v4.py`.
