# Rhythmer OS opening film

36 seconds / 1920x1080 / 30 fps. Host-rendered firmware UI, not physical FM-1 footage. The film covers every machine in the README gallery: DRUM, WAVETABLE, PHASE, LOFI, ANALOG, FM6, WHEEL, FM+STAT, SWAV-SAW, SID-6581, ACID and BREAK. Each model has 1.5 seconds of animated UI before the simultaneous Acid Generator / Breakbeat Manipulator scene and the logo ending.

The original synthesized opening bed continues through the machine sequence. The Amen section begins with the individual Break screen at 23.15 seconds, continues under the Acid/Break scene, then ends with a short chopped fill at 28.4 seconds. Amen credits and separate recording rights are described in site/media/video/NOTICE.txt and the main sample notices.

The code-native renderer is tools/six/render_boot_intro.py (`--extended-machines --output YOUR_FOLDER`), using the existing font and animated UI assets. tools/six/mix_intro_amen.py overlays a prepared three-second / two-bar / 160 BPM Amen WAV (`--length 36 --scene-start 23.15 --bars 3`). Requires Python 3, Pillow, NumPy and imageio-ffmpeg. Audio in this film is an edited soundtrack, not a recording of the physical device.
