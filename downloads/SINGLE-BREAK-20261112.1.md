# Restored playback base · 20261112.1

This release restores the single-Break playback implementation from the preserved JAZZY 20261111.2 and RHYTHMER 20261110.3 sources. Their unmodified rebuilds differ from the original application binaries only in the displayed compile date; all other program bytes match. Track-state allocation, audio buffers, scheduling and the earlier generator behavior are restored rather than reimplemented.

Only these firmware fixes are applied: HYDRA enables its high-note oversampling when a legato/glide phrase crosses the pitch threshold; LXR uses the track ADSR together with its native drum decay, with filter and FX processing retained; missing factory FX sends default to zero, while explicit preset FX and saved project/user-sound settings remain. Acid and Break load with all four sends at zero. There is no Acid-specific send branch.

Break is limited to one track. GEN uses the earlier track targets and dedicated BREAK target again. Keep projects from newer versions backed up; projects containing several Break tracks must be reduced to one Break before use with this version.

Both bundled Break banks are unchanged: A / Amen and B / Think. Installation replaces A/B, so keep your original custom audio and converted .slot files. RHYTHMER and JAZZY direct updates remain supported; V15 is not required between the two companion firmwares. Save the automatic musical backup before installation.

Software checks cover both constrained firmware builds, all available factory presets, dry defaults and saved mix preservation, all 14 HYDRA presets crossing the high-note threshold, and release termination across all 595 LXR presets. Physical FM-1 installation and endurance validation are separate. The reported broad playback regression has not been declared fixed by software tests alone; this rollback restores the earlier playback base for device verification.
