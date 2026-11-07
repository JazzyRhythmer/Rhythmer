#!/bin/sh
set -eu
cd "$(dirname "$0")/rhythmer"
export RHYTHMER_BUILD_DIR="${RHYTHMER_BUILD_DIR:-../build/firmware}"
export RHYTHMER_EMBEDDED_BANKS="${RHYTHMER_EMBEDDED_BANKS:-../assets/breaks}"
exec python3 tools/build.py --product FM-1_748005 "$@"
