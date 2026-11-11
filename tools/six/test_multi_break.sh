#!/bin/sh
# Native regression and deterministic one/two-core playback; no physical device I/O.
set -eu
test -d /Volumes/SSD/Development || { echo 'Configured SSD is not mounted.' >&2; exit 1; }
cd "$(dirname "$0")/../.."
: "${RHYTHMER_BUILD_DIR:?Set the candidate build directory containing gen/}"
: "${RHYTHMER_SAMPLE_DIR:?Set the directory containing amen-160.slot and think-160.slot}"
OUT=${RHYTHMER_QA_DIR:-$RHYTHMER_BUILD_DIR}
test -d "$OUT" || { echo 'Create/register the QA directory with ssd-qa-output first.' >&2; exit 1; }
export TMPDIR=/Volumes/SSD/Development/Caches/jazzy/tmp
CC=${CC:-clang}
extra=
if test -f rhythmer/tests/jazzy_host.h; then extra=-DTEST_JAZZY; fi
for testname in multi_break_test multi_break_mix_test; do
 "$CC" -O1 -g -w -fwrapv -fsanitize=address -DFM1_HOST_TEST -DRHYTHMER_USB_HOST=1 -DJUNGLE_AUDIO_CODE= $extra \
  -Irhythmer/tests -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src -Irhythmer/firmware/six \
  "rhythmer/tests/$testname.c" rhythmer/firmware/six/acid_generate.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c \
  -o "$OUT/$testname" -lm
done
"$OUT/multi_break_test" "$RHYTHMER_SAMPLE_DIR/amen-160.slot" "$RHYTHMER_SAMPLE_DIR/think-160.slot"
for count in 2 4 6; do
 for core in 0 1; do
  "$OUT/multi_break_mix_test" "$RHYTHMER_SAMPLE_DIR/amen-160.slot" "$RHYTHMER_SAMPLE_DIR/think-160.slot" \
   "$core" "$OUT/multi-break-$count-$core.pcm" "$OUT/multi-break-$count.project" "$count"
 done
 cmp "$OUT/multi-break-$count-0.pcm" "$OUT/multi-break-$count-1.pcm"
done
echo 'Independent Break tracks, target GEN and two-core PCM: PASS'
