# Build Rhythmer

See jiant/BUILDING.md for the pinned JieLi toolchain, AC79 SDK, Docker/Linux and font prerequisites. Run `sh build.sh`; the wrapper selects Rhythmer's independent app/loader identity and includes both prepared sample slots. Output is build/firmware. Set JIELI_TOOLCHAIN, AC79_SDK and RHYTHMER_BUILD_DIR for your environment. On a Mac with the configured development SSD, keep outputs and caches there.

The active source is jiant/firmware. Do not use an upstream X0X release workflow. The checked-in www/ directory is the reviewed static website, including the matching app/loader/sample package, browser runtime and browser data engine. The Pages workflow publishes only www/. The original browser runtime is retained with its source links and MIT notice in site/vendor/fm1-emu.

An emulator/host result is not a hardware result. See RELEASE-VALIDATION.json and re-test the exact rebuilt package before publishing a different binary. Replacing the bundled samples changes the release checksum and requires reviewing the sample notices too.
