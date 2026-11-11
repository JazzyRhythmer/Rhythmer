# Factory FX defaults · build 20261111.4

Break A/B factory sounds now load with DIST / PHASER / DLY / REV all at 0%. Acid factory sounds load with DIST 30%, PHASER 0%, DLY 0%, REV 10%. These defaults apply to factory preset loads, INIT SOUND, new projects and startup defaults. Saved project and user sound effect settings are preserved.

Factory sound decoding is shared between normal preset loading and legacy project conversion. JAZZY retains all 31 engines and 1,221 presets; it now has 176 bytes of application image headroom. The shared loader is main-loop work and adds no per-sample DSP processing. No audio buffers, stack guards, synthesis models or preset data were removed.

Both bundled Break banks (Amen A / Think B) are unchanged. Multi-Break independent playback, shared A/B samples and GEN targeting T1–T6 remain supported. Back up your music and original custom loops before installing: installation replaces both sample banks with the included factory loops.

Software validation: every factory preset decoded identically to the previous loading algorithm (JAZZY 1,221; RHYTHMER 178). Both OSes pass 62 default/load/project-preservation checks and constrained firmware builds. Emulator validation is separate from physical FM-1 installation and endurance testing.
