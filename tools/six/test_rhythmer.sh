#!/bin/sh
set -eu
test -d /Volumes/SSD/Development || { echo "Configured SSD is not mounted." >&2; exit 1; }
export RHYTHMER_BUILD_DIR="${RHYTHMER_BUILD_DIR:-/Volumes/SSD/Development/BuildCache/rhythmer/performance}"
export TMPDIR=/Volumes/SSD/Development/Caches/rhythmer/tmp
export PYTHONPYCACHEPREFIX=/Volumes/SSD/Development/Caches/rhythmer/python
mkdir -p "$TMPDIR"
cd "$(dirname "$0")/../.."
clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_integration_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_integration_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_integration_test ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-demo.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-mono.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-audition.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-wavetable-stress.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-m2stat-stress.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-m2saw-stress.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-m2sid-stress.project
clang -w -O0 -fsanitize=address,undefined rhythmer/tests/six_upload_test.c -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_upload_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_upload_test build/six/original-loop-1.slot

clang -w -O0 -g -fwrapv -fsanitize=address,undefined rhythmer/tests/six_break_test.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_break_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_break_test

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_gen_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_gen_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_gen_test

clang -w -O1 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/wavetable_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/wavetable_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/wavetable_test

clang -w -O1 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_models_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_models_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_models_test

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_flex_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_flex_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_flex_test ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-sixacid-stress.project ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-fiveacid-break-stress.project

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_navigation_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_navigation_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_navigation_test

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_break_seq_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_break_seq_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_break_seq_test
clang -w -O1 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen -Irhythmer/firmware/src rhythmer/tests/six_drum_core_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_drum_core_test
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_drum_core_test 0 ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-drum-core-one.pcm
${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_drum_core_test 1 ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-drum-core-two.pcm
cmp ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-drum-core-one.pcm ${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-drum-core-two.pcm

for case in six_display six_boot six_governor six_break_core six_ensemble six_lfo six_empty six_activation six_gen_policy six_efficiency six_native_acid six_break_idle audio; do
 clang -w -O1 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"${RHYTHMER_BUILD_DIR:-rhythmer/build}/gen" -Irhythmer/firmware/src "rhythmer/tests/${case}_test.c" rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "${RHYTHMER_BUILD_DIR:-rhythmer/build}/${case}_test"
done
"${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_display_test" "${RHYTHMER_BUILD_DIR:-rhythmer/build}"
"$RHYTHMER_BUILD_DIR/six_boot_test" "$RHYTHMER_BUILD_DIR"
"$RHYTHMER_BUILD_DIR/six_lfo_test" "$RHYTHMER_BUILD_DIR"
"${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_governor_test"
"$RHYTHMER_BUILD_DIR/six_efficiency_test"
"$RHYTHMER_BUILD_DIR/six_native_acid_test"
"$RHYTHMER_BUILD_DIR/six_break_idle_test"
"$RHYTHMER_BUILD_DIR/audio_test"
"$RHYTHMER_BUILD_DIR/six_empty_test" "$RHYTHMER_BUILD_DIR"
"$RHYTHMER_BUILD_DIR/six_activation_test"
"$RHYTHMER_BUILD_DIR/six_gen_policy_test"
clang -w -O1 -g -fsanitize=address,undefined -DT_C1=1 -DHALF_FRAMES=$(sed -n 's/^#define HALF_FRAMES \([0-9]*\).*/\1/p' rhythmer/firmware/src/core.h) rhythmer/tests/six_usb_guard_test.c -lm -o "$RHYTHMER_BUILD_DIR/six_usb_guard_test"
"$RHYTHMER_BUILD_DIR/six_usb_guard_test"
"${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_ensemble_test" "$RHYTHMER_BUILD_DIR"
"${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_break_core_test" 0 "${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-break-core-one.pcm" build/six/amen-160.slot
"${RHYTHMER_BUILD_DIR:-rhythmer/build}/six_break_core_test" 1 "${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-break-core-two.pcm" build/six/amen-160.slot
cmp "${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-break-core-one.pcm" "${RHYTHMER_BUILD_DIR:-rhythmer/build}/six-break-core-two.pcm"

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src rhythmer/tests/six_md_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/six_md_test"
"$RHYTHMER_BUILD_DIR/six_md_test" 0 "$RHYTHMER_BUILD_DIR/md-one.pcm"
"$RHYTHMER_BUILD_DIR/six_md_test" 1 "$RHYTHMER_BUILD_DIR/md-two.pcm"
cmp "$RHYTHMER_BUILD_DIR/md-one.pcm" "$RHYTHMER_BUILD_DIR/md-two.pcm"
clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src rhythmer/tests/six_capacity_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/six_capacity_test"
"$RHYTHMER_BUILD_DIR/six_capacity_test" "$RHYTHMER_BUILD_DIR"

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src rhythmer/tests/six_break_tools_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/six_break_tools_test"
"$RHYTHMER_BUILD_DIR/six_break_tools_test" "$RHYTHMER_BUILD_DIR"

clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src rhythmer/tests/six_break_modulation_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/six_break_modulation_test"
"$RHYTHMER_BUILD_DIR/six_break_modulation_test" "$RHYTHMER_BUILD_DIR"

clang -w -O1 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src rhythmer/tests/six_seq_viewport_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/six_seq_viewport_test"
"$RHYTHMER_BUILD_DIR/six_seq_viewport_test"

# Complete named MD bank, genre expansion and both Acid adapters.
for case in six_upgrade six_drum_voice six_hortator_core six_gen_navigation six_phaser_precision six_gen_bass; do
 clang -w -O1 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src "rhythmer/tests/${case}_test.c" rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/${case}_test"
done
"$RHYTHMER_BUILD_DIR/six_upgrade_test" "$RHYTHMER_BUILD_DIR"
"$RHYTHMER_BUILD_DIR/six_gen_bass_test" "$RHYTHMER_BUILD_DIR/bass-distribution.csv"
"$RHYTHMER_BUILD_DIR/six_drum_voice_test" "$RHYTHMER_BUILD_DIR/drum-voice.ppm"
"$RHYTHMER_BUILD_DIR/six_gen_navigation_test"
"$RHYTHMER_BUILD_DIR/six_phaser_precision_test"
for core in 0 1; do
 "$RHYTHMER_BUILD_DIR/six_hortator_core_test" "$core" "$RHYTHMER_BUILD_DIR/hort-${core}.pcm" "${RHYTHMER_SAMPLES_DIR:-build/six-ssd}" "$RHYTHMER_BUILD_DIR"
done
cmp "$RHYTHMER_BUILD_DIR/hort-0.pcm" "$RHYTHMER_BUILD_DIR/hort-1.pcm"

# Selective MIDI generation and automatic Break routing.
clang -w -O0 -g -fwrapv -DJUNGLE_AUDIO_CODE= -fsanitize=address,undefined -I"$RHYTHMER_BUILD_DIR/gen" -Irhythmer/firmware/src rhythmer/tests/six_gen_target_test.c rhythmer/firmware/six/jungle.c rhythmer/firmware/six/tb3po.c rhythmer/firmware/six/acid_generate.c -lm -o "$RHYTHMER_BUILD_DIR/six_gen_target_test"
"$RHYTHMER_BUILD_DIR/six_gen_target_test" "$RHYTHMER_BUILD_DIR"
