# Installer maintenance 20261116.1

RHYTHMER and JAZZY retain firmware build 20261112.1 and its original hashes, single-Break engine, bundled Amen A / Think B, sound, polyphony, CPU policies and storage format. No firmware or Web Play binary is changed. The deferred November 14 firmware candidate is not used.

## Confirmed installer code paths corrected
- Recognize JAZZY MIDI port names, in addition to FM-1 and RHYTHMER.
- Bound driver port opening and add the 300 ms CoreMIDI settling delay also used by the native installer.
- Bound download headers AND response bodies; a stalled network request no longer leaves preparation waiting indefinitely.
- Enforce transfer idle/progress/whole-phase deadlines even while unrelated or repeated MIDI messages arrive. Reject zero-length/out-of-range reads and cap pending SysEx messages.
- Retry transient duplicate endpoint visibility within the existing bounded reconnect window; do not choose an arbitrary device or output.
- A staging-port disconnect after valid reads advances only if the actual expected update loader subsequently answers. A disconnected write or failed finish acknowledgment is never accepted as successful installation.
- Close only the MIDI endpoints opened by this installer. Retain the shared device lock, source review, full musical backup and target hash/checkpoint gates.
- Retry read-only final bank verification on timeout. Keep final-check failures distinct from confirmed write completion.
- Show waiting time even when byte counts exist, warn when the writing tab is hidden, and retain the last request / endpoint scan in downloadable diagnostics.

The running firmware can still reject OTA entry or an incompatible official boot head. Browser changes cannot bypass those checks or recover hardware exposing no responding USB MIDI loader. A separate bootloader tool accepting the same image is consistent with a failed staging/browser path but does not prove which path caused a user report.

## Manuals
Both guides now describe supported V15/V16 starting points, matching recovery files, companion direct updates, phase-specific waiting/retry guidance, final-check uncertainty and the distinction between musical archives and sample audio. JAZZY project/section wording is corrected. The complete RHYTHMER PDF is regenerated from the same 18 web sections.

## Verification
22 transport/progress regression assertions, 4 network-deadline assertions (including a stalled response body), 13 source/package/backup/device-ambiguity assertions, module syntax checks, and local browser package/manifest/V16 gating checks. Both current firmware packages and their embedded sample-bank metadata remain unchanged. Physical installation was not performed and the specific field reports are not claimed reproduced without their stage/log details.

Transport retains GPL-3.0-only Felucca/JIANT attribution. Sources: https://github.com/juanjiant-bit/JIANT and the preserved corresponding sources served with build 20261112.1. This is a focused maintenance patch, not a wholesale upstream firmware update.
