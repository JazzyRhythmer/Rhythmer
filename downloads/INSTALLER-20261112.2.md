# Installer maintenance — 20261112.2

Both RHYTHMER and JAZZY installers now accept official V15 and V16 as installation sources. The recovery selector accepts only pinned, unmodified official images. V15 remains included; V16 is selected locally from M-VAVE's official download and is not redistributed in this change. A unit currently reporting V16 must have its matching V16 recovery file selected before installation. Companion firmware-to-firmware updates retain the existing backup and review gates; unrelated custom firmware still requires its documented return-to-official route.

Recovery retains the firmware's boot-area comparison. It never forces a V15/V16 downgrade or bypasses a boot-area mismatch. If a custom firmware's original boot version is unknown, choose the known original version; a refusal while comparing the boot area is reported with the request count and guidance, not treated as permission to overwrite it.

Reports add installer revision, source identity and MIDI ports, selected official recovery hash, browser capabilities, error code, phase and request counters. They explicitly mark the flash JEDEC ID unavailable through the current MIDI protocol. Both shipped firmwares already accept Puya 0x856014 and UC25HQ80 0xB36014; older installed firmware can still refuse update entry. No ROM/WebUSB/Transporter recovery is claimed.

## Validation

- Browser: official V15 and V16 verification, corrupted V16 rejection, matching-source policy, backups/final-review gates and unknown firmware/loader rejection.
- Browser UI: all four install/recovery screens, V16 file requirement, successful original-file validation, selected recovery identity, and switching back to included V15.
- Native: both unchanged firmware OTA staging implementations tested with official V15, official V16 and both custom packages, against both original boot heads. Matching recoveries and custom-loader staging succeed; mismatched official heads return -5 with no commit or writes outside the staging area.
- This is an installer-only maintenance release. Firmware binaries, sound, FX, voice allocation, polyphony, buffers and scheduling are unchanged. Real-device V16/new-chip installation is not established by host/browser simulation.

## References and attribution

Package verification and source routing reviewed against JIANT's public V10.7.17 installer; refusal analysis cross-checked against Skiff 2.5.1. Upstream copyright and GPL-3.0-only notices remain. JIANT main source and its live public installer do not have identical version provenance, so a wholesale replacement was not performed.

https://juanjiant-bit.github.io/JIANT/webapp/installer/
https://illuminus5.github.io/skiff/webapp/installer/
https://www.cuvave.com/download
