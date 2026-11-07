#!/bin/sh
set -eu
test -d /Volumes/SSD/Development || { echo "Configured SSD is not mounted." >&2; exit 1; }
export RHYTHMER_BUILD_DIR="${RHYTHMER_BUILD_DIR:-/Volumes/SSD/Development/BuildCache/rhythmer/performance}"
export TMPDIR=/Volumes/SSD/Development/Caches/rhythmer/tmp
export PYTHONPYCACHEPREFIX=/Volumes/SSD/Development/Caches/rhythmer/python
mkdir -p "$TMPDIR"
cd "$(dirname "$0")/../../rhythmer"
export PATH="/Volumes/SSD/Development/Toolchains/fm1-six-tooling/bin:/usr/bin:/bin:/usr/sbin:/sbin"
export JIELI_NERDCTL=1
export JIELI_GUEST_TOOLCHAIN=/Volumes/SSD/fm1-six-tooling/jieli/toolchain
export JIELI_TOOLCHAIN=/Volumes/SSD/Development/Toolchains/fm1-six-tooling/jieli/toolchain
export RHYTHMER_EMBEDDED_BANKS="${RHYTHMER_EMBEDDED_BANKS:-../build/six-ssd}"
export AC79_SDK=/Volumes/SSD/Development/Toolchains/fm1-six-tooling/fw-AC79_AIoT_SDK
exec /Volumes/SSD/Development/Toolchains/fm1-six-tooling/python/bin/python3 tools/build.py "$@"
