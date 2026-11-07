#!/bin/sh
set -eu
test -d /Volumes/SSD/Development || { echo "Configured SSD is not mounted." >&2; exit 1; }
export RHYTHMER_BUILD_DIR="${RHYTHMER_BUILD_DIR:-/Volumes/SSD/Development/BuildCache/rhythmer/performance}"
export TMPDIR=/Volumes/SSD/Development/Caches/rhythmer/tmp
export PYTHONPYCACHEPREFIX=/Volumes/SSD/Development/Caches/rhythmer/python
mkdir -p "$TMPDIR"
cd "$(dirname "$0")/../.."
WASI_CLANG=${WASI_CLANG:-/Volumes/SSD/Development/Toolchains/fm1-six-tooling/wasi-sdk-27.0-arm64-macos/bin/clang}
"$WASI_CLANG" -w -Wno-error=implicit-function-declaration -O2 -fwrapv -ffp-contract=off -DJUNGLE_AUDIO_CODE= -Isite/native/wasi-shim -I"${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen" -Irhythmer/firmware/src -mexec-model=reactor site/native/editor_workspace.c rhythmer/firmware/six/bass303.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/six303_bridge.c -lm -Wl,--export=ws_input -Wl,--export=ws_output -Wl,--export=ws_flash -Wl,--export=ws_archive_ptr -Wl,--export=ws_init -Wl,--export=ws_boot -Wl,--export=ws_request -Wl,--export=ws_archive -Wl,--export=ws_project -Wl,--export=ws_section_current -Wl,--export=ws_section_save -Wl,--export=ws_project_current -Wl,--export=ws_project_mask -Wl,--export=ws_acid -Wl,--export=ws_runtime_flash -Wl,-z,stack-size=1048576 -Wl,--initial-memory=33554432 -o "${RHYTHMER_BUILD_DIR:-/Volumes/SSD/Development/BuildCache/rhythmer/performance}/workspace.wasm"
